#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Weapon/WpWeapon.h"

class UWpEntityComponent;

class FWpProjectileWeapon final : public FWpWeapon
{
public:
	virtual void OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const void* const CustomData) override;

	void OnProjectileHit(UWpEntityComponent* const TargetEntityComponent);

private:
	virtual void Fire(const FVector& TargetLocation) override;

	float ProjectileRadius = 0.0f;
	UStaticMesh* ProjectileMesh = nullptr;
	float ProjectileSpeed = 0.0f;
};
