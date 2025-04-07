#include "WpEntityMovementComponent.h"

#include "Warpunk/Game/Entity/WpEntity.h"
#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Entity/WpPooledEntityData.h"
#include "Warpunk/Game/Player/WpGamePlayerController.h"

#include "GameFramework/GameStateBase.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Net/UnrealNetwork.h"

/*
 * v(t) = V * (1 - e ^ (-C / T * t)) / (1 - e ^ (-C))
 * a(t) = V * C * e ^ (-C / T * t) / T / (1 - e ^ (-C))
 * a(v) = V * C / T / (1 - e ^ (-C)) - C / T * v
 * phi(C) = asin(V * C / g / T / (1 - e ^ (-C)))
 * t - time, v - velocity, a - acceleration, phi - max ground incline angle
 * V - max velocity, T - max velocity acceleration time, C - torque coefficient, G - gravity acceleration
 */

const float UWpEntityMovementComponent::MinTickTime = 1.0e-6f;
const uint32 UWpEntityMovementComponent::MaxMoveIterationNum = 8;

const float UWpEntityMovementComponent::CollisionPullBackDistance = 1.0e-2f;
const uint32 UWpEntityMovementComponent::MaxResolvePenetrationIterationNum = 4; // should be even

const float UWpEntityMovementComponent::IdealGroundDistance = 1.0f;
const float UWpEntityMovementComponent::MaxGroundDistance = IdealGroundDistance + 1.0f;

const float UWpEntityMovementComponent::WallImpactElasticity = 0.2f;
const float UWpEntityMovementComponent::EntityImpactElasticity = 0.5f;
const float UWpEntityMovementComponent::EntityImpactReturnDistance = 1.0e-2f;
const float UWpEntityMovementComponent::EntityImpactLateralSpeedReduction = 0.1f;

const float UWpEntityMovementComponent::KineticToStaticGroundFriction = 0.8f;
const float UWpEntityMovementComponent::MinSlippingLateralSpeed = 5.0f;
const float UWpEntityMovementComponent::MinSlippingMagicValueScale = 0.4f;

const float UWpEntityMovementComponent::MinSpeedForLegsRotationScale = 0.1f;

const float UWpEntityMovementComponent::InterpolatedComponentSmoothingTime = 0.1f;
const float UWpEntityMovementComponent::InterpolatedComponentSnapDeltaLocation2 = FMath::Square(1.0f);
const float UWpEntityMovementComponent::InterpolatedComponentSnapDeltaRotation = FMath::DegreesToRadians(1.0f);
const float UWpEntityMovementComponent::InterpolatedComponentNoSmoothDistance2 = FMath::Square(100.0f);

UWpEntityMovementComponent::UWpEntityMovementComponent()
{
	bAutoActivate = false;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetIsReplicatedByDefault(true);

	bAutoUpdateTickRegistration = false;

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

	TickMovement(DeltaTime);
}

void UWpEntityMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DISABLE_REPLICATED_PRIVATE_PROPERTY(UActorComponent, bIsActive);

	FDoRepLifetimeParams DoRepLifetimeParams;
	DoRepLifetimeParams.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(UWpEntityMovementComponent, PredictionData, DoRepLifetimeParams);
}

float UWpEntityMovementComponent::GetMaxSpeed() const
{
	return MaxMoveSpeed;
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
	if (!bIsAcquired)
	{
		ensureAlways(GetOwner()->HasAuthority());
	}

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

			CollisionQueryParams = FCollisionQueryParams{NAME_None, false, GetOwner()};

			if (GetOwner()->HasAuthority())
			{
				MoveSpeed = FVector::ZeroVector;
				MoveAcceleration = FVector::ZeroVector;
				RotationSpeed = 0.0f;
				RotationAcceleration = 0.0f;
				WheelRotationAngle = 0.0f;
				bOnGround = false;
				GroundNormal = FVector::UpVector;
				bIsSlipping = false;
				SetIsFirstPersonControlled(false);
				UpdateNavVelocity();
				UpdatePredictionData(FVector::ZeroVector);

				FNavAgentProperties& NavAgentProperties = GetNavAgentPropertiesRef();
				NavAgentProperties.AgentStepHeight = MaxStepUpHeight;
			}
		}
		else
		{
			bIsAcquired = false;
		}
	}

	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	SetActive(bIsAcquired);
}

void UWpEntityMovementComponent::OnPooledObjectVisibilityChanged(const bool bIsVisible)
{
	if (!GetOwner()->HasAuthority())
	{
		SetActive(bIsVisible && (GetOwner<AWpEntity>()->GetEntityType() == EWpEntityType::Unit));
	}
}

void UWpEntityMovementComponent::SetCollisionComponent(UPrimitiveComponent* const InCollisionComponent)
{
	CollisionComponent = InCollisionComponent;

	CollisionShape = CollisionComponent->GetCollisionShape();

	if (GetOwner()->HasAuthority())
	{
		FNavAgentProperties& NavAgentProperties = GetNavAgentPropertiesRef();
		const FVector Extent = CollisionShape.GetExtent();
		NavAgentProperties.AgentRadius = FMath::Max(Extent.X, Extent.Y);
		NavAgentProperties.AgentHeight = Extent.Z;
	}
}

void UWpEntityMovementComponent::SetInterpolatedComponent(USceneComponent* const InInterpolatedComponent)
{
	InterpolatedComponent = InInterpolatedComponent;
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
	if (bIsFirstPersonControlled != bInIsFirstPersonControlled)
	{
		bIsFirstPersonControlled = bInIsFirstPersonControlled;

		if (bIsFirstPersonControlled && !GetOwner()->HasAuthority())
		{
			LastInputVector = FVector::ZeroVector;
		}
	}
}

void UWpEntityMovementComponent::CacheInitialInterpolatedComponentOffset()
{
	InitialInterpolatedComponentTransform = InterpolatedComponent->GetRelativeTransform();

	TargetInterpolatedComponentLocation = InitialInterpolatedComponentTransform.GetLocation();
	TargetInterpolatedComponentRotation = InitialInterpolatedComponentTransform.GetRotation().Rotator();

	DeltaInterpolatedComponentLocation = FVector::ZeroVector;
	DeltaInterpolatedComponentRotation = FRotator::ZeroRotator;
}

void UWpEntityMovementComponent::TickMovement(const float DeltaTime)
{
	UpdateInterpolatedComponentOffset(DeltaTime);

	const FVector InputVector = GetInputVector();

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

	if (GetOwner()->HasAuthority())
	{
		UpdateNavVelocity();

		UpdatePredictionData(InputVector);
	}
}

FVector UWpEntityMovementComponent::GetInputVector()
{
	const bool bConsume = bIsFirstPersonControlled
		                      ? (GetController() && GetController()->IsLocalPlayerController())
		                      : GetOwner()->HasAuthority();
	FVector InputVector = bConsume ? ConsumeInputVector() : GetPendingInputVector();

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

	if (bIsFirstPersonControlled && !GetOwner()->HasAuthority() && !InputVector.Equals(LastInputVector))
	{
		if (AWpGamePlayerController* const PlayerController = GetOwner<APawn>()->GetController<AWpGamePlayerController>())
		{
			PlayerController->ServerSetCommanderMovementInput(InputVector.X, InputVector.Y);

			LastInputVector = InputVector;
		}
	}

	return InputVector;
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
		MoveSpeed += GetGravityZ() * DeltaTime * FVector::UpVector;

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
	const FVector TargetDirection = MoveSpeed.IsNearlyZero(MinSpeedForLegsRotationScale * MaxMoveSpeed)
		                                ? CurrentDirection
		                                : MoveSpeed.GetSafeNormal2D();
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

	while ((RemainingTime > MinTickTime) && (IterationIndex < MaxMoveIterationNum))
	{
		++IterationIndex;

		if (!ResolvePenetration())
		{
			continue;
		}

		UpdateGround();

		const FVector DeltaLocation = CalcDeltaLocation(RemainingTime, GroundNormal);
		if (DeltaLocation.IsNearlyZero() && UpdatedComponent->GetComponentRotation().Equals(NewRotation))
		{
			break;
		}

		FHitResult Hit;
		MoveWithCollision(DeltaLocation, NewRotation, &Hit);

		const float SpentTime = Hit.Time * RemainingTime;
		RemainingTime -= SpentTime;

		if (Hit.bBlockingHit)
		{
			if (ProcessEntityImpact(Hit))
			{
				break;
			}

			const float InclineAngleCos = Hit.ImpactNormal.Dot(FVector::UpVector);
			if (InclineAngleCos < MaxInclineAngleCos)
			{
				float StepUpSpentTime;
				if (StepUp(RemainingTime, StepUpSpentTime))
				{
					RemainingTime -= StepUpSpentTime;
				}
				else
				{
					ProcessWallImpact(Hit);
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

bool UWpEntityMovementComponent::ResolvePenetration()
{
	FHitResult Hit;
	FVector AdjustmentVector = FVector::ZeroVector;

	const FQuat Rotation = CollisionComponent->GetComponentQuat();

	for (uint32 IterationIndex = 0; IterationIndex < MaxResolvePenetrationIterationNum; ++IterationIndex)
	{
		const FVector Location = CollisionComponent->GetComponentLocation();
		Sweep(Hit, Location, Location, Rotation);

		if (!Hit.bBlockingHit || !Hit.bStartPenetrating)
		{
			return true;
		}

		AdjustmentVector += (Hit.PenetrationDepth + CollisionPullBackDistance) * Hit.ImpactNormal;
		UpdatedComponent->MoveComponent(AdjustmentVector, UpdatedComponent->GetComponentQuat(), false);
	}

	return false;
}

void UWpEntityMovementComponent::UpdateGround()
{
	const bool bWasOnGround = bOnGround;

	FHitResult Hit;
	const FVector StartLocation = CollisionComponent->GetComponentLocation();
	const FVector EndLocation = StartLocation - (1.0f + MaxGroundDistance) * FVector::UpVector;
	const FQuat Rotation = CollisionComponent->GetComponentQuat();
	Sweep(Hit, StartLocation, EndLocation, Rotation);

	bool bShouldLineTrace = false;
	bool bSetNormalWhenNotOnGround = false;

	if (Hit.bBlockingHit)
	{
		if (Hit.Distance < MaxGroundDistance)
		{
			const float InclineAngleCos = Hit.ImpactNormal.Dot(FVector::UpVector);
			bOnGround = InclineAngleCos > MaxInclineAngleCos;

			bShouldLineTrace = !bOnGround;
			bSetNormalWhenNotOnGround = true;
		}
		else
		{
			bOnGround = false;
		}
	}
	else
	{
		bOnGround = false;
	}

	if (bShouldLineTrace)
	{
		FHitResult LineTraceHit;
		LineTraceGround(LineTraceHit, StartLocation, EndLocation);

		if (LineTraceHit.bBlockingHit && (LineTraceHit.Distance < MaxGroundDistance))
		{
			const float InclineAngleCos = LineTraceHit.ImpactNormal.Dot(FVector::UpVector);
			bOnGround = InclineAngleCos > MaxInclineAngleCos;

			if (bOnGround)
			{
				Hit = LineTraceHit;
			}
		}
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
		GroundNormal = bSetNormalWhenNotOnGround ? Hit.ImpactNormal : FVector::UpVector;
	}

	if (Hit.bBlockingHit && bOnGround)
	{
		const FVector OnGroundLocation = Hit.Location + IdealGroundDistance * FVector::UpVector;
		const FVector DeltaLocation = OnGroundLocation - CollisionComponent->GetComponentLocation();
		if (!DeltaLocation.IsNearlyZero())
		{
			MoveWithCollision(DeltaLocation, UpdatedComponent->GetComponentRotation());
		}
	}
}

void UWpEntityMovementComponent::ProcessWallImpact(const FHitResult& Hit)
{
	ensureAlways(Hit.bBlockingHit);
	ensureAlways(!Hit.Normal.Equals(FVector::UpVector) && !Hit.Normal.Equals(FVector::DownVector));

	const FVector HorizontalNormal = Hit.Normal.GetUnsafeNormal2D();
	const FVector HorizontalMoveSpeed = MoveSpeed * (FVector::ForwardVector + FVector::RightVector);
	const FVector SpeedInto = HorizontalMoveSpeed.ProjectOnToNormal(HorizontalNormal);
	const FVector SpeedAlong = HorizontalMoveSpeed - SpeedInto;

	const FVector NewSpeedInto = WallImpactElasticity * -SpeedInto;
	MoveSpeed = NewSpeedInto + SpeedAlong;

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

bool UWpEntityMovementComponent::ProcessEntityImpact(const FHitResult& Hit)
{
	ensureAlways(Hit.bBlockingHit);

	if (Hit.Normal.Equals(FVector::UpVector) || Hit.Normal.Equals(FVector::DownVector))
	{
		return false;
	}

	AWpEntity* const OtherEntity = Cast<AWpEntity>(Hit.GetActor());
	if (!OtherEntity)
	{
		return false;
	}

	UWpEntityMovementComponent* const OtherMovementComponent = OtherEntity->GetEntityMovementComponent();
	ensureAlways(this != OtherMovementComponent);
	if (OtherMovementComponent->IsActive())
	{
		const FVector ThisMoveSpeed = MoveSpeed;
		const FVector OtherMoveSpeed = OtherMovementComponent->MoveSpeed;

		ReceiveEntityImpact(Hit, OtherMovementComponent, OtherMoveSpeed);
		if (OtherEntity->HasAuthority())
		{
			OtherMovementComponent->ReceiveEntityImpact(Hit, this, ThisMoveSpeed);
		}

		const FVector EntityImpactReturnDeltaLocation = EntityImpactReturnDistance * Hit.Normal;
		MoveWithCollision(EntityImpactReturnDeltaLocation, UpdatedComponent->GetComponentRotation());

		return true;
	}

	return false;
}

void UWpEntityMovementComponent::ReceiveEntityImpact(const FHitResult& Hit, const UWpEntityMovementComponent* const OtherMovementComponent, const FVector& OtherMoveSpeed)
{
	ensureAlways(Hit.bBlockingHit);
	ensureAlways(!Hit.Normal.Equals(FVector::UpVector) && !Hit.Normal.Equals(FVector::DownVector));
	ensureAlways(OtherMovementComponent);
	ensureAlways(this != OtherMovementComponent);

	const FVector HorizontalNormal = Hit.Normal.GetUnsafeNormal2D();
	const FVector HorizontalMoveSpeed = MoveSpeed * (FVector::ForwardVector + FVector::RightVector);
	const FVector SpeedInto = HorizontalMoveSpeed.ProjectOnToNormal(HorizontalNormal);
	const FVector SpeedAlong = HorizontalMoveSpeed - SpeedInto;

	const FVector OtherHorizontalMoveSpeed = OtherMoveSpeed * (FVector::ForwardVector + FVector::RightVector);
	const FVector OtherSpeedInto = OtherHorizontalMoveSpeed.ProjectOnToNormal(HorizontalNormal);
	const FVector OtherSpeedAlong = OtherHorizontalMoveSpeed - OtherSpeedInto;
	const float& OtherMass = OtherMovementComponent->Mass;

	const FVector NewSpeedInto = EntityImpactElasticity
		* ((Mass - OtherMass) * SpeedInto
			+ 2.0 * OtherMass * OtherSpeedInto)
		/ (Mass + OtherMass);
	const FVector NewSpeedAlong = SpeedAlong * (1.0f - EntityImpactLateralSpeedReduction);
	MoveSpeed = NewSpeedInto + NewSpeedAlong;

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

bool UWpEntityMovementComponent::StepUp(const float DeltaTime, float& OutSpentTime)
{
	FHitResult Hit;

	const FQuat Rotation = CollisionComponent->GetComponentQuat();

	const FVector LocationBeforeStepUp = CollisionComponent->GetComponentLocation();
	const FVector StepUpDeltaLocation = MaxStepUpHeight * FVector::UpVector;
	FVector LocationAfterStepUp = LocationBeforeStepUp + StepUpDeltaLocation;
	Sweep(Hit, LocationBeforeStepUp, LocationAfterStepUp, Rotation);
	if (Hit.bBlockingHit)
	{
		LocationAfterStepUp = Hit.Location;
	}

	const FVector LocationBeforeStepForward = LocationAfterStepUp;
	const FVector StepForwardDeltaLocation = CalcDeltaLocation(DeltaTime, FVector::UpVector);
	FVector LocationAfterStepForward = LocationBeforeStepForward + StepForwardDeltaLocation;
	Sweep(Hit, LocationBeforeStepForward, LocationAfterStepForward, Rotation);
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
	Sweep(Hit, LocationBeforeStepDown, LocationAfterStepDown, Rotation);
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

	MoveWithCollision(LocationAfterStepUp - CollisionComponent->GetComponentLocation(), UpdatedComponent->GetComponentRotation());
	MoveWithCollision(LocationAfterStepForward - CollisionComponent->GetComponentLocation(), UpdatedComponent->GetComponentRotation(), &Hit);
	MoveWithCollision(LocationAfterStepDown - CollisionComponent->GetComponentLocation(), UpdatedComponent->GetComponentRotation());

	const FVector NewLocation = UpdatedComponent->GetComponentLocation();
	const FVector DeltaLocation = NewLocation - OldLocation;
	AddInterpolatedComponentOffset(DeltaLocation, FRotator::ZeroRotator);

	OutSpentTime = Hit.Time * DeltaTime;
	return true;
}

void UWpEntityMovementComponent::MoveWithCollision(const FVector& DeltaLocation, const FRotator& NewRotation, FHitResult* const OutHit)
{
	FHitResult Hit;

	const FVector PullBackCollisionVector = -CollisionPullBackDistance * DeltaLocation.GetSafeNormal();

	const FVector StartLocation = CollisionComponent->GetComponentLocation();
	const FVector EndLocation = StartLocation + DeltaLocation - PullBackCollisionVector;
	const FQuat Rotation =
		(NewRotation.Quaternion() * UpdatedComponent->GetComponentQuat().Inverse())
		* CollisionComponent->GetComponentQuat();
	Sweep(Hit, StartLocation, EndLocation, Rotation);

	FVector ActualDeltaLocation;
	if (Hit.bBlockingHit)
	{
		Hit.Location += PullBackCollisionVector.GetClampedToMaxSize(Hit.Distance);
		Hit.Distance = (Hit.Location - StartLocation).Size();
		Hit.Time = DeltaLocation.IsNearlyZero() ? 0.0f : (Hit.Distance / DeltaLocation.Size());

		ActualDeltaLocation = Hit.Location - StartLocation;
	}
	else
	{
		ActualDeltaLocation = DeltaLocation;
	}

	UpdatedComponent->MoveComponent(ActualDeltaLocation, NewRotation, false);

	if (OutHit)
	{
		*OutHit = Hit;
	}
}

void UWpEntityMovementComponent::Sweep(FHitResult& OutHit, const FVector& StartLocation, const FVector& EndLocation, const FQuat& Rotation) const
{
	GetWorld()->SweepSingleByProfile(OutHit, StartLocation, EndLocation, Rotation,
	                                 AWpEntity::CollisionComponentCollisionProfileName, CollisionShape, CollisionQueryParams);
}

void UWpEntityMovementComponent::LineTraceGround(FHitResult& OutHit, const FVector& StartLocation, const FVector& EndLocation) const
{
	const FVector RelativeLocation = -CollisionShape.GetExtent().Z * FVector::UpVector;
	const FVector ActualStartLocation = StartLocation + RelativeLocation;
	const FVector ActualEndLocation = EndLocation + RelativeLocation;

	GetWorld()->LineTraceSingleByProfile(OutHit, ActualStartLocation, ActualEndLocation,
	                                     AWpEntity::CollisionComponentCollisionProfileName, CollisionQueryParams);

	if (OutHit.bBlockingHit)
	{
		OutHit.Location -= RelativeLocation;
	}
}

void UWpEntityMovementComponent::AddInterpolatedComponentOffset(const FVector& DeltaLocation, const FRotator& DeltaRotation)
{
	const FVector LocalDeltaLocation = InitialInterpolatedComponentTransform.InverseTransformVectorNoScale(
		UpdatedComponent->GetComponentTransform().InverseTransformVectorNoScale(
			DeltaLocation
		));

	DeltaInterpolatedComponentLocation += LocalDeltaLocation;
	DeltaInterpolatedComponentRotation += DeltaRotation;
	DeltaInterpolatedComponentRotation.Normalize();

	const FVector CurrentLocation = TargetInterpolatedComponentLocation - DeltaInterpolatedComponentLocation;
	const FRotator CurrentRotation = TargetInterpolatedComponentRotation - DeltaInterpolatedComponentRotation;
	InterpolatedComponent->SetRelativeLocation(CurrentLocation, false);
	InterpolatedComponent->SetRelativeRotation(CurrentRotation, false);
}

void UWpEntityMovementComponent::UpdateTargetInterpolatedComponentOffset()
{
	if (!bOnGround)
	{
		return;
	}

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
	TargetInterpolatedComponentRotation = InitialInterpolatedComponentTransform.GetRotation().Rotator() + LocalGroundNormalRotation;
	TargetInterpolatedComponentRotation.Normalize();
	TargetInterpolatedComponentLocation = TargetInterpolatedComponentRotation.RotateVector(InitialInterpolatedComponentTransform.GetLocation());

	DeltaInterpolatedComponentLocation += TargetInterpolatedComponentLocation - OldTargetInterpolatedComponentLocation;
	DeltaInterpolatedComponentRotation += TargetInterpolatedComponentRotation - OldTargetInterpolatedComponentRotation;
	DeltaInterpolatedComponentRotation.Normalize();
}

void UWpEntityMovementComponent::UpdateInterpolatedComponentOffset(const float DeltaTime)
{
	if (DeltaInterpolatedComponentLocation.IsNearlyZero() && DeltaInterpolatedComponentRotation.IsNearlyZero())
	{
		return;
	}

	if ((DeltaTime < InterpolatedComponentSmoothingTime) && (DeltaInterpolatedComponentLocation.SizeSquared() < InterpolatedComponentNoSmoothDistance2))
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
	ensureAlways(GetOwner()->HasAuthority());

	const FVector GroundVelocity = FVector::VectorPlaneProject(MoveSpeed, GroundNormal).GetSafeNormal() * MoveSpeed.Size2D();
	const FVector VerticalVelocity = MoveSpeed * FVector::UpVector;
	Velocity = GroundVelocity + VerticalVelocity;
}

void UWpEntityMovementComponent::UpdatePredictionData(const FVector& InputVector)
{
	ensureAlways(GetOwner()->HasAuthority());

	PredictionData.ServerTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
	PredictionData.UpdatedComponentLocation = UpdatedComponent->GetComponentLocation();
	PredictionData.UpdatedComponentRotation = UpdatedComponent->GetComponentRotation();
	PredictionData.InputVector = InputVector;
	PredictionData.MoveSpeed = MoveSpeed;
	PredictionData.RotationSpeed = RotationSpeed;
	PredictionData.WheelRotationAngle = WheelRotationAngle;
	PredictionData.TargetInterpolatedComponentLocation = TargetInterpolatedComponentLocation;
	PredictionData.TargetInterpolatedComponentRotation = TargetInterpolatedComponentRotation;

	MARK_PROPERTY_DIRTY_FROM_NAME(UWpEntityMovementComponent, PredictionData, this);
}

void UWpEntityMovementComponent::OnRep_PredictionData()
{
	ensureAlways(!GetOwner()->HasAuthority());

	const FVector DeltaLocation = PredictionData.UpdatedComponentLocation - UpdatedComponent->GetComponentLocation();
	const FRotator DeltaRotation = (PredictionData.UpdatedComponentRotation - UpdatedComponent->GetComponentRotation()).GetNormalized();
	UpdatedComponent->MoveComponent(DeltaLocation, PredictionData.UpdatedComponentRotation, false);
	AddInterpolatedComponentOffset(DeltaLocation, DeltaRotation);

	if (!bIsFirstPersonControlled)
	{
		ConsumeInputVector();
		AddInputVector(PredictionData.InputVector);
	}

	MoveSpeed = PredictionData.MoveSpeed;
	RotationSpeed = PredictionData.RotationSpeed;
	WheelRotationAngle = PredictionData.WheelRotationAngle;

	const FVector OldTargetInterpolatedComponentLocation = TargetInterpolatedComponentLocation;
	const FRotator OldTargetInterpolatedComponentRotation = TargetInterpolatedComponentRotation;
	TargetInterpolatedComponentLocation = PredictionData.TargetInterpolatedComponentLocation;
	TargetInterpolatedComponentRotation = PredictionData.TargetInterpolatedComponentRotation;
	DeltaInterpolatedComponentLocation += TargetInterpolatedComponentLocation - OldTargetInterpolatedComponentLocation;
	DeltaInterpolatedComponentRotation += TargetInterpolatedComponentRotation - OldTargetInterpolatedComponentRotation;
	DeltaInterpolatedComponentRotation.Normalize();

	if (const AGameStateBase* const GameState = GetWorld()->GetGameState())
	{
		const float DeltaTime = GameState->GetServerWorldTimeSeconds() - PredictionData.ServerTime;
		if (DeltaTime > MinTickTime)
		{
			TickMovement(DeltaTime);
		}
	}
}
