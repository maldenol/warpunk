#include "WpStrikeOrderHandler.h"

#include "Warpunk/Game/Entity/WpEntityInterface.h"
#include "Warpunk/Game/Weapon/WpWeapon.h"
#include "Warpunk/Game/Weapon/WpWeaponComponent.h"

void FWpStrikeOrderHandler::Reactivate()
{
	FWpOrderHandler::Reactivate();

	WeaponComponent = GetEntity()->GetWeaponComponent();

	for (uint32 Index = 0; Index < WP_ENTITY_MAX_WEAPON; ++Index)
	{
		WaitingWeapons[Index] = WeaponComponent->GetWeapon(Index) != nullptr;
	}

	const bool bHasAtLeastOneWeapon = WeaponComponent->GetWeapon(0) != nullptr;
	ensureAlways(bHasAtLeastOneWeapon);
}

void FWpStrikeOrderHandler::Tick(const float CurrentTime)
{
	FWpOrderHandler::Tick(CurrentTime);

	bool bNoWaitingWeapons = true;

	for (uint32 Index = 0; Index < WP_ENTITY_MAX_WEAPON; ++Index)
	{
		if (!WaitingWeapons[Index])
		{
			continue;
		}

		bNoWaitingWeapons = false;

		FWpWeapon* const Weapon = WeaponComponent->GetWeapon(Index);

		if (GetOrder().TargetEntityActor)
		{
			if (Weapon->TryFire(GetOrder().GetTargetEntity()))
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
