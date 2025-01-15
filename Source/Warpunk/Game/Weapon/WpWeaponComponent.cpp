#include "WpWeaponComponent.h"

#include "Warpunk/Game/Endurance/WpEnduranceComponent.h"
#include "Warpunk/Game/Entity/WpCommander.h"
#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Entity/WpEntityInterface.h"
#include "Warpunk/Game/Entity/WpPooledEntityData.h"
#include "Warpunk/Game/Weapon/WpPooledWeaponData.h"

UWpWeaponComponent::UWpWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicatedByDefault(false);
}

void UWpWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!GetOwner()->HasAuthority())
	{
		SetComponentTickEnabled(false);
	}
}

void UWpWeaponComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* const ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetOwner<IWpEntityInterface>()->GetEnduranceComponent()->GetEntityState() != EWpEntityState::Operational)
	{
		return;
	}

	if (bManualControlOnly)
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

void UWpWeaponComponent::OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
{
	if (bIsAcquired)
	{
		const FWpPooledEntityData& PooledEntityData = static_cast<const FWpPooledEntityData&>(PooledObjectData);
		const TArray<FWpWeaponConfig>& WeaponConfigs = PooledEntityData.EntityConfig->WeaponConfigs;

		for (uint32 Index = 0; Index < WP_ENTITY_MAX_WEAPON; ++Index)
		{
			if (WeaponConfigs.IsValidIndex(Index))
			{
				const FWpWeaponConfig& WeaponConfig = WeaponConfigs[Index];

				FWpWeapon* const Weapon = Weapons[Index].Set(WeaponConfig.Type);
				Weapon->Initialize(GetOwner<IWpEntityInterface>());

				FWpPooledWeaponData PooledWeaponData;
				PooledWeaponData.WeaponConfig = &WeaponConfig;
				Weapon->AcquirePooledObject(PooledWeaponData);
			}
			else
			{
				Weapons[Index].Reset();
			}
		}

		bManualControlOnly = GetOwner()->GetClass()->IsChildOf(AWpCommander::StaticClass());

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

	SetActive(bIsAcquired);
	SetComponentTickEnabled(bIsAcquired);
}

FWpWeapon* UWpWeaponComponent::GetWeapon(const uint32 Index)
{
	return Weapons[Index].Get();
}

const FWpWeapon* UWpWeaponComponent::GetWeapon(const uint32 Index) const
{
	return Weapons[Index].Get();
}
