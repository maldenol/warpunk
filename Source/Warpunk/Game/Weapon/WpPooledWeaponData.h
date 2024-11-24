#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Pool/WpPooledObjectData.h"

struct FWpWeaponConfig;

struct FWpPooledWeaponData final : FWpPooledObjectData
{
	const FWpWeaponConfig* WeaponConfig = nullptr;
};
