#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"

#include "GameFramework/ProjectileMovementComponent.h"

#include "WpProjectileMovementComponent.generated.h"

USTRUCT(NotBlueprintable, NotBlueprintType)
struct FWpProjectileMovementComponentPredictionData final
{
	GENERATED_BODY()

	UPROPERTY(VisibleInstanceOnly)
	bool bIsAcquired = false;

	UPROPERTY(VisibleInstanceOnly)
	double ServerTime = 0.0;

	UPROPERTY(VisibleInstanceOnly)
	FVector UpdatedComponentLocation = FVector::ZeroVector;
	UPROPERTY(VisibleInstanceOnly)
	FRotator UpdatedComponentRotation = FRotator::ZeroRotator;

	UPROPERTY(VisibleInstanceOnly)
	FVector Velocity = FVector::ZeroVector;
};

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class UWpProjectileMovementComponent final : public UProjectileMovementComponent, public IWpPooledObjectInterface
{
	GENERATED_BODY()

public:
	UWpProjectileMovementComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void StopSimulating(const FHitResult& HitResult) override;

	virtual void OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData) override;

private:
	void UpdatePredictionData();

	UFUNCTION()
	void OnRep_PredictionData();

	UPROPERTY(ReplicatedUsing=OnRep_PredictionData)
	FWpProjectileMovementComponentPredictionData PredictionData = {};
};
