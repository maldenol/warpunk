#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"
#include "Warpunk/Game/Weapon/WpWeapon.h"
#include "Warpunk/Game/Weapon/WpWeaponWrapper.h"

#define WP_ENTITY_MAX_WEAPON 4

struct FWpWeaponConfig;

class FWpWeaponArray final : public IWpPooledObjectInterface
{
public:
	virtual void OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const void* const CustomData) override;

	void Initialize(UWpEntityComponent* const InOwnerEntityComponent);

	bool IsActive() const;

	void Tick(const float DeltaTime);

	FWpWeapon* GetWeapon(const uint32 Index);
	const FWpWeapon* GetWeapon(const uint32 Index) const;

private:
	UWpEntityComponent* OwnerEntityComponent = nullptr;
	bool bIsActive = false;

	TStaticArray<FWpWeaponWrapper, WP_ENTITY_MAX_WEAPON> Weapons = {};
};
