#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Pool/WpPooledObjectData.h"
#include "Warpunk/Game/Weapon/WpProjectileConfig.h"

struct FWpWeaponConfig;

struct FWpPooledWeaponData final : FWpPooledObjectData
{
	const FWpWeaponConfig* WeaponConfig = nullptr;
	FWpProjectileConfig ProjectileConfig = {};
};
