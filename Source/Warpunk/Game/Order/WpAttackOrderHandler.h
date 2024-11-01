#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Order/WpOrderHandler.h"

class FWpWeapon;
class FWpWeaponArray;

class AAIController;

class FWpAttackOrderHandler final : public FWpOrderHandler
{
public:
	virtual void SetOwnerEntityComponent(UWpEntityComponent* const InOwnerEntityComponent) override;

	virtual void Reactivate() override;
	virtual void Tick() override;
	virtual void Deactivate() override;

private:
	AAIController* AIController = nullptr;

	FWpWeaponArray* WeaponArray = nullptr;

	const FWpWeapon* ShortestRangeWeapon = nullptr;
};
