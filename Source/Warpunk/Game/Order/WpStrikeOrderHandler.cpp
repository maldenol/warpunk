#include "WpStrikeOrderHandler.h"

#include "Warpunk/Game/Entity/WpEntity.h"
#include "Warpunk/Game/Weapon/WpWeapon.h"
#include "Warpunk/Game/Weapon/WpWeaponComponent.h"

void FWpStrikeOrderHandler::Reactivate()
{
	FWpOrderHandler::Reactivate();

	WeaponComponent = GetEntity()->GetWeaponComponent();
	ensureAlways(WeaponComponent->IsActive());

	for (uint32 Index = 0; Index < WP_ENTITY_MAX_WEAPON; ++Index)
	{
		WaitingWeapons[Index] = (WeaponComponent->GetWeapon(Index) != nullptr);
	}
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

		if (GetOrder().TargetEntity)
		{
			if (Weapon->TryFire(GetOrder().TargetEntity))
			{
				WaitingWeapons[Index] = false;
			}
		}
		else
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
