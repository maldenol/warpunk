#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"

#include "GameFramework/ProjectileMovementComponent.h"

#include "WpProjectileMovementComponent.generated.h"

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class UWpProjectileMovementComponent final : public UProjectileMovementComponent, public IWpPooledObjectInterface
{
	GENERATED_BODY()

public:
	UWpProjectileMovementComponent();

	virtual void BeginPlay() override;

	virtual void OnRep_IsActive() override;

	virtual void OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData) override;
	virtual void OnPooledObjectVisibilityChanged(const bool bIsVisible) override;
};
