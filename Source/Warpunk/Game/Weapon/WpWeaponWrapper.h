#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Weapon/WpHitScanWeapon.h"
#include "Warpunk/Game/Weapon/WpProjectileWeapon.h"

#include "WpWeaponWrapper.generated.h"

UENUM(NotBlueprintable, NotBlueprintType)
enum class EWpWeaponType : uint8
{
	Undefined UMETA(Hidden),
	HitScan,
	Projectile,
};

class FWpWeaponWrapper final
{
	using FWpWeaponVariant = TVariant
	<
		std::monostate,
		FWpHitScanWeapon,
		FWpProjectileWeapon
	>;

public:
	FWpWeapon* Set(const EWpWeaponType InWeaponType);
	FWpWeapon* Get();
	const FWpWeapon* Get() const;
	void Reset();

private:
	FWpWeaponVariant WeaponVariant = {};
	EWpWeaponType WeaponType = EWpWeaponType::Undefined;
};
