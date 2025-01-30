#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Order/WpOrderHandler.h"

class UWpProductionComponent;

class AAIController;

class FWpProduceBuildingOrderHandler final : public FWpOrderHandler
{
public:
	virtual void Initialize(AWpEntity* const InEntity) override;

	virtual void Reactivate() override;
	virtual void Deactivate() override;

private:
	void OnRepairFinished(AWpEntity* const InEntity);

	AAIController* AIController = nullptr;

	UWpProductionComponent* ProductionComponent = nullptr;

	FDelegateHandle OnRepairFinishedDelegateHandle = {};
};
