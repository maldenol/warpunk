#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Weapon/WpWeapon.h"

class IWpEntityInterface;

class FWpProjectileWeapon final : public FWpWeapon
{
public:
	virtual void OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData) override;

	virtual bool CanReach(const FVector& Location) const override;

	void OnProjectileHit(IWpEntityInterface* const Entity);

private:
	virtual void Fire(const FVector& Location) override;

	float CalcElevationAngle(const FVector& ProjectileLocation, const FVector& TargetLocation) const;
	void CalcAzimuthVectorAndProjectileLocation(const FVector& TargetLocation, FVector& OutAzimuthVector, FVector& OutProjectileLocation) const;
	static void CalcLocationDelta(const FVector& ProjectileLocation, const FVector& TargetLocation, float& OutHorizontalDelta, float& OutVerticalDelta);
	float CalcDiscriminant(const float HorizontalDelta, const float VerticalDelta, const float GravityZ) const;
	float CalcDiscriminantMinRange(const float HorizontalDelta, const float VerticalDelta, const float GravityZ) const;
	float GetGravityZ() const;

	float ProjectileRadius = 0.0f;
	UStaticMesh* ProjectileMesh = nullptr;
	bool bHighAngle = false;

	float ProjectileSpeed = 0.0f;
	float ProjectileSpeed2 = 0.0f;
	float ProjectileSpeed4 = 0.0f;

	float ProjectileSpeedMinRange2 = 0.0f;
	float ProjectileSpeedMinRange4 = 0.0f;
};
