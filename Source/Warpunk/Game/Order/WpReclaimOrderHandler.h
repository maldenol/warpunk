#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Order/WpOrderHandler.h"

class UWpProductionComponent;

class AAIController;

class FWpReclaimOrderHandler final : public FWpOrderHandler
{
public:
	virtual void Initialize(IWpEntityInterface* const InEntity) override;

	virtual void Reactivate() override;
	virtual void Tick() override;
	virtual void Deactivate() override;

private:
	void OnReclaimFinished();

	AAIController* AIController = nullptr;

	UWpProductionComponent* ProductionComponent = nullptr;

	FDelegateHandle OnReclaimFinishedDelegateHandle = {};
};
