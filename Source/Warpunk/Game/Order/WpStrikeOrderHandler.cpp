#include "WpStrikeOrderHandler.h"

#include "Warpunk/Game/EntityComponent/WpEntityComponent.h"
#include "Warpunk/Game/Weapon/WpWeapon.h"
#include "Warpunk/Game/Weapon/WpWeaponArray.h"

void FWpStrikeOrderHandler::Reactivate()
{
	FWpOrderHandler::Reactivate();

	WeaponArray = &WP_AS_WEAPON_ARRAY(GetOwnerEntityComponent());

	for (uint32 Index = 0; Index < WP_ENTITY_MAX_WEAPON; ++Index)
	{
		WaitingWeapons[Index] = WeaponArray->GetWeapon(Index) != nullptr;
	}

	const bool bHasAtLeastOneWeapon = WeaponArray->GetWeapon(0) != nullptr;
	ensureAlways(bHasAtLeastOneWeapon);
}

void FWpStrikeOrderHandler::Tick()
{
	FWpOrderHandler::Tick();

	bool bNoWaitingWeapons = true;

	for (uint32 Index = 0; Index < WP_ENTITY_MAX_WEAPON; ++Index)
	{
		if (!WaitingWeapons[Index])
		{
			continue;
		}

		bNoWaitingWeapons = false;

		FWpWeapon* const Weapon = WeaponArray->GetWeapon(Index);

		if (IsValid(GetOrder().TargetEntityComponent))
		{
			if (Weapon->TryFire(GetOrder().TargetEntityComponent))
			{
				WaitingWeapons[Index] = false;
			}
		}
		else if (!GetOrder().TargetLocation.IsNearlyZero())
		{
			if (Weapon->TryFire(GetOrder().TargetLocation))
			{
				WaitingWeapons[Index] = false;
			}
		}
	}

	if (bNoWaitingWeapons)
	{
		Finish();
	}
}
