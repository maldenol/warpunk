#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Pool/WpPooledObjectData.h"
#include "Warpunk/Game/Weapon/WpProjectileConfig.h"

class FWpProjectileWeapon;

struct FWpPooledProjectileData final : FWpPooledObjectData
{
	FWpProjectileConfig ProjectileConfig = {};
	FVector InitialVelocity = FVector::ZeroVector;
	float Damage = 0.0f;
	FWpProjectileWeapon* Weapon = nullptr;
};
