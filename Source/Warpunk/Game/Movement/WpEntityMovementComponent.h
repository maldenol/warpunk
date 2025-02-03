#pragma once

#include "CoreMinimal.h"

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

	virtual void OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const FWpPooledObjectData& PooledObjectData) override;
	virtual void OnPooledObjectVisibilityChanged(const bool bIsVisible) override;

	void AddMovementInput(const float Forward, const float Right);

	void SetIsFirstPersonControlled(const bool bIsFirstPersonControlled);

private:
	void UpdateAcceleration(const float DeltaTime, const FVector& InputVector);
	void UpdateRotationAcceleration(const float DeltaTime, const FVector& InputVector);

	void UpdateSpeed(const float DeltaTime);
	void UpdateRotationSpeed(const float DeltaTime);

	FVector CalcNewSpeed(const float DeltaTime) const;
	float CalcNewRotationSpeed(const float DeltaTime) const;

	void Rotate(const float DeltaTime);
	void Move(const float DeltaTime);

	void UpdateGround(const float DeltaTime);

	bool StepUp(const float DeltaTime);

	void ProcessWallImpact();

	FVector CalcDeltaLocation(const float DeltaTime, const FVector& GroundNormal) const;

	FVector CalcInclineUpSpeed(const FVector& GroundNormal) const;
	FVector CalcInclineDownSpeed(const FVector& GroundNormal) const;
	FVector CalcInclineSideSpeed(const FVector& GroundNormal) const;

	void RevertLocationTo(const FVector& OldLocation);

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
	float MaxInclineAngle = 0.0f;
	UPROPERTY(VisibleInstanceOnly)
	float MaxStepUpHeight = 0.0f;

	FVector Speed = FVector::ZeroVector;
	FVector Acceleration = FVector::ZeroVector;

	float RotationSpeed = 0.0f;
	float RotationAcceleration = 0.0f;

	bool bOnGround = true;
	FVector CurrentGroundNormal = FVector::UpVector;

public:
	static const float IdealGroundDistance;
	static const float MaxGroundDistance;
};
