#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Order/WpOrderHandler.h"
#include "Warpunk/Game/Weapon/WpWeaponComponent.h"

class FWpStrikeOrderHandler final : public FWpOrderHandler
{
public:
	virtual void Reactivate() override;
	virtual void Tick(const float CurrentTime) override;

private:
	UWpWeaponComponent* WeaponComponent = nullptr;

	TStaticArray<bool, WP_ENTITY_MAX_WEAPON> WaitingWeapons = {};
};
