#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Order/WpOrderHandler.h"

class UWpProductionComponent;

class AAIController;

class FWpRepairOrderHandler final : public FWpOrderHandler
{
public:
	FWpRepairOrderHandler();

	virtual void Initialize(IWpEntityInterface* const InEntity) override;

	virtual void Reactivate() override;
	virtual void Tick(const float CurrentTime) override;
	virtual void Deactivate() override;

private:
	void OnRepairFinished(IWpEntityInterface* const InEntity);

	AAIController* AIController = nullptr;

	UWpProductionComponent* ProductionComponent = nullptr;

	FDelegateHandle OnRepairFinishedDelegateHandle = {};
};
