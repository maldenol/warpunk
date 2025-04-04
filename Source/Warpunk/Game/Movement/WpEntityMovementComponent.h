#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Movement/WpEntityMovementType.h"
#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"

#include "GameFramework/PawnMovementComponent.h"

#include "WpEntityMovementComponent.generated.h"

USTRUCT(NotBlueprintable, NotBlueprintType)
struct FWpEntityMovementComponentPredictionData final
{
	GENERATED_BODY()

	UPROPERTY(VisibleInstanceOnly)
	double ServerTime = 0.0;

	UPROPERTY(VisibleInstanceOnly)
	FVector UpdatedComponentLocation = FVector::ZeroVector;
	UPROPERTY(VisibleInstanceOnly)
	FRotator UpdatedComponentRotation = FRotator::ZeroRotator;

	UPROPERTY(VisibleInstanceOnly)
	FVector InputVector = FVector::ZeroVector;
	UPROPERTY(VisibleInstanceOnly)
	FVector MoveSpeed = FVector::ZeroVector;
	UPROPERTY(VisibleInstanceOnly)
	float RotationSpeed = 0.0f;
	UPROPERTY(VisibleInstanceOnly)
	float WheelRotationAngle = 0.0f;

	UPROPERTY(VisibleInstanceOnly)
	FVector TargetInterpolatedComponentLocation = FVector::ZeroVector;
	UPROPERTY(VisibleInstanceOnly)
	FRotator TargetInterpolatedComponentRotation = FRotator::ZeroRotator;
};

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class UWpEntityMovementComponent final : public UPawnMovementComponent, public IWpPooledObjectInterface
{
	GENERATED_BODY()

public:
	UWpEntityMovementComponent();

	virtual void TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* const ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void SetUpdatedComponent(USceneComponent* const NewUpdatedComponent) override;
	virtual float GetMaxSpeed() const override;

	virtual void RequestPathMove(const FVector& MoveInput) override;

	virtual void OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const FWpPooledObjectData& PooledObjectData) override;
	virtual void OnPooledObjectVisibilityChanged(const bool bIsVisible) override;

	void SetInterpolatedComponent(USceneComponent* const InInterpolatedComponent);

	void SetCollisionSize(const float Radius, const float Height);

	float GetMinBrakingDistance() const;

	void AddMovementInput(const float Forward, const float Right);
	void SetIsFirstPersonControlled(const bool bInIsFirstPersonControlled);

	void CacheInitialInterpolatedComponentOffset();

private:
	void TickMovement(const float DeltaTime);

	FVector GetInputVector();

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

	void UpdateGround();

	void ProcessWallImpact(const FHitResult& Hit);
	bool ProcessEntityImpact(const FHitResult& Hit);
	void ReceiveEntityImpact(const FHitResult& Hit, const UWpEntityMovementComponent* const OtherMovementComponent, const FVector& OtherMoveSpeed);

	bool StepUp(const float DeltaTime, float& OutSpentTime);

	void SweepCapsule(FHitResult& OutHit, const FVector& StartLocation, const FVector& EndLocation) const;
	void SweepBottomSphere(FHitResult& OutHit, const FVector& StartLocation, const FVector& EndLocation) const;
	void LineTrace(FHitResult& OutHit, const FVector& StartLocation, const FVector& EndLocation) const;

	void AddInterpolatedComponentOffset(const FVector& DeltaLocation, const FRotator& DeltaRotation);
	void UpdateTargetInterpolatedComponentOffset();
	void UpdateInterpolatedComponentOffset(const float DeltaTime);

	void UpdateNavVelocity();

	void UpdatePredictionData(const FVector& InputVector);

	UFUNCTION()
	void OnRep_PredictionData();

	EWpEntityMovementType MovementType = EWpEntityMovementType::Undefined;

	float MaxMoveSpeed = 0.0f;
	float MoveAccelerationCoefficient1 = 0.0f;
	float MoveAccelerationCoefficient2 = 0.0f;
	float MaxMoveBrakingDeceleration = 0.0f;
	float MaxMoveInertiaDeceleration = 0.0f;
	float MaxGroundStaticFriction = 0.0f;

	float MaxRotationSpeed = 0.0f;
	float MaxRotationAcceleration = 0.0f;
	float MaxRotationBrakingDeceleration = 0.0f;

	float MaxWheelRotationAngle = 0.0f;
	float MaxWheelEdgeRotationSpeed = 0.0f;
	float MaxWheelCenterRotationSpeed = 0.0f;

	float Mass = 0.0f;
	float MaxInclineAngleCos = 0.0f;
	float MaxStepUpHeight = 0.0f;

	UPROPERTY(VisibleInstanceOnly)
	FVector LastInputVector = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly)
	FVector MoveSpeed = FVector::ZeroVector;
	UPROPERTY(VisibleInstanceOnly)
	FVector MoveAcceleration = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly)
	float RotationSpeed = 0.0f;
	UPROPERTY(VisibleInstanceOnly)
	float RotationAcceleration = 0.0f;

	UPROPERTY(VisibleInstanceOnly)
	float WheelRotationAngle = 0.0f;

	UPROPERTY(VisibleInstanceOnly)
	bool bOnGround = false;
	UPROPERTY(VisibleInstanceOnly)
	FVector GroundNormal = FVector::UpVector;

	UPROPERTY(VisibleInstanceOnly)
	bool bIsSlipping = false;

	bool bIsFirstPersonControlled = false;

	USceneComponent* InterpolatedComponent = nullptr;

	FTransform InitialInterpolatedComponentTransform = FTransform::Identity;
	UPROPERTY(VisibleInstanceOnly)
	FVector TargetInterpolatedComponentLocation = FVector::ZeroVector;
	UPROPERTY(VisibleInstanceOnly)
	FRotator TargetInterpolatedComponentRotation = FRotator::ZeroRotator;
	UPROPERTY(VisibleInstanceOnly)
	FVector DeltaInterpolatedComponentLocation = FVector::ZeroVector;
	UPROPERTY(VisibleInstanceOnly)
	FRotator DeltaInterpolatedComponentRotation = FRotator::ZeroRotator;

	UPROPERTY(ReplicatedUsing=OnRep_PredictionData)
	FWpEntityMovementComponentPredictionData PredictionData = {};

public:
	static const float MinTickTime;
	static const uint32 MaxMoveIterationNum;

	static const float IdealGroundDistance;
	static const float MaxGroundDistance;

	static const float WallImpactElasticity;
	static const float EntityImpactElasticity;
	static const float EntityImpactReturnDistance;
	static const float EntityImpactLateralSpeedReduction;

	static const float KineticToStaticGroundFriction;
	static const float MinSlippingLateralSpeed;
	static const float MinSlippingMagicValueScale;

	static const float InterpolatedComponentSmoothingTime;
	static const float InterpolatedComponentSnapDeltaLocation2;
	static const float InterpolatedComponentSnapDeltaRotation;
	static const float InterpolatedComponentNoSmoothDistance2;
};
