#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Weapon/WpWeapon.h"

class FWpHitScanWeapon final : public FWpWeapon
{
public:
	virtual bool CanReach(const FVector& TargetLocation) const override;

private:
	virtual void Fire(const FVector& TargetLocation) override;

	FHitResult HitScan(const FVector& TargetLocation) const;
};
