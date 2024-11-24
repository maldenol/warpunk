#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Weapon/WpWeapon.h"

class IWpEntityInterface;

class FWpProjectileWeapon final : public FWpWeapon
{
public:
	virtual void OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData) override;

	void OnProjectileHit(IWpEntityInterface* const Entity);

private:
	virtual void Fire(const FVector& TargetLocation) override;

	float ProjectileRadius = 0.0f;
	UStaticMesh* ProjectileMesh = nullptr;
	float ProjectileSpeed = 0.0f;
};
