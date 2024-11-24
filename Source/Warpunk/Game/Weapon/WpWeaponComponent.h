#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"
#include "Warpunk/Game/Weapon/WpWeapon.h"
#include "Warpunk/Game/Weapon/WpWeaponWrapper.h"

#include "Components/ActorComponent.h"

#include "WpWeaponComponent.generated.h"

#define WP_ENTITY_MAX_WEAPON 4

struct FWpWeaponConfig;

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class UWpWeaponComponent final : public UActorComponent, public IWpPooledObjectInterface
{
	GENERATED_BODY()

public:
	UWpWeaponComponent();

	virtual void TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* const ThisTickFunction) override;

	virtual void OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const FWpPooledObjectData& PooledObjectData) override;

	FWpWeapon* GetWeapon(const uint32 Index);
	const FWpWeapon* GetWeapon(const uint32 Index) const;

private:
	TStaticArray<FWpWeaponWrapper, WP_ENTITY_MAX_WEAPON> Weapons = {};

	bool bManualControlOnly = false;
};
