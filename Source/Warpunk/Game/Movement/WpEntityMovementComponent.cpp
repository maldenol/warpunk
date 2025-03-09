#include "WpEntityMovementComponent.h"

#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Entity/WpPooledEntityData.h"

#include "Components/CapsuleComponent.h"

const uint32 UWpEntityMovementComponent::MaxMoveIterationNum = 10;

const float UWpEntityMovementComponent::IdealGroundDistance = 1.0f;
const float UWpEntityMovementComponent::MaxGroundDistance = IdealGroundDistance + 1.0f;

const float UWpEntityMovementComponent::WallBumpIntensity = 0.1f;

UWpEntityMovementComponent::UWpEntityMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.SetTickFunctionEnable(false);

	SetIsReplicatedByDefault(false);

	// NetworkNoSmoothUpdateDistance = 100.0f; todo

	FNavAgentProperties& NavAgentProperties = GetNavAgentPropertiesRef();
	NavAgentProperties.bCanCrouch = false;
	NavAgentProperties.bCanFly = false;
	NavAgentProperties.bCanJump = false;
	NavAgentProperties.bCanSwim = false;
	NavAgentProperties.bCanWalk = true;
}

void UWpEntityMovementComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* const ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsActive() || !GetOwner()->HasAuthority())
	{
		return; // todo
	}

	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	const FVector InputVector = ConsumeInputVector();

	UpdateWheelRotation(DeltaTime, InputVector);

	UpdateRotationAcceleration(DeltaTime, InputVector);
	UpdateRotationSpeed(DeltaTime);
	Rotate(DeltaTime);

	UpdateAcceleration(DeltaTime, InputVector);
	UpdateSpeed(DeltaTime);
	Move(DeltaTime);
}

void UWpEntityMovementComponent::OnRep_IsActive()
{
	// Override to not enable tick
}

void UWpEntityMovementComponent::SetUpdatedComponent(USceneComponent* const NewUpdatedComponent)
{
	ensureAlways(NewUpdatedComponent == NewUpdatedComponent->GetOwner()->GetRootComponent());
	ensureAlways(NewUpdatedComponent->IsA<UCapsuleComponent>());

	Super::SetUpdatedComponent(NewUpdatedComponent);
}

void UWpEntityMovementComponent::UpdateTickRegistration()
{
	// Override to not enable tick
}

FVector UWpEntityMovementComponent::ConsumeInputVector()
{
	FVector InputVector = Super::ConsumeInputVector();
	InputVector.Z = 0.0;

	if (MovementType == EWpEntityMovementType::Legs || MovementType == EWpEntityMovementType::Tracks)
	{
		InputVector.Normalize();
	}
	else if (MovementType == EWpEntityMovementType::Wheels)
	{
		InputVector.X = FMath::Min(InputVector.X, 1.0);
		InputVector.Y = FMath::Min(InputVector.Y, 1.0);
	}

	return InputVector;
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

			ensureAlways(MovementConfig.Type != EWpEntityMovementType::Undefined);
			ensureAlways(!FMath::IsNearlyZero(MovementConfig.MaxSpeed));
			ensureAlways(!FMath::IsNearlyZero(MovementConfig.MaxAcceleration));
			ensureAlways(!FMath::IsNearlyZero(MovementConfig.MaxInertDeceleration));
			ensureAlways(!FMath::IsNearlyZero(MovementConfig.MaxBrakingDeceleration));
			ensureAlways(!FMath::IsNearlyZero(MovementConfig.MaxRotationSpeed));
			ensureAlways(!FMath::IsNearlyZero(MovementConfig.MaxRotationAcceleration));
			ensureAlways(!FMath::IsNearlyZero(MovementConfig.MaxRotationInertDeceleration));
			ensureAlways(!FMath::IsNearlyZero(MovementConfig.MaxRotationBrakingDeceleration));
			if (MovementConfig.Type == EWpEntityMovementType::Wheels)
			{
				ensureAlways(!FMath::IsNearlyZero(MovementConfig.MaxWheelRotationAngle));
				ensureAlways(!FMath::IsNearlyZero(MovementConfig.MaxWheelRotationSpeed));
			}
			ensureAlways(!FMath::IsNearlyZero(MovementConfig.MaxInclineAngle));
			ensureAlways(!FMath::IsNearlyZero(MovementConfig.MaxStepUpHeight));
			if (MovementConfig.Type == EWpEntityMovementType::Tracks || MovementConfig.Type == EWpEntityMovementType::Wheels)
			{
				ensureAlways(!FMath::IsNearlyZero(MovementConfig.MaxGroundFriction));
			}

			MovementType = MovementConfig.Type;
			MaxSpeed = MovementConfig.MaxSpeed;
			MaxAcceleration = MovementConfig.MaxAcceleration;
			MaxInertDeceleration = MovementConfig.MaxInertDeceleration;
			MaxBrakingDeceleration = MovementConfig.MaxBrakingDeceleration;
			MaxRotationSpeed = MovementConfig.MaxRotationSpeed;
			MaxRotationAcceleration = MovementConfig.MaxRotationAcceleration;
			MaxRotationInertDeceleration = MovementConfig.MaxRotationInertDeceleration;
			MaxRotationBrakingDeceleration = MovementConfig.MaxRotationBrakingDeceleration;
			MaxWheelRotationAngle = MovementConfig.MaxWheelRotationAngle;
			MaxWheelRotationSpeed = MovementConfig.MaxWheelRotationSpeed;
			MaxInclineAngle = MovementConfig.MaxInclineAngle;
			MaxStepUpHeight = MovementConfig.MaxStepUpHeight;
			MaxGroundFriction = MovementConfig.MaxGroundFriction;

			SetIsFirstPersonControlled(false);
		}
		else
		{
			bIsAcquired = false;
		}
	}

	SetActive(bIsAcquired);
	SetComponentTickEnabled(bIsAcquired);
}

void UWpEntityMovementComponent::OnPooledObjectVisibilityChanged(const bool bIsVisible)
{
	if (!GetOwner()->HasAuthority())
	{
		// SetComponentTickEnabled(bIsVisible); todo
	}
}

void UWpEntityMovementComponent::AddMovementInput(const float Forward, const float Right)
{
	AddInputVector(FVector{Forward, Right, 0.0});
}

void UWpEntityMovementComponent::SetIsFirstPersonControlled(const bool bIsFirstPersonControlled)
{
	if (bIsFirstPersonControlled)
	{
		const APawn* const OwnerPawn = GetOwner<APawn>();
		if (AController* const OwnerController = (OwnerPawn->GetLocalRole() == ROLE_SimulatedProxy) ? GetWorld()->GetFirstPlayerController() : OwnerPawn->GetController())
		{
			OwnerController->SetControlRotation(OwnerPawn->GetActorRotation());
		}
	}
}

void UWpEntityMovementComponent::UpdateWheelRotation(const float DeltaTime, const FVector& InputVector)
{
	if (MovementType != EWpEntityMovementType::Wheels)
	{
		return;
	}

	float WheelRotationSpeed = 0.0f;
	if (!FMath::IsNearlyZero(InputVector.Y))
	{
		WheelRotationSpeed = MaxWheelRotationSpeed * InputVector.Y;
	}
	else if (!FMath::IsNearlyZero(WheelRotationAngle))
	{
		WheelRotationSpeed = MaxWheelRotationSpeed * ((WheelRotationAngle > 0.0f) ? -1.0f : 1.0f);

		const bool bAngleWillBeZero = (CalcNewWheelRotationAngle(DeltaTime, WheelRotationSpeed) > 0.0f) != (WheelRotationAngle > 0.0f);
		if (bAngleWillBeZero)
		{
			WheelRotationAngle = 0.0f;
			WheelRotationSpeed = 0.0f;
		}
	}

	if (!FMath::IsNearlyZero(WheelRotationSpeed))
	{
		WheelRotationAngle = CalcNewWheelRotationAngle(DeltaTime, WheelRotationSpeed);
	}
}

float UWpEntityMovementComponent::CalcNewWheelRotationAngle(const float DeltaTime, const float WheelRotationSpeed) const
{
	return FMath::Clamp(WheelRotationAngle + WheelRotationSpeed * DeltaTime, -MaxWheelRotationAngle, MaxWheelRotationAngle);
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
		ensureAlways(false);
		break;
	}
}

void UWpEntityMovementComponent::ApplyRotationAccelerationLegs(const float DeltaTime)
{
	ensureAlways(bOnGround);

	const FVector EntityDirectionForward = UpdatedComponent->GetForwardVector();
	const FVector MovementDirection = Speed.GetSafeNormal2D();
	const float DeltaYaw = FQuat::FindBetweenNormals(EntityDirectionForward, MovementDirection).Rotator().Yaw;

	if (FMath::IsNearlyZero(RotationSpeed) && FMath::IsNearlyZero(DeltaYaw, 0.5f))
	{
		return;
	}

	const float StoppingYaw = FMath::Square(RotationSpeed) / (2.0f * MaxRotationBrakingDeceleration);

	if (FMath::Abs(DeltaYaw) > StoppingYaw)
	{
		RotationAcceleration += MaxRotationAcceleration * ((DeltaYaw > 0.0f) ? 1.0f : -1.0f);
	}
	else
	{
		RotationAcceleration += MaxRotationBrakingDeceleration * ((RotationSpeed > 0.0f) ? -1.0f : 1.0f);

		if (!FMath::IsNearlyZero(RotationSpeed))
		{
			const bool bSpeedWillBeZero = (CalcNewRotationSpeed(DeltaTime) > 0.0f) != (RotationSpeed > 0.0f);
			if (bSpeedWillBeZero)
			{
				RotationSpeed = 0.0f;
				RotationAcceleration = 0.0f;

				if (!Speed.IsNearlyZero())
				{
					UpdatedComponent->SetRelativeRotation(Speed.GetSafeNormal2D().Rotation());
				}
			}
		}
	}
}

void UWpEntityMovementComponent::ApplyRotationAccelerationTracks(const float DeltaTime, const FVector& InputVector)
{
	ensureAlways(bOnGround);

	const float InputRotationAcceleration = InputVector.Y;

	if (!FMath::IsNearlyZero(InputRotationAcceleration))
	{
		const bool bAcceleratesInCurrentDirection = (InputRotationAcceleration > 0.0f) == (RotationSpeed > 0.0f);
		RotationAcceleration += InputRotationAcceleration * (bAcceleratesInCurrentDirection ? MaxRotationAcceleration : MaxRotationBrakingDeceleration);
	}
	else if (!FMath::IsNearlyZero(RotationSpeed))
	{
		RotationAcceleration += MaxRotationInertDeceleration * ((RotationSpeed > 0.0f) ? -1.0f : 1.0f);
	}

	if (!FMath::IsNearlyZero(RotationSpeed) && FMath::IsNearlyZero(InputRotationAcceleration))
	{
		const bool bSpeedWillBeZero = (CalcNewRotationSpeed(DeltaTime) > 0.0f) != (RotationSpeed > 0.0f);
		if (bSpeedWillBeZero)
		{
			RotationSpeed = 0.0f;
			RotationAcceleration = 0.0f;
		}
	}
}

void UWpEntityMovementComponent::ApplyRotationAccelerationWheels(const float DeltaTime)
{
	ensureAlways(bOnGround);

	RotationSpeed = 0.0f;

	if (!Speed.IsNearlyZero() && !FMath::IsNearlyZero(WheelRotationAngle))
	{
		const float RotationSpeedDirection = (Speed.Dot(UpdatedComponent->GetForwardVector()) > 0.0f) ? 1.0f : -1.0f;
		const float NewRotationSpeed = Speed.Size2D() * FMath::Tan(FMath::DegreesToRadians(WheelRotationAngle)) * RotationSpeedDirection;
		RotationAcceleration += NewRotationSpeed / DeltaTime;
	}
}

void UWpEntityMovementComponent::UpdateRotationSpeed(const float DeltaTime)
{
	RotationSpeed = CalcNewRotationSpeed(DeltaTime);
}

float UWpEntityMovementComponent::CalcNewRotationSpeed(const float DeltaTime) const
{
	float NewRotationSpeed = RotationSpeed + RotationAcceleration * DeltaTime;
	if (MovementType != EWpEntityMovementType::Wheels)
	{
		NewRotationSpeed = FMath::Clamp(NewRotationSpeed, -MaxRotationSpeed, MaxRotationSpeed);
	}
	return NewRotationSpeed;
}

void UWpEntityMovementComponent::Rotate(const float DeltaTime)
{
	const float DeltaAngle = RotationSpeed * DeltaTime;
	if (FMath::IsNearlyZero(DeltaAngle))
	{
		return;
	}

	const FRotator OldRotation = UpdatedComponent->GetComponentRotation();
	const FRotator NewRotation = OldRotation.Vector().RotateAngleAxis(DeltaAngle, FVector::UpVector).Rotation();

	UpdatedComponent->MoveComponent(FVector::ZeroVector, NewRotation, false);

	if (MovementType == EWpEntityMovementType::Wheels)
	{
		const FVector OldForwardSpeed = Speed.ProjectOnToNormal(UpdatedComponent->GetForwardVector());
		const FVector NewForwardSpeed = OldForwardSpeed.RotateAngleAxis(DeltaAngle, FVector::UpVector);
		const FVector DeltaForwardSpeed = NewForwardSpeed - OldForwardSpeed;
		Speed += DeltaForwardSpeed;
	}
}

void UWpEntityMovementComponent::UpdateAcceleration(const float DeltaTime, const FVector& InputVector)
{
	Acceleration = FVector::ZeroVector;

	if (!bOnGround)
	{
		return;
	}

	switch (MovementType)
	{
	case EWpEntityMovementType::Legs:
		ApplyInputAcceleration(InputVector.X, GetControllerDirectionForward());
		ApplyInputAcceleration(InputVector.Y, GetControllerDirectionRight());
		break;
	case EWpEntityMovementType::Tracks:
	case EWpEntityMovementType::Wheels:
		ApplyInputAcceleration(InputVector.X, UpdatedComponent->GetForwardVector());
		break;
	default:
		ensureAlways(false);
		break;
	}

	ApplyForces();

	if (!Speed.IsNearlyZero() && InputVector.IsNearlyZero())
	{
		const bool bSpeedWillBeZero = Speed.Dot(CalcNewSpeed(DeltaTime)) <= 0.0;
		if (bSpeedWillBeZero)
		{
			Speed = FVector::ZeroVector;
			Acceleration = FVector::ZeroVector;
		}
	}
}

void UWpEntityMovementComponent::ApplyInputAcceleration(const float Input, const FVector& Direction)
{
	ensureAlways(bOnGround);

	const FVector InputAcceleration = Input * Direction;
	const FVector SpeedInDirection = Speed.ProjectOnToNormal(Direction);

	if (!InputAcceleration.IsNearlyZero())
	{
		const bool bAcceleratesInCurrentDirection = InputAcceleration.Dot(SpeedInDirection) >= 0.0;
		Acceleration += InputAcceleration * (bAcceleratesInCurrentDirection ? MaxAcceleration : MaxBrakingDeceleration);
	}
	else if (!SpeedInDirection.IsNearlyZero())
	{
		Acceleration += MaxInertDeceleration * -SpeedInDirection.GetSafeNormal();
	}
}

void UWpEntityMovementComponent::ApplyForces()
{
	ensureAlways(bOnGround);

	if (Speed.IsNearlyZero())
	{
		return;
	}

	if (MovementType == EWpEntityMovementType::Tracks || MovementType == EWpEntityMovementType::Wheels)
	{
		const FVector LateralSpeed = Speed.ProjectOnToNormal(UpdatedComponent->GetRightVector());
		const float LateralDecelerationScale = MaxGroundFriction * CurrentGroundNormal.Dot(FVector::UpVector);
		const FVector LateralDeceleration = LateralDecelerationScale * -LateralSpeed.GetSafeNormal2D();
		Acceleration += LateralDeceleration;
	}

	if (!CurrentGroundNormal.Equals(FVector::UpVector))
	{
		const float GravityAccelerationScale = 0.5f * -GetWorld()->GetGravityZ() * FMath::Sqrt(1.0f - FMath::Square(CurrentGroundNormal.Dot(FVector::UpVector)));
		const FVector GravityAccelerationDirection = FVector::VectorPlaneProject(CurrentGroundNormal, FVector::UpVector).GetUnsafeNormal2D();
		const FVector GravityAcceleration = GravityAccelerationScale * GravityAccelerationDirection;
		Acceleration += GravityAcceleration;
	}
}

void UWpEntityMovementComponent::UpdateSpeed(const float DeltaTime)
{
	Speed = CalcNewSpeed(DeltaTime);
}

FVector UWpEntityMovementComponent::CalcNewSpeed(const float DeltaTime) const
{
	return (Speed + Acceleration * DeltaTime).GetClampedToMaxSize2D(MaxSpeed);
}

void UWpEntityMovementComponent::Move(const float DeltaTime)
{
	if (Speed.IsNearlyZero())
	{
		return;
	}

	const FRotator Rotation = UpdatedComponent->GetComponentRotation();

	uint32 IterationIndex = 0;
	float RemainingTime = DeltaTime;
	bool bIsSteppingUp = false;

	while ((IterationIndex < MaxMoveIterationNum) && !FMath::IsNearlyZero(RemainingTime))
	{
		++IterationIndex;

		if (!bIsSteppingUp)
		{
			UpdateGround(RemainingTime);
		}

		const FVector DeltaLocation = CalcDeltaLocation(RemainingTime, CurrentGroundNormal);
		if (DeltaLocation.IsNearlyZero())
		{
			break;
		}

		FHitResult Hit;
		UpdatedComponent->MoveComponent(DeltaLocation, Rotation, true, &Hit);

		const float SpentTime = Hit.Time * RemainingTime;
		RemainingTime -= SpentTime;
		bIsSteppingUp = false;

		if (Hit.bBlockingHit)
		{
			const float InclineAngle = FMath::RadiansToDegrees(FMath::Acos(Hit.ImpactNormal.Dot(FVector::UpVector)));
			if (InclineAngle > MaxInclineAngle)
			{
				if (StepUp(RemainingTime))
				{
					bIsSteppingUp = true;
				}
				else
				{
					ProcessWallImpact(Hit.ImpactNormal);
				}
			}
		}
	}
}

FVector UWpEntityMovementComponent::CalcDeltaLocation(const float DeltaTime, const FVector& GroundNormal) const
{
	if (GroundNormal.Equals(FVector::UpVector))
	{
		return Speed * DeltaTime;
	}

	if (!bOnGround)
	{
		return (CalcInclineDownSpeed(GroundNormal) + CalcInclineSideSpeed(GroundNormal)) * DeltaTime;
	}

	return (CalcInclineUpSpeed(GroundNormal) + CalcInclineSideSpeed(GroundNormal)) * DeltaTime;
}

FVector UWpEntityMovementComponent::CalcInclineUpSpeed(const FVector& GroundNormal) const
{
	ensureAlways(!GroundNormal.Equals(FVector::UpVector));
	ensureAlways(bOnGround);

	const float InclineAngle = FMath::RadiansToDegrees(FMath::Acos(GroundNormal.Dot(FVector::UpVector)));
	ensureAlways(InclineAngle < MaxInclineAngle);

	const FVector HorizontalGroundNormal = GroundNormal.GetSafeNormal2D();
	const FVector HorizontalInclineUpSpeed = Speed.ProjectOnToNormal(-HorizontalGroundNormal);
	const FVector InclineSideDirection = HorizontalGroundNormal.Cross(FVector::UpVector);
	const FVector InclineUpSpeed = HorizontalInclineUpSpeed.RotateAngleAxis(-InclineAngle, InclineSideDirection);

	return InclineUpSpeed;
}

FVector UWpEntityMovementComponent::CalcInclineDownSpeed(const FVector& GroundNormal) const
{
	ensureAlways(!GroundNormal.Equals(FVector::UpVector));
	ensureAlways(!bOnGround);

	const FVector DownSpeed = Speed * FVector::UpVector;
	const FVector InclineDownSpeed = FVector::VectorPlaneProject(DownSpeed, GroundNormal);

	return InclineDownSpeed;
}

FVector UWpEntityMovementComponent::CalcInclineSideSpeed(const FVector& GroundNormal) const
{
	ensureAlways(!GroundNormal.Equals(FVector::UpVector));

	const FVector HorizontalGroundNormal = GroundNormal.GetSafeNormal2D();
	const FVector InclineSideDirection = HorizontalGroundNormal.Cross(FVector::UpVector);
	const FVector InclineSideSpeed = Speed.ProjectOnToNormal(InclineSideDirection);

	return InclineSideSpeed;
}

void UWpEntityMovementComponent::UpdateGround(const float DeltaTime)
{
	FHitResult Hit;
	SweepUnder(Hit, 1.0f + MaxGroundDistance);

	if (Hit.bBlockingHit)
	{
		if (UpdatedComponent->GetComponentLocation().Z - Hit.Location.Z > MaxGroundDistance)
		{
			bOnGround = false;
		}
		else
		{
			const float InclineAngle = FMath::RadiansToDegrees(FMath::Acos(Hit.ImpactNormal.Dot(FVector::UpVector)));
			bOnGround = InclineAngle < MaxInclineAngle;
		}
	}
	else
	{
		bOnGround = false;
	}

	if (bOnGround)
	{
		CurrentGroundNormal = Hit.ImpactNormal;

		Speed.Z = 0.0;
	}
	else
	{
		CurrentGroundNormal = Hit.bBlockingHit ? Hit.ImpactNormal : FVector::UpVector;

		const float VerticalAcceleration = GetGravityZ();
		Speed.Z += VerticalAcceleration * DeltaTime;
	}

	if (Hit.bBlockingHit)
	{
		FVector OnGroundLocation = Hit.Location;
		OnGroundLocation.Z += IdealGroundDistance;
		const FVector DeltaLocation = OnGroundLocation - UpdatedComponent->GetComponentLocation();
		UpdatedComponent->MoveComponent(DeltaLocation, UpdatedComponent->GetComponentRotation(), true);
	}
}

bool UWpEntityMovementComponent::StepUp(const float DeltaTime)
{
	const FRotator Rotation = UpdatedComponent->GetComponentRotation();
	const FVector StepUpOldLocation = UpdatedComponent->GetComponentLocation();

	FHitResult Hit;

	const FVector StepUpDeltaLocation = FVector{0.0, 0.0, MaxStepUpHeight};
	UpdatedComponent->MoveComponent(StepUpDeltaLocation, Rotation, true, &Hit);

	if (Hit.bBlockingHit)
	{
		RevertLocationTo(StepUpOldLocation);
		return false;
	}

	const FVector StepForwardOldLocation = UpdatedComponent->GetComponentLocation();

	const FVector StepForwardDeltaLocation = CalcDeltaLocation(DeltaTime, FVector::UpVector);
	UpdatedComponent->MoveComponent(StepForwardDeltaLocation, Rotation, true, &Hit);

	if (Hit.bBlockingHit)
	{
		const float InclineAngle = FMath::RadiansToDegrees(FMath::Acos(Hit.ImpactNormal.Dot(FVector::UpVector)));
		if (InclineAngle > MaxInclineAngle)
		{
			RevertLocationTo(StepForwardOldLocation);
			RevertLocationTo(StepUpOldLocation);
			return false;
		}

		RevertLocationTo(StepForwardOldLocation);

		const FVector StepForwardInclineDeltaLocation = CalcDeltaLocation(DeltaTime, Hit.ImpactNormal);
		UpdatedComponent->MoveComponent(StepForwardInclineDeltaLocation, Rotation, true, &Hit);

		if (Hit.bBlockingHit)
		{
			RevertLocationTo(StepForwardOldLocation);
			RevertLocationTo(StepUpOldLocation);
			return false;
		}
	}

	SweepUnder(Hit, 1.0f + MaxGroundDistance + MaxStepUpHeight);

	if (Hit.bBlockingHit)
	{
		const float InclineAngle = FMath::RadiansToDegrees(FMath::Acos(Hit.ImpactNormal.Dot(FVector::UpVector)));
		if (InclineAngle > MaxInclineAngle)
		{
			RevertLocationTo(StepForwardOldLocation);
			RevertLocationTo(StepUpOldLocation);
			return false;
		}
	}
	else
	{
		Hit.Location = Hit.TraceStart - FVector::UpVector * (MaxStepUpHeight + IdealGroundDistance);
	}

	RevertLocationTo(StepForwardOldLocation);

	const FVector GroundLocation = Hit.Location + FVector::UpVector * IdealGroundDistance;
	const FVector GroundDeltaLocation = GroundLocation - UpdatedComponent->GetComponentLocation();
	UpdatedComponent->MoveComponent(GroundDeltaLocation, Rotation, true);

	CurrentGroundNormal = FVector::UpVector;
	return true;
}

void UWpEntityMovementComponent::ProcessWallImpact(const FVector& ImpactNormal)
{
	const FVector HorizontalImpactNormal = ImpactNormal.GetUnsafeNormal2D();
	const FVector OldSpeedIntoWall = Speed.ProjectOnToNormal(HorizontalImpactNormal);
	const FVector NewSpeedIntoWall = OldSpeedIntoWall * -WallBumpIntensity;
	const FVector DeltaSpeedIntoWall = NewSpeedIntoWall - OldSpeedIntoWall;
	Speed += DeltaSpeedIntoWall;

	RotationSpeed = 0.0f;
}

void UWpEntityMovementComponent::SweepUnder(FHitResult& OutHit, const float Distance) const
{
	const FVector StartLocation = UpdatedComponent->GetComponentLocation();
	const FVector EndLocation = StartLocation - FVector::UpVector * Distance;
	const FQuat Rotation = UpdatedComponent->GetComponentQuat();
	const FName CollisionProfile = Cast<UCapsuleComponent>(UpdatedComponent)->GetCollisionProfileName();
	float Radius, HalfHeight;
	Cast<UCapsuleComponent>(UpdatedComponent)->GetScaledCapsuleSize(Radius, HalfHeight);
	const FCollisionShape CollisionShape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.AddIgnoredActor(GetOwner());

	GetWorld()->SweepSingleByProfile(OutHit, StartLocation, EndLocation, Rotation, CollisionProfile, CollisionShape, CollisionQueryParams);
}

void UWpEntityMovementComponent::RevertLocationTo(const FVector& OldLocation)
{
	const FVector RevertDeltaLocation = OldLocation - UpdatedComponent->GetComponentLocation();
	const FRotator Rotation = UpdatedComponent->GetComponentRotation();
	FHitResult Hit;
	UpdatedComponent->MoveComponent(RevertDeltaLocation, Rotation, true, &Hit);
	ensureAlways(!Hit.bBlockingHit);
}

FVector UWpEntityMovementComponent::GetControllerDirectionForward() const
{
	return GetOwner<APawn>()->GetController()->GetControlRotation().Vector();
}

FVector UWpEntityMovementComponent::GetControllerDirectionRight() const
{
	return FVector::UpVector.Cross(GetControllerDirectionForward());
}
