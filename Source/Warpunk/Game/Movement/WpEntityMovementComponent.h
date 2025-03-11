#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Movement/WpEntityMovementType.h"
#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"

#include "GameFramework/PawnMovementComponent.h"

#include "WpEntityMovementComponent.generated.h"

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class UWpEntityMovementComponent final : public UPawnMovementComponent, public IWpPooledObjectInterface
{
	GENERATED_BODY()

public:
	UWpEntityMovementComponent();

	virtual void TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* const ThisTickFunction) override;

	virtual void OnRep_IsActive() override;

	virtual void SetUpdatedComponent(USceneComponent* const NewUpdatedComponent) override;
	virtual void UpdateTickRegistration() override;

	virtual float GetMaxSpeed() const override;

	virtual void RequestPathMove(const FVector& MoveInput) override;

	virtual FVector ConsumeInputVector() override;

	virtual void OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const FWpPooledObjectData& PooledObjectData) override;
	virtual void OnPooledObjectVisibilityChanged(const bool bIsVisible) override;

	void AddMovementInput(const float Forward, const float Right);

	void SetIsFirstPersonControlled(const bool bInIsFirstPersonControlled);

	void SetCollisionSize(const float Radius, const float Height);

	float GetBrakingDistance() const;

private:
	void UpdateWheelRotation(const float DeltaTime, const FVector& InputVector);
	float CalcNewWheelRotationAngle(const float DeltaTime, const float WheelRotationSpeed) const;

	void UpdateRotationAcceleration(const float DeltaTime, const FVector& InputVector);
	void ApplyRotationAccelerationLegs(const float DeltaTime);
	void ApplyRotationAccelerationTracks(const float DeltaTime, const FVector& InputVector);
	void ApplyRotationAccelerationWheels(const float DeltaTime);

	void UpdateRotationSpeed(const float DeltaTime);
	float CalcNewRotationSpeed(const float DeltaTime) const;

	void Rotate(const float DeltaTime);

	void UpdateAcceleration(const float DeltaTime, const FVector& InputVector);
	void ApplyInputAcceleration(const float Input, const FVector& Direction);
	void ApplyForces();

	void UpdateSpeed(const float DeltaTime);
	FVector CalcNewSpeed(const float DeltaTime) const;

	void Move(const float DeltaTime);

	FVector CalcDeltaLocation(const float DeltaTime, const FVector& GroundNormal) const;
	FVector CalcInclineUpSpeed(const FVector& GroundNormal) const;
	FVector CalcInclineDownSpeed(const FVector& GroundNormal) const;
	FVector CalcInclineSideSpeed(const FVector& GroundNormal) const;

	void UpdateGround(const float DeltaTime);

	bool StepUp(const float DeltaTime);

	void ProcessWallImpact(const FVector& ImpactNormal);

	void SweepUnder(FHitResult& OutHit, const float Distance) const;
	void RevertLocationTo(const FVector& OldLocation);

	FVector GetControllerDirectionForward() const;
	FVector GetControllerDirectionRight() const;

	UPROPERTY(VisibleInstanceOnly)
	EWpEntityMovementType MovementType = EWpEntityMovementType::Undefined;

	UPROPERTY(VisibleInstanceOnly)
	float MaxSpeed = 0.0f;
	UPROPERTY(VisibleInstanceOnly)
	float MaxAcceleration = 0.0f;
	UPROPERTY(VisibleInstanceOnly)
	float MaxInertDeceleration = 0.0f;
	UPROPERTY(VisibleInstanceOnly)
	float MaxBrakingDeceleration = 0.0f;

	UPROPERTY(VisibleInstanceOnly)
	float MaxRotationSpeed = 0.0f;
	UPROPERTY(VisibleInstanceOnly)
	float MaxRotationAcceleration = 0.0f;
	UPROPERTY(VisibleInstanceOnly)
	float MaxRotationInertDeceleration = 0.0f;
	UPROPERTY(VisibleInstanceOnly)
	float MaxRotationBrakingDeceleration = 0.0f;

	UPROPERTY(VisibleInstanceOnly)
	float MaxWheelRotationAngle = 0.0f;
	UPROPERTY(VisibleInstanceOnly)
	float MaxWheelRotationSpeed = 0.0f;

	UPROPERTY(VisibleInstanceOnly)
	float MaxInclineAngle = 0.0f;
	UPROPERTY(VisibleInstanceOnly)
	float MaxStepUpHeight = 0.0f;

	UPROPERTY(VisibleInstanceOnly)
	float MaxGroundFriction = 0.0f;

	FVector Speed = FVector::ZeroVector;
	FVector Acceleration = FVector::ZeroVector;

	float RotationSpeed = 0.0f;
	float RotationAcceleration = 0.0f;

	float WheelRotationAngle = 0.0f;

	bool bOnGround = false;
	FVector CurrentGroundNormal = FVector::UpVector;

	bool bIsFirstPersonControlled = false;

public:
	static const uint32 MaxMoveIterationNum;

	static const float IdealGroundDistance;
	static const float MaxGroundDistance;

	static const float WallBumpIntensity;
};
