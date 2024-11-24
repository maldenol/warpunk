#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Order/WpOrderHandler.h"

class FWpWeapon;
class UWpWeaponComponent;

class AAIController;

class FWpAttackOrderHandler final : public FWpOrderHandler
{
public:
	virtual void Initialize(IWpEntityInterface* const InEntity) override;

	virtual void Reactivate() override;
	virtual void Tick() override;
	virtual void Deactivate() override;

private:
	AAIController* AIController = nullptr;

	UWpWeaponComponent* WeaponComponent = nullptr;

	const FWpWeapon* ShortestRangeWeapon = nullptr;
};
