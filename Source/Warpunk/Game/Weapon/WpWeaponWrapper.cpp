#include "WpWeaponWrapper.h"

FWpWeapon* FWpWeaponWrapper::Set(const EWpWeaponType InWeaponType)
{
	FWpWeapon* Weapon = nullptr;

	switch (InWeaponType)
	{
	case EWpWeaponType::HitScan:
		WeaponVariant.Emplace<FWpHitScanWeapon>();
		Weapon = &WeaponVariant.Get<FWpHitScanWeapon>();
		break;
	case EWpWeaponType::Projectile:
		WeaponVariant.Emplace<FWpProjectileWeapon>();
		Weapon = &WeaponVariant.Get<FWpProjectileWeapon>();
		break;
	default:
		ensureAlways(false);
		WeaponVariant.Emplace<std::monostate>();
		break;
	}

	WeaponType = Weapon ? InWeaponType : EWpWeaponType::Undefined;

	return Weapon;
}

FWpWeapon* FWpWeaponWrapper::Get()
{
	FWpWeapon* Weapon = nullptr;

	switch (WeaponType)
	{
	case EWpWeaponType::HitScan:
		Weapon = &WeaponVariant.Get<FWpHitScanWeapon>();
		break;
	case EWpWeaponType::Projectile:
		Weapon = &WeaponVariant.Get<FWpProjectileWeapon>();
		break;
	default:
		break;
	}

	return Weapon;
}

const FWpWeapon* FWpWeaponWrapper::Get() const
{
	return const_cast<FWpWeaponWrapper*>(this)->Get();
}

void FWpWeaponWrapper::Reset()
{
	WeaponVariant.Emplace<std::monostate>();
	WeaponType = EWpWeaponType::Undefined;
}
