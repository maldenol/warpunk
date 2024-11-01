#include "WpWeaponArray.h"

#include "Warpunk/Game/Data/WpEntityConfig.h"
#include "Warpunk/Game/Data/WpPooledEntityData.h"

void FWpWeaponArray::OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const void* const CustomData)
{
	if (bIsAcquired)
	{
		const FWpPooledEntityData* const PooledEntityData = static_cast<const FWpPooledEntityData*>(CustomData);
		const TArray<FWpWeaponConfig>& WeaponConfigs = PooledEntityData->EntityConfig->WeaponConfigs;

		for (uint32 Index = 0; Index < WP_ENTITY_MAX_WEAPON; ++Index)
		{
			if (WeaponConfigs.IsValidIndex(Index))
			{
				const FWpWeaponConfig& WeaponConfig = WeaponConfigs[Index];
				FWpWeapon* const Weapon = Weapons[Index].Set(WeaponConfig.Type);
				Weapon->Initialize(OwnerEntityComponent);
				Weapon->AcquirePooledObject(&WeaponConfig);
			}
			else
			{
				Weapons[Index].Reset();
			}
		}

		bIsAcquired = !WeaponConfigs.IsEmpty();
	}

	if (!bIsAcquired)
	{
		for (uint32 Index = 0; Index < WP_ENTITY_MAX_WEAPON; ++Index)
		{
			if (FWpWeapon* const Weapon = Weapons[Index].Get())
			{
				Weapon->ReleasePooledObject();
				Weapons[Index].Reset();
			}
			else
			{
				break;
			}
		}
	}

	bIsActive = bIsAcquired;
}

void FWpWeaponArray::Initialize(UWpEntityComponent* const InOwnerEntityComponent)
{
	OwnerEntityComponent = InOwnerEntityComponent;
}

bool FWpWeaponArray::IsActive() const
{
	return bIsActive;
}

void FWpWeaponArray::Tick(const float DeltaTime)
{
	if (!bIsActive)
	{
		return;
	}

	for (uint32 Index = 0; Index < WP_ENTITY_MAX_WEAPON; ++Index)
	{
		if (FWpWeapon* const Weapon = Weapons[Index].Get())
		{
			Weapon->Tick(DeltaTime);
		}
		else
		{
			break;
		}
	}
}

FWpWeapon* FWpWeaponArray::GetWeapon(const uint32 Index)
{
	return Weapons[Index].Get();
}

const FWpWeapon* FWpWeaponArray::GetWeapon(const uint32 Index) const
{
	return Weapons[Index].Get();
}
