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

	virtual void UpdateTickRegistration() override;
	virtual void SetUpdatedComponent(USceneComponent* const NewUpdatedComponent) override;
	virtual float GetMaxSpeed() const override;
	virtual FVector ConsumeInputVector() override;

	virtual void RequestPathMove(const FVector& MoveInput) override;

	virtual void OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const FWpPooledObjectData& PooledObjectData) override;
	virtual void OnPooledObjectVisibilityChanged(const bool bIsVisible) override;

	void SetCollisionSize(const float Radius, const float Height);

	float GetMinBrakingDistance() const;

	void AddMovementInput(const float Forward, const float Right);
	void SetIsFirstPersonControlled(const bool bInIsFirstPersonControlled);

private:
	void UpdateSlipping();

	void UpdateWheelRotationAngle(const float DeltaTime, const float Input);

	void UpdateMoveAcceleration(const float DeltaTime, const FVector& InputVector);
	void ApplyInputMoveAcceleration1D(const float Input);
	void ApplyInputMoveAcceleration2D(const FVector& InputVector);
	void ApplyMoveForces();
	void ApplyMoveFrictionLimit();

	void UpdateRotationAcceleration(const float DeltaTime, const FVector& InputVector);
	void ApplyRotationAccelerationLegs(const float DeltaTime);
	void ApplyRotationAccelerationTracks(const float DeltaTime, const FVector& InputVector);
	void ApplyRotationAccelerationWheels(const float DeltaTime);

	FVector CalcNewMoveSpeed(const float DeltaTime) const;
	float CalcNewRotationSpeed(const float DeltaTime) const;

	void Move(const float DeltaTime);

	FVector CalcDeltaLocation(const float DeltaTime, const FVector& InGroundNormal) const;
	FVector CalcInclineUpSpeed(const FVector& InGroundNormal) const;
	FVector CalcInclineDownSpeed(const FVector& InGroundNormal) const;
	FVector CalcInclineSideSpeed(const FVector& InGroundNormal) const;

	void UpdateGround(const float DeltaTime);

	bool StepUp(const float DeltaTime);

	void ProcessWallImpact(const FHitResult& Hit);

	void SweepUnder(FHitResult& OutHit, const float Distance) const;
	void RevertLocationTo(const FVector& OldLocation);

	void UpdateNavVelocity();

	UPROPERTY(VisibleInstanceOnly)
	EWpEntityMovementType MovementType = EWpEntityMovementType::Undefined;

	UPROPERTY(VisibleInstanceOnly)
	float MaxMoveSpeed = 0.0f;
	UPROPERTY(VisibleInstanceOnly)
	float MoveAccelerationCoefficient1 = 0.0f;
	UPROPERTY(VisibleInstanceOnly)
	float MoveAccelerationCoefficient2 = 0.0f;
	UPROPERTY(VisibleInstanceOnly)
	float MaxMoveBrakingDeceleration = 0.0f;
	UPROPERTY(VisibleInstanceOnly)
	float MaxMoveInertiaDeceleration = 0.0f;

	UPROPERTY(VisibleInstanceOnly)
	float MaxRotationSpeed = 0.0f;
	UPROPERTY(VisibleInstanceOnly)
	float MaxRotationAcceleration = 0.0f;
	UPROPERTY(VisibleInstanceOnly)
	float MaxRotationBrakingDeceleration = 0.0f;

	UPROPERTY(VisibleInstanceOnly)
	float MaxWheelRotationAngle = 0.0f;
	UPROPERTY(VisibleInstanceOnly)
	float MaxWheelEdgeRotationSpeed = 0.0f;
	UPROPERTY(VisibleInstanceOnly)
	float MaxWheelCenterRotationSpeed = 0.0f;

	UPROPERTY(VisibleInstanceOnly)
	float MaxInclineAngle = 0.0f;
	UPROPERTY(VisibleInstanceOnly)
	float MaxGroundStaticFriction = 0.0f;

	UPROPERTY(VisibleInstanceOnly)
	float MaxStepUpHeight = 0.0f;

	FVector MoveSpeed = FVector::ZeroVector;
	FVector MoveAcceleration = FVector::ZeroVector;

	float RotationSpeed = 0.0f;
	float RotationAcceleration = 0.0f;

	float WheelRotationAngle = 0.0f;

	bool bOnGround = false;
	FVector GroundNormal = FVector::UpVector;

	bool bIsSlipping = false;

	bool bIsFirstPersonControlled = false;

public:
	static const uint32 MaxMoveIterationNum;

	static const float IdealGroundDistance;
	static const float MaxGroundDistance;

	static const float WallBumpIntensity;

	static const float KineticToStaticGroundFriction;
	static const float MinSlippingLateralSpeed;
	static const float MinSlippingMagicValueScale;
};
