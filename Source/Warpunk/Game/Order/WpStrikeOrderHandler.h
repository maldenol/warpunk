#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Order/WpOrderHandler.h"
#include "Warpunk/Game/Weapon/WpWeaponArray.h"

class FWpStrikeOrderHandler final : public FWpOrderHandler
{
public:
	virtual void Reactivate() override;
	virtual void Tick() override;

private:
	FWpWeaponArray* WeaponArray = nullptr;

	TStaticArray<bool, WP_ENTITY_MAX_WEAPON> WaitingWeapons = {};
};
