#include "WpEntityMovementComponent.h"

#include "Warpunk/Game/Entity/WpEntity.h"
#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Entity/WpPooledEntityData.h"

#include "Components/CapsuleComponent.h"

/*
 * v(t) = V * (1 - e ^ (-C / T * t)) / (1 - e ^ (-C))
 * a(t) = V * C * e ^ (-C / T * t) / T / (1 - e ^ (-C))
 * a(v) = V * C / T / (1 - e ^ (-C)) - C / T * v
 * phi(C) = asin(V * C / g / T / (1 - e ^ (-C)))
 * t - time, v - velocity, a - acceleration, phi - max ground incline angle
 * V - max velocity, T - max velocity acceleration time, C - torque coefficient, G - gravity acceleration
 */

const uint32 UWpEntityMovementComponent::MaxMoveIterationNum = 10;

const float UWpEntityMovementComponent::IdealGroundDistance = 1.0f;
const float UWpEntityMovementComponent::MaxGroundDistance = IdealGroundDistance + 1.0f;

const float UWpEntityMovementComponent::WallImpactElasticity = 0.2f;
const float UWpEntityMovementComponent::EntityImpactElasticity = 0.5f;

const float UWpEntityMovementComponent::KineticToStaticGroundFriction = 0.8f;
const float UWpEntityMovementComponent::MinSlippingLateralSpeed = 5.0f;
const float UWpEntityMovementComponent::MinSlippingMagicValueScale = 0.4f;

const float UWpEntityMovementComponent::InterpolatedComponentSmoothingTime = 0.1f;
const float UWpEntityMovementComponent::InterpolatedComponentSnapDeltaLocation2 = FMath::Square(1.0f);
const float UWpEntityMovementComponent::InterpolatedComponentSnapDeltaRotation = FMath::DegreesToRadians(1.0f);

UWpEntityMovementComponent::UWpEntityMovementComponent()
{
	bAutoActivate = false;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetIsReplicatedByDefault(false);

	// NetworkNoSmoothUpdateDistance = 100.0f; todo

	bUpdateNavAgentWithOwnersCollision = false;

	FNavAgentProperties& NavAgentProperties = GetNavAgentPropertiesRef();
	NavAgentProperties.bCanCrouch = false;
	NavAgentProperties.bCanJump = false;
	NavAgentProperties.bCanWalk = true;
	NavAgentProperties.bCanSwim = false;
	NavAgentProperties.bCanFly = false;

	MovementState.bCanCrouch = false;
	MovementState.bCanJump = false;
	MovementState.bCanWalk = true;
	MovementState.bCanSwim = false;
	MovementState.bCanFly = false;
}

void UWpEntityMovementComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* const ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ensureAlways(IsActive());

	if (!GetOwner()->HasAuthority())
	{
		return; // todo
	}

	ensureAlways(GetOwner()->HasAuthority());

	UpdateInterpolatedComponentOffset(DeltaTime);

	const FVector InputVector = ConsumeInputVector();

	if (InputVector.IsNearlyZero()
		&& FMath::IsNearlyZero(WheelRotationAngle)
		&& MoveSpeed.IsNearlyZero() && MoveAcceleration.IsNearlyZero()
		&& FMath::IsNearlyZero(RotationSpeed) && FMath::IsNearlyZero(RotationAcceleration)
		&& bOnGround)
	{
		return;
	}

	UpdateSlipping();
	UpdateWheelRotationAngle(DeltaTime, InputVector.Y);
	UpdateMoveAcceleration(DeltaTime, InputVector);
	UpdateRotationAcceleration(DeltaTime, InputVector);

	MoveSpeed = CalcNewMoveSpeed(DeltaTime);
	RotationSpeed = CalcNewRotationSpeed(DeltaTime);

	Move(DeltaTime);

	UpdateTargetInterpolatedComponentOffset();

	UpdateNavVelocity();
}

void UWpEntityMovementComponent::OnRep_IsActive()
{
	// Override to not enable tick
}

void UWpEntityMovementComponent::UpdateTickRegistration()
{
	// Override to not enable tick
}

void UWpEntityMovementComponent::SetUpdatedComponent(USceneComponent* const NewUpdatedComponent)
{
	ensureAlways(NewUpdatedComponent == NewUpdatedComponent->GetOwner()->GetRootComponent());
	ensureAlways(NewUpdatedComponent->IsA<UCapsuleComponent>());

	Super::SetUpdatedComponent(NewUpdatedComponent);
}

float UWpEntityMovementComponent::GetMaxSpeed() const
{
	return MaxMoveSpeed;
}

FVector UWpEntityMovementComponent::ConsumeInputVector()
{
	FVector InputVector = Super::ConsumeInputVector();

	switch (MovementType)
	{
	case EWpEntityMovementType::Legs:
	case EWpEntityMovementType::Tracks:
		InputVector = InputVector.GetClampedToMaxSize2D(1.0);
		break;
	case EWpEntityMovementType::Wheels:
		if ((InputVector.X > 1.0) || (InputVector.Y > 1.0))
		{
			const float MaxComponent = InputVector.GetMax();
			InputVector /= MaxComponent;
		}
		break;
	default:
		break;
	}

	return InputVector;
}

void UWpEntityMovementComponent::RequestPathMove(const FVector& MoveInput)
{
	const FVector HorizontalMoveInput = MoveInput.GetSafeNormal2D() * MoveInput.Size();

	FVector InputVector = HorizontalMoveInput;

	switch (MovementType)
	{
	case EWpEntityMovementType::Legs:
		break;
	case EWpEntityMovementType::Tracks:
	case EWpEntityMovementType::Wheels:
		{
			const FRotator LocalRotation = FRotationMatrix::MakeFromXZ(UpdatedComponent->GetForwardVector(), FVector::UpVector).Rotator();
			const FTransform LocalTransform = FTransform{LocalRotation};
			InputVector = LocalTransform.InverseTransformVectorNoScale(InputVector);
			InputVector.X = FMath::Sign(InputVector.X);
			InputVector.Y = FMath::Sign(InputVector.Y) * InputVector.X;
		}
		break;
	default:
		break;
	}

	Super::RequestPathMove(InputVector);
}

void UWpEntityMovementComponent::OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
{
	ensureAlways(GetOwner()->HasAuthority());

	if (bIsAcquired)
	{
		const FWpPooledEntityData& PooledEntityData = static_cast<const FWpPooledEntityData&>(PooledObjectData);

		if (PooledEntityData.EntityConfig->Type == EWpEntityType::Unit)
		{
			const FWpMovementConfig& MovementConfig = PooledEntityData.EntityConfig->MovementConfig;

			MovementType = MovementConfig.Type;
			MaxMoveSpeed = MovementConfig.MaxMoveSpeed;
			MoveAccelerationCoefficient1 = MovementConfig.MoveAccelerationCoefficient1;
			MoveAccelerationCoefficient2 = MovementConfig.MoveAccelerationCoefficient2;
			MaxMoveBrakingDeceleration = MovementConfig.MaxMoveBrakingDeceleration;
			MaxMoveInertiaDeceleration = MovementConfig.MaxMoveInertiaDeceleration;
			MaxGroundStaticFriction = MovementConfig.MaxGroundStaticFriction;
			MaxRotationSpeed = MovementConfig.MaxRotationSpeed;
			MaxRotationAcceleration = MovementConfig.MaxRotationAcceleration;
			MaxRotationBrakingDeceleration = MovementConfig.MaxRotationBrakingDeceleration;
			MaxWheelRotationAngle = MovementConfig.MaxWheelRotationAngle;
			MaxWheelEdgeRotationSpeed = MovementConfig.MaxWheelEdgeRotationSpeed;
			MaxWheelCenterRotationSpeed = MovementConfig.MaxWheelCenterRotationSpeed;
			Mass = MovementConfig.Mass;
			MaxInclineAngleCos = MovementConfig.MaxInclineAngleCos;
			MaxStepUpHeight = MovementConfig.MaxStepUpHeight;

			MoveSpeed = FVector::ZeroVector;
			Velocity = FVector::ZeroVector;
			MoveAcceleration = FVector::ZeroVector;
			RotationSpeed = 0.0f;
			RotationAcceleration = 0.0f;
			WheelRotationAngle = 0.0f;
			bOnGround = false;
			GroundNormal = FVector::UpVector;
			bIsSlipping = false;
			SetIsFirstPersonControlled(false);
			CacheInitialInterpolatedComponentOffset();

			FNavAgentProperties& NavAgentProperties = GetNavAgentPropertiesRef();
			NavAgentProperties.AgentStepHeight = MaxStepUpHeight;
		}
		else
		{
			bIsAcquired = false;
		}
	}

	SetActive(bIsAcquired);
}

void UWpEntityMovementComponent::OnPooledObjectVisibilityChanged(const bool bIsVisible)
{
	if (!GetOwner()->HasAuthority())
	{
		// SetActive(bIsVisible); // todo
	}
}

void UWpEntityMovementComponent::SetInterpolatedComponent(USceneComponent* const InInterpolatedComponent)
{
	InterpolatedComponent = InInterpolatedComponent;
}

void UWpEntityMovementComponent::SetCollisionSize(const float Radius, const float Height)
{
	FNavAgentProperties& NavAgentProperties = GetNavAgentPropertiesRef();
	NavAgentProperties.AgentRadius = Radius;
	NavAgentProperties.AgentHeight = Height;
}

float UWpEntityMovementComponent::GetMinBrakingDistance() const
{
	return MoveSpeed.SizeSquared2D() / (2.0f * MaxMoveBrakingDeceleration);
}

void UWpEntityMovementComponent::AddMovementInput(const float Forward, const float Right)
{
	AddInputVector(FVector{Forward, Right, 0.0});
}

void UWpEntityMovementComponent::SetIsFirstPersonControlled(const bool bInIsFirstPersonControlled)
{
	bIsFirstPersonControlled = bInIsFirstPersonControlled;
}

void UWpEntityMovementComponent::UpdateSlipping()
{
	if ((MovementType != EWpEntityMovementType::Tracks) && (MovementType != EWpEntityMovementType::Wheels))
	{
		return;
	}

	if (!bOnGround)
	{
		return;
	}

	bIsSlipping = false;

	if (MovementType == EWpEntityMovementType::Tracks)
	{
		const float MinSlippingMagicValue = MaxMoveSpeed * MaxRotationSpeed * MinSlippingMagicValueScale;
		const float CurrentSlippingMagicValue = MoveSpeed.Size2D() * FMath::Abs(RotationSpeed);
		bIsSlipping |= CurrentSlippingMagicValue > MinSlippingMagicValue;
	}

	const FVector LateralSpeed = MoveSpeed.ProjectOnToNormal(UpdatedComponent->GetRightVector());
	bIsSlipping |= !LateralSpeed.IsNearlyZero(MinSlippingLateralSpeed);

	if (!bIsSlipping)
	{
		MoveSpeed -= LateralSpeed;
	}
}

void UWpEntityMovementComponent::UpdateWheelRotationAngle(const float DeltaTime, const float Input)
{
	if (MovementType != EWpEntityMovementType::Wheels)
	{
		return;
	}

	const float DesiredWheelRotationAngle = Input * MaxWheelRotationAngle;
	const float DeltaWheelRotationAngle = DesiredWheelRotationAngle - WheelRotationAngle;

	if (FMath::IsNearlyZero(DeltaWheelRotationAngle))
	{
		return;
	}

	const bool bRotatesToEdge =
		(FMath::Sign(DesiredWheelRotationAngle) == FMath::Sign(WheelRotationAngle))
		&& (FMath::Abs(DesiredWheelRotationAngle) > FMath::Abs(WheelRotationAngle));
	const float WheelRotationSpeedScale = bRotatesToEdge ? MaxWheelEdgeRotationSpeed : MaxWheelCenterRotationSpeed;
	const float WheelRotationSpeedDirection = FMath::Sign(DeltaWheelRotationAngle);
	const float WheelRotationSpeed = WheelRotationSpeedScale * WheelRotationSpeedDirection;
	WheelRotationAngle = FMath::Clamp(WheelRotationAngle + WheelRotationSpeed * DeltaTime, -MaxWheelRotationAngle, MaxWheelRotationAngle);

	const float NewDeltaWheelRotationAngle = DesiredWheelRotationAngle - WheelRotationAngle;
	const bool bOverrun = FMath::Sign(NewDeltaWheelRotationAngle) != FMath::Sign(DeltaWheelRotationAngle);
	if (bOverrun)
	{
		WheelRotationAngle = DesiredWheelRotationAngle;
	}
}

void UWpEntityMovementComponent::UpdateMoveAcceleration(const float DeltaTime, const FVector& InputVector)
{
	MoveAcceleration = FVector::ZeroVector;

	if (!bOnGround)
	{
		return;
	}

	switch (MovementType)
	{
	case EWpEntityMovementType::Legs:
		ApplyInputMoveAcceleration2D(InputVector);
		break;
	case EWpEntityMovementType::Tracks:
	case EWpEntityMovementType::Wheels:
		ApplyInputMoveAcceleration1D(InputVector.X);
		break;
	default:
		break;
	}

	ApplyMoveForces();

	ApplyMoveFrictionLimit();

	const bool bIsBrakingToZeroMoveSpeed = !MoveSpeed.IsNearlyZero()
		&& (
			(MovementType != EWpEntityMovementType::Legs) && FMath::IsNearlyZero(InputVector.X)
			|| InputVector.IsNearlyZero()
		);
	if (bIsBrakingToZeroMoveSpeed)
	{
		const bool bMoveSpeedWillChangeSign = MoveSpeed.Dot(CalcNewMoveSpeed(DeltaTime)) < 0.0;
		if (bMoveSpeedWillChangeSign)
		{
			MoveSpeed = FVector::ZeroVector;
			MoveAcceleration = FVector::ZeroVector;
			bIsSlipping = false;
		}
	}
}

void UWpEntityMovementComponent::ApplyInputMoveAcceleration1D(const float Input)
{
	ensureAlways(bOnGround);

	const FVector Direction = UpdatedComponent->GetForwardVector();
	const FVector DesiredSpeed = Input * MaxMoveSpeed * Direction;
	const FVector CurrentSpeed = MoveSpeed.ProjectOnToNormal(Direction);

	if (DesiredSpeed.Equals(CurrentSpeed))
	{
		return;
	}

	const bool bShouldAccelerate = CurrentSpeed.IsNearlyZero()
		|| (DesiredSpeed.Dot(CurrentSpeed) > 0.0) && (DesiredSpeed.SizeSquared2D() > CurrentSpeed.SizeSquared2D());
	if (bShouldAccelerate)
	{
		const float AccelerationScale = MoveAccelerationCoefficient1 + MoveAccelerationCoefficient2 * MoveSpeed.Size2D();
		MoveAcceleration += AccelerationScale * DesiredSpeed.GetUnsafeNormal2D();
	}
	else
	{
		float DecelerationScale;

		const bool bDeceleratesDownIncline = CurrentSpeed.Dot(GroundNormal) > 0.0;
		if (bDeceleratesDownIncline)
		{
			DecelerationScale = MoveAccelerationCoefficient1;
		}
		else
		{
			const bool bDeceleratesToStop = DesiredSpeed.IsNearlyZero();
			if (bDeceleratesToStop)
			{
				const bool bShouldBrake = !bIsFirstPersonControlled;
				DecelerationScale = bShouldBrake ? MaxMoveBrakingDeceleration : MaxMoveInertiaDeceleration;
			}
			else
			{
				DecelerationScale = MaxMoveBrakingDeceleration;
			}
		}

		MoveAcceleration += -DecelerationScale * CurrentSpeed.GetUnsafeNormal2D();
	}
}

void UWpEntityMovementComponent::ApplyInputMoveAcceleration2D(const FVector& InputVector)
{
	ensureAlways(bOnGround);

	FVector DirectionForward, DirectionRight;
	if (bIsFirstPersonControlled)
	{
		DirectionForward = GetOwner<APawn>()->GetController()->GetControlRotation().Vector().GetUnsafeNormal2D();
		DirectionRight = FVector::UpVector.Cross(DirectionForward);
	}
	else
	{
		DirectionForward = FVector::ForwardVector;
		DirectionRight = FVector::RightVector;
	}

	const FVector CurrentSpeed = MoveSpeed * (FVector::ForwardVector + FVector::RightVector);
	const FVector DesiredSpeed = MaxMoveSpeed * (InputVector.X * DirectionForward + InputVector.Y * DirectionRight);
	const FVector DeltaSpeed = DesiredSpeed - CurrentSpeed;

	if (DeltaSpeed.IsNearlyZero())
	{
		return;
	}

	const FVector CurrentSpeedForward = CurrentSpeed.ProjectOnToNormal(DirectionForward);
	const FVector CurrentSpeedRight = CurrentSpeed - CurrentSpeedForward;
	const FVector DeltaSpeedForward = DeltaSpeed.ProjectOnToNormal(DirectionForward);
	const FVector DeltaSpeedRight = DeltaSpeed - DeltaSpeedForward;

	const bool bMovesDownIncline = CurrentSpeed.Dot(GroundNormal) > 0.0;
	const float DecelerationScale = bMovesDownIncline ? MoveAccelerationCoefficient1 : MaxMoveBrakingDeceleration;

	FVector InputAcceleration = FVector::ZeroVector;

	if (!DeltaSpeedForward.IsNearlyZero())
	{
		const bool bShouldAccelerate = CurrentSpeedForward.IsNearlyZero() || (CurrentSpeedForward.Dot(DeltaSpeedForward) > 0.0f);
		const float InputAccelerationScale =
			(bShouldAccelerate
				 ? MoveAccelerationCoefficient1 + MoveAccelerationCoefficient2 * CurrentSpeedForward.Size2D()
				 : DecelerationScale)
			* FMath::Abs(DeltaSpeed.GetSafeNormal2D().Dot(DirectionForward));
		const FVector InputAccelerationDirection = DeltaSpeedForward.GetUnsafeNormal2D();
		InputAcceleration += InputAccelerationScale * InputAccelerationDirection;
	}

	if (!DeltaSpeedRight.IsNearlyZero())
	{
		const bool bShouldAccelerate = CurrentSpeedRight.IsNearlyZero() || (CurrentSpeedRight.Dot(DeltaSpeedForward) > 0.0f);
		const float InputAccelerationScale =
			(bShouldAccelerate
				 ? MoveAccelerationCoefficient1 + MoveAccelerationCoefficient2 * CurrentSpeedRight.Size2D()
				 : DecelerationScale)
			* FMath::Abs(DeltaSpeed.GetSafeNormal2D().Dot(DirectionRight));
		const FVector InputAccelerationDirection = DeltaSpeedRight.GetUnsafeNormal2D();
		InputAcceleration += InputAccelerationScale * InputAccelerationDirection;
	}

	MoveAcceleration += InputAcceleration;
}

void UWpEntityMovementComponent::ApplyMoveForces()
{
	ensureAlways(bOnGround);

	if (MoveSpeed.IsNearlyZero())
	{
		return;
	}

	if (!GroundNormal.Equals(FVector::UpVector))
	{
		const float HorizontalGravityScale = -GetGravityZ() * FMath::Sqrt(1.0 - FMath::Square(GroundNormal.Dot(FVector::UpVector)));
		const FVector HorizontalGravityDeceleration = HorizontalGravityScale * GroundNormal.GetUnsafeNormal2D();
		MoveAcceleration += HorizontalGravityDeceleration;
	}

	if (bIsSlipping)
	{
		const FVector LateralSpeed = MoveSpeed.ProjectOnToNormal(UpdatedComponent->GetRightVector());
		const float MaxGroundKineticFriction = MaxGroundStaticFriction * KineticToStaticGroundFriction;
		const FVector LateralDeceleration = -MaxGroundKineticFriction * LateralSpeed.GetUnsafeNormal2D();
		MoveAcceleration += LateralDeceleration;
	}

	if (!FMath::IsNearlyZero(WheelRotationAngle))
	{
		const float TurningRadius = 1.0f / FMath::Sin(WheelRotationAngle);
		const FVector ForwardSpeed = MoveSpeed.ProjectOnToNormal(UpdatedComponent->GetForwardVector());
		const float CentripetalForceScale = ForwardSpeed.SizeSquared2D() / TurningRadius;
		const FVector CentripetalForce = CentripetalForceScale * UpdatedComponent->GetRightVector();
		MoveAcceleration += CentripetalForce;
	}
}

void UWpEntityMovementComponent::ApplyMoveFrictionLimit()
{
	ensureAlways(bOnGround);

	const float MaxGroundFriction = MaxGroundStaticFriction * (bIsSlipping ? KineticToStaticGroundFriction : 1.0f);
	const float GroundFriction = MaxGroundFriction * GroundNormal.Dot(FVector::UpVector);
	MoveAcceleration = MoveAcceleration.GetClampedToMaxSize2D(GroundFriction);
}

void UWpEntityMovementComponent::UpdateRotationAcceleration(const float DeltaTime, const FVector& InputVector)
{
	RotationAcceleration = 0.0f;

	if (!bOnGround)
	{
		return;
	}

	switch (MovementType)
	{
	case EWpEntityMovementType::Legs:
		ApplyRotationAccelerationLegs(DeltaTime);
		break;
	case EWpEntityMovementType::Tracks:
		ApplyRotationAccelerationTracks(DeltaTime, InputVector);
		break;
	case EWpEntityMovementType::Wheels:
		ApplyRotationAccelerationWheels(DeltaTime);
		break;
	default:
		break;
	}
}

void UWpEntityMovementComponent::ApplyRotationAccelerationLegs(const float DeltaTime)
{
	ensureAlways(bOnGround);

	const FVector CurrentDirection = UpdatedComponent->GetForwardVector();
	const FVector TargetDirection = MoveSpeed.GetSafeNormal2D();
	const float DeltaYaw = FMath::DegreesToRadians(FQuat::FindBetweenNormals(CurrentDirection, TargetDirection).Rotator().Yaw);

	if (FMath::IsNearlyZero(RotationSpeed) && FMath::IsNearlyZero(DeltaYaw, FMath::DegreesToRadians(0.5f)))
	{
		return;
	}

	const float BrakingYaw = FMath::Square(RotationSpeed) / (2.0f * MaxRotationBrakingDeceleration);
	const bool bShouldAccelerate = (FMath::Abs(DeltaYaw) > BrakingYaw) && (FMath::Abs(RotationSpeed) < MaxRotationSpeed);
	if (bShouldAccelerate)
	{
		RotationAcceleration += MaxRotationAcceleration * FMath::Sign(DeltaYaw);
	}
	else
	{
		RotationAcceleration += -MaxRotationBrakingDeceleration * FMath::Sign(RotationSpeed);

		const bool bRotationSpeedWillChangeSign = FMath::Sign(RotationSpeed) != FMath::Sign(CalcNewRotationSpeed(DeltaTime));
		if (bRotationSpeedWillChangeSign)
		{
			RotationSpeed = 0.0f;
			RotationAcceleration = 0.0f;
		}
	}
}

void UWpEntityMovementComponent::ApplyRotationAccelerationTracks(const float DeltaTime, const FVector& InputVector)
{
	ensureAlways(bOnGround);

	const float DesiredRotationSpeedScale = InputVector.Y * MaxRotationSpeed;
	const float DesiredRotationSpeedDirection = FMath::IsNearlyZero(InputVector.X) ? 1.0f : FMath::Sign(InputVector.X);
	const float DesiredRotationSpeed = DesiredRotationSpeedScale * DesiredRotationSpeedDirection;

	if (FMath::IsNearlyEqual(DesiredRotationSpeed, RotationSpeed))
	{
		return;
	}

	const float FrictionScale = bIsSlipping ? KineticToStaticGroundFriction : 1.0f;

	const bool bShouldAccelerate = FMath::IsNearlyZero(RotationSpeed)
		||
		(FMath::Sign(DesiredRotationSpeed) == FMath::Sign(RotationSpeed))
		&& (FMath::Abs(DesiredRotationSpeed) > FMath::Abs(RotationSpeed));
	if (bShouldAccelerate)
	{
		const float AccelerationScale = MaxRotationAcceleration * FrictionScale;
		const float AccelerationDirection = !FMath::IsNearlyZero(RotationSpeed) ? FMath::Sign(RotationSpeed) : FMath::Sign(DesiredRotationSpeed);
		RotationAcceleration += AccelerationScale * AccelerationDirection;
	}
	else
	{
		const float DecelerationScale = MaxRotationBrakingDeceleration * FrictionScale;
		RotationAcceleration += -DecelerationScale * FMath::Sign(RotationSpeed);

		const bool bIsBrakingToZeroRotationSpeed = FMath::IsNearlyZero(DesiredRotationSpeed);
		if (bIsBrakingToZeroRotationSpeed)
		{
			const bool bRotationSpeedWillChangeSign = FMath::Sign(RotationSpeed) != FMath::Sign(CalcNewRotationSpeed(DeltaTime));
			if (bRotationSpeedWillChangeSign)
			{
				RotationSpeed = 0.0f;
				RotationAcceleration = 0.0f;
			}
		}
	}
}

void UWpEntityMovementComponent::ApplyRotationAccelerationWheels(const float DeltaTime)
{
	ensureAlways(bOnGround);

	if (FMath::IsNearlyZero(RotationSpeed) && FMath::IsNearlyZero(RotationAcceleration)
		&& (MoveSpeed.IsNearlyZero() || FMath::IsNearlyZero(WheelRotationAngle)))
	{
		return;
	}

	float DesiredRotationSpeed = 0.0f;
	if (!FMath::IsNearlyZero(WheelRotationAngle))
	{
		const float TurningRadius = 1.0f / FMath::Sin(WheelRotationAngle);
		const FVector ForwardSpeed = MoveSpeed.ProjectOnToNormal(UpdatedComponent->GetForwardVector());
		const float ForwardSpeedScale = ForwardSpeed.Size2D() * FMath::Sign(ForwardSpeed.Dot(UpdatedComponent->GetForwardVector()));
		DesiredRotationSpeed = ForwardSpeedScale / TurningRadius;
	}
	const float DeltaRotationSpeed = DesiredRotationSpeed - RotationSpeed;
	const float DeltaRotationAcceleration = DeltaRotationSpeed / DeltaTime;
	const float FrictionScale = bIsSlipping ? KineticToStaticGroundFriction : 1.0f;
	RotationAcceleration += DeltaRotationAcceleration * FrictionScale;
}

FVector UWpEntityMovementComponent::CalcNewMoveSpeed(const float DeltaTime) const
{
	return MoveSpeed + MoveAcceleration * DeltaTime;
}

float UWpEntityMovementComponent::CalcNewRotationSpeed(const float DeltaTime) const
{
	return RotationSpeed + RotationAcceleration * DeltaTime;
}

void UWpEntityMovementComponent::Move(const float DeltaTime)
{
	if (MoveSpeed.IsNearlyZero() && FMath::IsNearlyZero(RotationSpeed) && bOnGround)
	{
		return;
	}

	const float DeltaAngle = RotationSpeed * DeltaTime;
	const FRotator OldRotation = UpdatedComponent->GetComponentRotation();
	const FRotator NewRotation = OldRotation.Vector().RotateAngleAxisRad(DeltaAngle, FVector::UpVector).Rotation();

	uint32 IterationIndex = 0;
	float RemainingTime = DeltaTime;

	while ((IterationIndex < MaxMoveIterationNum) && !FMath::IsNearlyZero(RemainingTime))
	{
		++IterationIndex;

		UpdateGround(RemainingTime);

		const FVector DeltaLocation = CalcDeltaLocation(RemainingTime, GroundNormal);
		if (DeltaLocation.IsNearlyZero() && UpdatedComponent->GetComponentRotation().Equals(NewRotation))
		{
			break;
		}

		FHitResult Hit;
		UpdatedComponent->MoveComponent(DeltaLocation, NewRotation, true, &Hit);

		const float SpentTime = Hit.Time * RemainingTime;
		RemainingTime -= SpentTime;

		if (Hit.bBlockingHit)
		{
			if (Hit.GetActor()->GetClass()->IsChildOf<AWpEntity>())
			{
				ProcessImpact(RemainingTime, Hit);
			}
			else
			{
				const float InclineAngleCos = Hit.ImpactNormal.Dot(FVector::UpVector);
				if (InclineAngleCos < MaxInclineAngleCos)
				{
					if (!StepUp(RemainingTime))
					{
						ProcessImpact(RemainingTime, Hit);
					}
				}
			}
		}
	}
}

FVector UWpEntityMovementComponent::CalcDeltaLocation(const float DeltaTime, const FVector& InGroundNormal) const
{
	if (MoveSpeed.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	if (InGroundNormal.Equals(FVector::UpVector))
	{
		return MoveSpeed * DeltaTime;
	}

	FVector DeltaLocation = FVector::VectorPlaneProject(MoveSpeed, InGroundNormal).GetUnsafeNormal() * MoveSpeed.Size() * DeltaTime;

	if (DeltaLocation.Z > 0.0)
	{
		const float InclineAngleCos = InGroundNormal.Dot(FVector::UpVector);
		if (InclineAngleCos < MaxInclineAngleCos)
		{
			DeltaLocation.Z = 0.0;
		}
	}

	return DeltaLocation;
}

void UWpEntityMovementComponent::UpdateGround(const float DeltaTime)
{
	const bool bWasOnGround = bOnGround;

	FHitResult Hit;
	const FVector StartLocation = UpdatedComponent->GetComponentLocation();
	const FVector EndLocation = StartLocation - (1.0f + MaxGroundDistance) * FVector::UpVector;
	Sweep(Hit, StartLocation, EndLocation);

	if (Hit.bBlockingHit)
	{
		if (UpdatedComponent->GetComponentLocation().Z - Hit.Location.Z > MaxGroundDistance)
		{
			bOnGround = false;
		}
		else
		{
			const float InclineAngleCos = Hit.ImpactNormal.Dot(FVector::UpVector);
			bOnGround = InclineAngleCos > MaxInclineAngleCos;
		}
	}
	else
	{
		bOnGround = false;
	}

	if (bOnGround)
	{
		GroundNormal = Hit.ImpactNormal;

		if (!bWasOnGround)
		{
			MoveSpeed *= FVector::ForwardVector + FVector::RightVector;
		}
	}
	else
	{
		GroundNormal = Hit.bBlockingHit ? Hit.ImpactNormal : FVector::UpVector;

		MoveSpeed += GetGravityZ() * DeltaTime * FVector::UpVector;
	}

	if (Hit.bBlockingHit)
	{
		if (bOnGround)
		{
			const FVector OnGroundLocation = Hit.Location + IdealGroundDistance * FVector::UpVector;
			const FVector DeltaLocation = OnGroundLocation - UpdatedComponent->GetComponentLocation();
			if (!DeltaLocation.IsNearlyZero())
			{
				UpdatedComponent->MoveComponent(DeltaLocation, UpdatedComponent->GetComponentRotation(), true);
			}
		}
		else if (bWasOnGround)
		{
			const FVector InclineOffsetLocation = Hit.Location + IdealGroundDistance * GroundNormal.GetSafeNormal2D();
			const FVector DeltaLocation = InclineOffsetLocation - UpdatedComponent->GetComponentLocation();
			UpdatedComponent->MoveComponent(DeltaLocation, UpdatedComponent->GetComponentRotation(), true);

			UpdateGround(0.0f);
		}
	}
}

void UWpEntityMovementComponent::ProcessImpact(const float DeltaTime, const FHitResult& Hit)
{
	ensureAlways(Hit.bBlockingHit);

	if (Hit.ImpactNormal.Equals(FVector::UpVector))
	{
		return;
	}

	AWpEntity* const OtherEntity = Cast<AWpEntity>(Hit.GetActor());
	UWpEntityMovementComponent* const OtherEntityMovementComponent = OtherEntity ? OtherEntity->GetEntityMovementComponent() : nullptr;
	const bool bImpactWithUnit = OtherEntityMovementComponent && OtherEntityMovementComponent->IsActive();

	if (bImpactWithUnit)
	{
		const FVector ThisMoveSpeed = MoveSpeed;
		const FVector OtherMoveSpeed = OtherEntityMovementComponent->MoveSpeed;

		ProcessEntityImpact(Hit, OtherEntityMovementComponent, OtherMoveSpeed);
		OtherEntityMovementComponent->ProcessEntityImpact(Hit, this, ThisMoveSpeed);
		OtherEntityMovementComponent->Move(DeltaTime);
	}
	else
	{
		ProcessWallImpact(Hit);
	}
}

void UWpEntityMovementComponent::ProcessWallImpact(const FHitResult& Hit)
{
	const FVector HorizontalImpactNormal = Hit.ImpactNormal.GetUnsafeNormal2D();
	const FVector HorizontalMoveSpeed = MoveSpeed * (FVector::ForwardVector + FVector::RightVector);
	const FVector SpeedInto = HorizontalMoveSpeed.ProjectOnToNormal(HorizontalImpactNormal);
	const FVector SpeedAlong = HorizontalMoveSpeed - SpeedInto;

	const FVector NewSpeedInto = WallImpactElasticity * -SpeedInto;
	const FVector DeltaSpeed = NewSpeedInto - SpeedInto;
	MoveSpeed += DeltaSpeed;

	if (MovementType != EWpEntityMovementType::Legs)
	{
		const FVector ImpactRadiusVector = Hit.ImpactPoint - Hit.Location;
		const float ImpactRotationSpeedScale = SpeedAlong.Size2D() / ImpactRadiusVector.Size2D();
		const float ImpactRotationSpeedDirection =
			FMath::Sign(ImpactRadiusVector.Dot(UpdatedComponent->GetRightVector()))
			* FMath::Sign(HorizontalMoveSpeed.Dot(UpdatedComponent->GetForwardVector()));
		const float ImpactRotationSpeed = ImpactRotationSpeedScale * ImpactRotationSpeedDirection;
		RotationSpeed = FMath::Clamp(ImpactRotationSpeed, -MaxRotationSpeed, MaxRotationSpeed);
	}
}

void UWpEntityMovementComponent::ProcessEntityImpact(const FHitResult& Hit, const UWpEntityMovementComponent* const OtherMovementComponent, const FVector& OtherMoveSpeed)
{
	ensureAlways(OtherMovementComponent);

	const FVector HorizontalImpactNormal = Hit.ImpactNormal.GetUnsafeNormal2D();
	const FVector HorizontalMoveSpeed = MoveSpeed * (FVector::ForwardVector + FVector::RightVector);
	const FVector SpeedInto = HorizontalMoveSpeed.ProjectOnToNormal(HorizontalImpactNormal);
	const FVector SpeedAlong = HorizontalMoveSpeed - SpeedInto;

	const FVector OtherHorizontalMoveSpeed = OtherMoveSpeed * (FVector::ForwardVector + FVector::RightVector);
	const FVector OtherSpeedInto = OtherHorizontalMoveSpeed.ProjectOnToNormal(HorizontalImpactNormal);
	const FVector OtherSpeedAlong = OtherHorizontalMoveSpeed - OtherSpeedInto;
	const float& OtherMass = OtherMovementComponent->Mass;

	const FVector NewSpeedInto = EntityImpactElasticity
		* ((Mass - OtherMass) * SpeedInto
			+ 2.0 * OtherMass * OtherSpeedInto)
		/ (Mass + OtherMass);
	const FVector DeltaSpeed = NewSpeedInto - SpeedInto;
	MoveSpeed += DeltaSpeed;

	if (MovementType != EWpEntityMovementType::Legs)
	{
		const FVector ImpactRadiusVector = Hit.ImpactPoint - UpdatedComponent->GetComponentLocation();
		const float ImpactRotationSpeedScale = (SpeedAlong - OtherSpeedAlong).Size2D() / ImpactRadiusVector.Size2D();
		const float ImpactRotationSpeedDirection =
			FMath::Sign(ImpactRadiusVector.Dot(UpdatedComponent->GetRightVector()))
			* FMath::Sign(HorizontalMoveSpeed.Dot(UpdatedComponent->GetForwardVector()));
		const float ImpactRotationSpeed = ImpactRotationSpeedScale * ImpactRotationSpeedDirection;
		RotationSpeed = FMath::Clamp(ImpactRotationSpeed, -MaxRotationSpeed, MaxRotationSpeed);
	}
}

bool UWpEntityMovementComponent::StepUp(const float DeltaTime)
{
	FHitResult Hit;

	const FVector LocationBeforeStepUp = UpdatedComponent->GetComponentLocation();
	const FVector StepUpDeltaLocation = MaxStepUpHeight * FVector::UpVector;
	FVector LocationAfterStepUp = LocationBeforeStepUp + StepUpDeltaLocation;
	Sweep(Hit, LocationBeforeStepUp, LocationAfterStepUp);
	if (Hit.bBlockingHit)
	{
		LocationAfterStepUp = Hit.Location;
	}

	const FVector LocationBeforeStepForward = LocationAfterStepUp;
	const FVector StepForwardDeltaLocation = CalcDeltaLocation(DeltaTime, FVector::UpVector);
	FVector LocationAfterStepForward = LocationBeforeStepForward + StepForwardDeltaLocation;
	Sweep(Hit, LocationBeforeStepForward, LocationAfterStepForward);
	if (Hit.bBlockingHit)
	{
		const float InclineAngleCos = Hit.ImpactNormal.Dot(FVector::UpVector);
		if (InclineAngleCos < MaxInclineAngleCos)
		{
			return false;
		}

		LocationAfterStepForward = Hit.Location;
	}

	const FVector LocationBeforeStepDown = LocationAfterStepForward;
	const FVector StepDownDeltaLocation = -(1.0f + MaxGroundDistance + MaxStepUpHeight) * FVector::UpVector;
	FVector LocationAfterStepDown = LocationBeforeStepDown + StepDownDeltaLocation;
	Sweep(Hit, LocationBeforeStepDown, LocationAfterStepDown);
	if (Hit.bBlockingHit)
	{
		const float InclineAngleCos = Hit.ImpactNormal.Dot(FVector::UpVector);
		if (InclineAngleCos < MaxInclineAngleCos)
		{
			return false;
		}

		LocationAfterStepDown = Hit.Location;
	}
	else
	{
		LocationAfterStepDown = LocationBeforeStepDown - (MaxStepUpHeight + IdealGroundDistance) * FVector::UpVector;
	}

	LocationAfterStepDown += IdealGroundDistance * FVector::UpVector;

	const FVector OldLocation = UpdatedComponent->GetComponentLocation();

	const FRotator Rotation = UpdatedComponent->GetComponentRotation();
	UpdatedComponent->MoveComponent(LocationAfterStepUp - UpdatedComponent->GetComponentLocation(), Rotation, true);
	UpdatedComponent->MoveComponent(LocationAfterStepForward - UpdatedComponent->GetComponentLocation(), Rotation, true);
	UpdatedComponent->MoveComponent(LocationAfterStepDown - UpdatedComponent->GetComponentLocation(), Rotation, true);

	const FVector NewLocation = UpdatedComponent->GetComponentLocation();
	const FVector DeltaLocation = NewLocation - OldLocation;
	AddInterpolatedComponentOffset(DeltaLocation, FRotator::ZeroRotator);

	return true;
}

void UWpEntityMovementComponent::Sweep(FHitResult& OutHit, const FVector& StartLocation, const FVector& EndLocation) const
{
	const FQuat Rotation = UpdatedComponent->GetComponentQuat();
	const FName CollisionProfile = Cast<UCapsuleComponent>(UpdatedComponent)->GetCollisionProfileName();
	float Radius, HalfHeight;
	Cast<UCapsuleComponent>(UpdatedComponent)->GetScaledCapsuleSize(Radius, HalfHeight);
	const FCollisionShape CollisionShape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.AddIgnoredActor(GetOwner());

	GetWorld()->SweepSingleByProfile(OutHit, StartLocation, EndLocation, Rotation, CollisionProfile, CollisionShape, CollisionQueryParams);
}

void UWpEntityMovementComponent::CacheInitialInterpolatedComponentOffset()
{
	InitialInterpolatedComponentTransform = InterpolatedComponent->GetRelativeTransform();

	TargetInterpolatedComponentLocation = InitialInterpolatedComponentTransform.GetLocation();
	TargetInterpolatedComponentRotation = InitialInterpolatedComponentTransform.GetRotation().Rotator();

	DeltaInterpolatedComponentLocation = FVector::ZeroVector;
	DeltaInterpolatedComponentRotation = FRotator::ZeroRotator;
}

void UWpEntityMovementComponent::UpdateTargetInterpolatedComponentOffset()
{
	if (MovementType == EWpEntityMovementType::Legs)
	{
		return;
	}

	const FVector OldTargetInterpolatedComponentLocation = TargetInterpolatedComponentLocation;
	const FRotator OldTargetInterpolatedComponentRotation = TargetInterpolatedComponentRotation;

	const FVector ActualGroundNormal = bOnGround ? GroundNormal : FVector::UpVector;
	const FVector LocalGroundNormal = InitialInterpolatedComponentTransform.InverseTransformVectorNoScale(
		UpdatedComponent->GetComponentTransform().InverseTransformVectorNoScale(
			ActualGroundNormal
		));
	const FRotator LocalGroundNormalRotation = FQuat::FindBetweenNormals(FVector::UpVector, LocalGroundNormal).Rotator();
	TargetInterpolatedComponentLocation = LocalGroundNormalRotation.RotateVector(InitialInterpolatedComponentTransform.GetLocation());
	TargetInterpolatedComponentRotation = InitialInterpolatedComponentTransform.GetRotation().Rotator() + LocalGroundNormalRotation;

	DeltaInterpolatedComponentLocation += TargetInterpolatedComponentLocation - OldTargetInterpolatedComponentLocation;
	DeltaInterpolatedComponentRotation += TargetInterpolatedComponentRotation - OldTargetInterpolatedComponentRotation;
}

void UWpEntityMovementComponent::AddInterpolatedComponentOffset(const FVector& DeltaLocation, const FRotator& DeltaRotation)
{
	DeltaInterpolatedComponentLocation += DeltaLocation;
	DeltaInterpolatedComponentRotation += DeltaRotation;

	const FVector CurrentLocation = TargetInterpolatedComponentLocation - DeltaInterpolatedComponentLocation;
	const FRotator CurrentRotation = TargetInterpolatedComponentRotation - DeltaInterpolatedComponentRotation;
	InterpolatedComponent->SetRelativeLocation(CurrentLocation, false);
	InterpolatedComponent->SetRelativeRotation(CurrentRotation, false);
}

void UWpEntityMovementComponent::UpdateInterpolatedComponentOffset(const float DeltaTime)
{
	if (DeltaInterpolatedComponentLocation.IsNearlyZero() && DeltaInterpolatedComponentRotation.IsNearlyZero())
	{
		return;
	}

	if (DeltaTime < InterpolatedComponentSmoothingTime)
	{
		const float Scale = 1.0f - DeltaTime / InterpolatedComponentSmoothingTime;
		DeltaInterpolatedComponentLocation *= Scale;
		DeltaInterpolatedComponentRotation *= Scale;

		if (DeltaInterpolatedComponentLocation.SizeSquared() < InterpolatedComponentSnapDeltaLocation2)
		{
			DeltaInterpolatedComponentLocation = FVector::ZeroVector;
		}
		if (DeltaInterpolatedComponentRotation.IsNearlyZero(InterpolatedComponentSnapDeltaRotation))
		{
			DeltaInterpolatedComponentRotation = FRotator::ZeroRotator;
		}
	}
	else
	{
		DeltaInterpolatedComponentLocation = FVector::ZeroVector;
		DeltaInterpolatedComponentRotation = FRotator::ZeroRotator;
	}

	const FVector CurrentLocation = TargetInterpolatedComponentLocation - DeltaInterpolatedComponentLocation;
	const FRotator CurrentRotation = TargetInterpolatedComponentRotation - DeltaInterpolatedComponentRotation;
	InterpolatedComponent->SetRelativeLocation(CurrentLocation, false);
	InterpolatedComponent->SetRelativeRotation(CurrentRotation, false);
}

void UWpEntityMovementComponent::UpdateNavVelocity()
{
	const FVector GroundVelocity = FVector::VectorPlaneProject(MoveSpeed, GroundNormal).GetSafeNormal() * MoveSpeed.Size2D();
	const FVector VerticalVelocity = MoveSpeed * FVector::UpVector;
	Velocity = GroundVelocity + VerticalVelocity;
}
