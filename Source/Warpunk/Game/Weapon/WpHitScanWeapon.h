#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Weapon/WpWeapon.h"

class FWpHitScanWeapon final : public FWpWeapon
{
public:
	virtual bool CanReach(const FVector& Location) const override;

private:
	virtual void Fire(const FVector& Location) override;

	FHitResult HitScan(const FVector& Location) const;
};
