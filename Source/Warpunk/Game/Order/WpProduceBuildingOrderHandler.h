#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Order/WpOrderHandler.h"

class FWpProduction;

class AAIController;

class FWpProduceBuildingOrderHandler final : public FWpOrderHandler
{
public:
	virtual void SetOwnerEntityComponent(UWpEntityComponent* const InOwnerEntityComponent) override;

	virtual void Reactivate() override;
	virtual void Deactivate() override;

private:
	void OnRepairFinished(UWpEntityComponent* const EntityComponent);

	AAIController* AIController = nullptr;

	FWpProduction* Production = nullptr;

	FDelegateHandle OnRepairFinishedDelegateHandle = {};
};
