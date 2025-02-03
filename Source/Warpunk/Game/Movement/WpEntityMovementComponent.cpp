#include "WpEntityMovementComponent.h"

#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Entity/WpPooledEntityData.h"

#include "Components/CapsuleComponent.h"

const float UWpEntityMovementComponent::IdealGroundDistance = 1.0f;
const float UWpEntityMovementComponent::MaxGroundDistance = IdealGroundDistance + 1.0f;

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

	const FVector InputVector = ConsumeInputVector().GetClampedToMaxSize(1.0);

	UpdateAcceleration(DeltaTime, InputVector);
	UpdateSpeed(DeltaTime);

	UpdateRotationAcceleration(DeltaTime, InputVector);
	UpdateRotationSpeed(DeltaTime);

	Rotate(DeltaTime);
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

void UWpEntityMovementComponent::OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
{
	ensureAlways(GetOwner()->HasAuthority());

	if (bIsAcquired)
	{
		const FWpPooledEntityData& PooledEntityData = static_cast<const FWpPooledEntityData&>(PooledObjectData);

		if (PooledEntityData.EntityConfig->Type == EWpEntityType::Unit)
		{
			const FWpMovementConfig& MovementConfig = PooledEntityData.EntityConfig->MovementConfig;

			ensureAlways(!FMath::IsNearlyZero(MovementConfig.MaxSpeed));
			ensureAlways(!FMath::IsNearlyZero(MovementConfig.MaxAcceleration));
			ensureAlways(!FMath::IsNearlyZero(MovementConfig.MaxInertDeceleration));
			ensureAlways(!FMath::IsNearlyZero(MovementConfig.MaxBrakingDeceleration));
			ensureAlways(!FMath::IsNearlyZero(MovementConfig.MaxRotationSpeed));
			ensureAlways(!FMath::IsNearlyZero(MovementConfig.MaxRotationAcceleration));
			ensureAlways(!FMath::IsNearlyZero(MovementConfig.MaxRotationInertDeceleration));
			ensureAlways(!FMath::IsNearlyZero(MovementConfig.MaxRotationBrakingDeceleration));
			ensureAlways(!FMath::IsNearlyZero(MovementConfig.MaxInclineAngle));
			ensureAlways(!FMath::IsNearlyZero(MovementConfig.MaxStepUpHeight));

			MaxSpeed = MovementConfig.MaxSpeed;
			MaxAcceleration = MovementConfig.MaxAcceleration;
			MaxInertDeceleration = MovementConfig.MaxInertDeceleration;
			MaxBrakingDeceleration = MovementConfig.MaxBrakingDeceleration;
			MaxRotationSpeed = MovementConfig.MaxRotationSpeed;
			MaxRotationAcceleration = MovementConfig.MaxRotationAcceleration;
			MaxRotationInertDeceleration = MovementConfig.MaxRotationInertDeceleration;
			MaxRotationBrakingDeceleration = MovementConfig.MaxRotationBrakingDeceleration;
			MaxInclineAngle = MovementConfig.MaxInclineAngle;
			MaxStepUpHeight = MovementConfig.MaxStepUpHeight;

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

void UWpEntityMovementComponent::UpdateAcceleration(const float DeltaTime, const FVector& InputVector)
{
	if (!bOnGround)
	{
		Acceleration.X = 0.0;
		Acceleration.Y = 0.0;
		return;
	}

	const FVector AccelerationInputVector = InputVector.X * UpdatedComponent->GetForwardVector();
	if (!AccelerationInputVector.IsNearlyZero())
	{
		const bool bAcceleratesInCurrentDirection = (AccelerationInputVector.Dot(Speed) >= 0.0);
		Acceleration = AccelerationInputVector * (bAcceleratesInCurrentDirection ? MaxAcceleration : MaxBrakingDeceleration);
	}
	else
	{
		Acceleration = MaxInertDeceleration * -Speed.GetSafeNormal();

		const bool bSpeedWillBeZero = Speed.Dot(CalcNewSpeed(DeltaTime)) <= 0.0;
		if (Speed.IsNearlyZero() || bSpeedWillBeZero)
		{
			Speed = FVector::ZeroVector;
			Acceleration = FVector::ZeroVector;
		}
	}
}

void UWpEntityMovementComponent::UpdateRotationAcceleration(const float DeltaTime, const FVector& InputVector)
{
	if (!bOnGround)
	{
		RotationAcceleration = 0.0f;
		return;
	}

	const float RotationAccelerationInput = InputVector.Y;
	if (!FMath::IsNearlyZero(RotationAccelerationInput))
	{
		const bool bAcceleratesInCurrentDirection = ((RotationAccelerationInput > 0.0f) == (RotationSpeed > 0.0f));
		RotationAcceleration = RotationAccelerationInput * (bAcceleratesInCurrentDirection ? MaxRotationAcceleration : MaxRotationBrakingDeceleration);
	}
	else
	{
		RotationAcceleration = MaxRotationInertDeceleration * ((RotationSpeed > 0) ? -1.0f : 1.0f);

		const bool bSpeedWillBeZero = ((CalcNewRotationSpeed(DeltaTime) > 0.0f) != (RotationSpeed > 0.0f));
		if (FMath::IsNearlyZero(RotationSpeed) || bSpeedWillBeZero)
		{
			RotationSpeed = 0.0f;
			RotationAcceleration = 0.0f;
		}
	}
}

void UWpEntityMovementComponent::UpdateSpeed(const float DeltaTime)
{
	Speed = CalcNewSpeed(DeltaTime);
}

void UWpEntityMovementComponent::UpdateRotationSpeed(const float DeltaTime)
{
	RotationSpeed = CalcNewRotationSpeed(DeltaTime);
}

FVector UWpEntityMovementComponent::CalcNewSpeed(const float DeltaTime) const
{
	return (Speed + Acceleration * DeltaTime).GetClampedToMaxSize(MaxSpeed);
}

float UWpEntityMovementComponent::CalcNewRotationSpeed(const float DeltaTime) const
{
	return FMath::Clamp(RotationSpeed + RotationAcceleration * DeltaTime, -MaxRotationSpeed, MaxRotationSpeed);
}

void UWpEntityMovementComponent::Rotate(const float DeltaTime)
{
	const FRotator OldRotation = UpdatedComponent->GetComponentRotation();
	const float DeltaAngle = RotationSpeed * DeltaTime;
	const FRotator NewRotation = OldRotation.Vector().RotateAngleAxis(DeltaAngle, FVector::UpVector).Rotation();

	UpdatedComponent->MoveComponent(FVector::ZeroVector, NewRotation, false);

	if (bOnGround)
	{
		Speed = Speed.RotateAngleAxis(DeltaAngle, FVector::UpVector);
	}
}

void UWpEntityMovementComponent::Move(const float DeltaTime)
{
	const FRotator Rotation = UpdatedComponent->GetComponentRotation();

	float RemainingTime = DeltaTime;
	bool bIsSteppingUp = false;

	while (!FMath::IsNearlyZero(RemainingTime))
	{
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

		if (FMath::IsNearlyZero(Hit.Time))
		{
			break;
		}

		const float SpentTime = Hit.Time * RemainingTime;
		RemainingTime -= SpentTime;
		bIsSteppingUp = false;

		if (Hit.bBlockingHit && !Hit.ImpactNormal.Equals(FVector::UpVector))
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
					ProcessWallImpact();
				}
			}
		}
	}
}

void UWpEntityMovementComponent::UpdateGround(const float DeltaTime)
{
	const bool bWasOnGround = bOnGround;

	const FVector StartLocation = UpdatedComponent->GetComponentLocation();
	const FVector EndLocation = StartLocation - FVector::UpVector * (1.0f + MaxGroundDistance);
	const FQuat Rotation = UpdatedComponent->GetComponentQuat();
	const FName CollisionProfile = Cast<UCapsuleComponent>(UpdatedComponent)->GetCollisionProfileName();
	float Radius, HalfHeight;
	Cast<UCapsuleComponent>(UpdatedComponent)->GetScaledCapsuleSize(Radius, HalfHeight);
	const FCollisionShape CollisionShape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.AddIgnoredActor(GetOwner());

	FHitResult Hit;
	GetWorld()->SweepSingleByProfile(Hit, StartLocation, EndLocation, Rotation, CollisionProfile, CollisionShape, CollisionQueryParams);

	if (Hit.bBlockingHit)
	{
		if (StartLocation.Z - Hit.Location.Z > MaxGroundDistance)
		{
			bOnGround = false;
		}
		else
		{
			const float InclineAngle = FMath::RadiansToDegrees(FMath::Acos(Hit.ImpactNormal.Dot(FVector::UpVector)));
			bOnGround = (InclineAngle < MaxInclineAngle);
		}
	}
	else
	{
		bOnGround = false;
	}

	if (bOnGround)
	{
		Speed.Z = 0.0;

		if (bWasOnGround)
		{
			const FVector NewSpeedDirection = UpdatedComponent->GetComponentRotation().Vector();
			Speed = Speed.ProjectOnToNormal(NewSpeedDirection);
		}

		CurrentGroundNormal = Hit.ImpactNormal;
	}
	else
	{
		const float VerticalAcceleration = GetGravityZ();
		Speed.Z += VerticalAcceleration * DeltaTime;

		CurrentGroundNormal = Hit.bBlockingHit ? Hit.ImpactNormal : FVector::UpVector;
	}

	if (Hit.bBlockingHit)
	{
		FVector OnGroundLocation = Hit.Location;
		OnGroundLocation.Z += IdealGroundDistance;
		const FVector DeltaLocation = OnGroundLocation - UpdatedComponent->GetComponentLocation();
		UpdatedComponent->MoveComponent(DeltaLocation, Rotation, true);
	}
}

bool UWpEntityMovementComponent::StepUp(const float DeltaTime)
{
	const FRotator Rotation = UpdatedComponent->GetComponentRotation();
	const FVector OldLocation = UpdatedComponent->GetComponentLocation();

	FHitResult Hit;

	const FVector StepUpDeltaLocation = FVector{0.0, 0.0, MaxStepUpHeight};
	UpdatedComponent->MoveComponent(StepUpDeltaLocation, Rotation, true, &Hit);

	if (Hit.bBlockingHit)
	{
		RevertLocationTo(OldLocation);
		return false;
	}

	const FVector StepUpOldLocation = UpdatedComponent->GetComponentLocation();

	const FVector StepForwardDeltaLocation = CalcDeltaLocation(DeltaTime, FVector::UpVector);
	UpdatedComponent->MoveComponent(StepForwardDeltaLocation, Rotation, true, &Hit);

	if (Hit.bBlockingHit)
	{
		const float InclineAngle = FMath::RadiansToDegrees(FMath::Acos(Hit.ImpactNormal.Dot(FVector::UpVector)));
		if (InclineAngle > MaxInclineAngle)
		{
			RevertLocationTo(StepUpOldLocation);
			RevertLocationTo(OldLocation);
			return false;
		}

		RevertLocationTo(StepUpOldLocation);

		const FVector StepForwardInclineDeltaLocation = CalcDeltaLocation(DeltaTime, Hit.ImpactNormal);
		UpdatedComponent->MoveComponent(StepForwardInclineDeltaLocation, Rotation, true, &Hit);

		if (Hit.bBlockingHit)
		{
			RevertLocationTo(StepUpOldLocation);
			RevertLocationTo(OldLocation);
			return false;
		}
	}

	const FVector StartLocation = UpdatedComponent->GetComponentLocation();
	const FVector EndLocation = StartLocation - FVector::UpVector * (1.0f + MaxStepUpHeight);
	const FName CollisionProfile = Cast<UCapsuleComponent>(UpdatedComponent)->GetCollisionProfileName();
	float Radius, HalfHeight;
	Cast<UCapsuleComponent>(UpdatedComponent)->GetScaledCapsuleSize(Radius, HalfHeight);
	const FCollisionShape CollisionShape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.AddIgnoredActor(GetOwner());

	GetWorld()->SweepSingleByProfile(Hit, StartLocation, EndLocation, Rotation.Quaternion(), CollisionProfile, CollisionShape, CollisionQueryParams);
	ensureAlways(Hit.bBlockingHit);

	RevertLocationTo(StepUpOldLocation);

	FVector GroundLocation = Hit.Location;
	GroundLocation.Z += IdealGroundDistance;
	const FVector GroundDeltaLocation = GroundLocation - UpdatedComponent->GetComponentLocation();
	UpdatedComponent->MoveComponent(GroundDeltaLocation, Rotation, true);

	CurrentGroundNormal = FVector::UpVector;
	return true;
}

void UWpEntityMovementComponent::ProcessWallImpact()
{
	Speed.X = 0.0;
	Speed.Y = 0.0;
	RotationSpeed = 0.0f;
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

void UWpEntityMovementComponent::RevertLocationTo(const FVector& OldLocation)
{
	const FVector RevertDeltaLocation = OldLocation - UpdatedComponent->GetComponentLocation();
	const FRotator Rotation = UpdatedComponent->GetComponentRotation();
	FHitResult Hit;
	UpdatedComponent->MoveComponent(RevertDeltaLocation, Rotation, true, &Hit);
	ensureAlways(!Hit.bBlockingHit);
}
