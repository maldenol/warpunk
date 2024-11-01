#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Order/WpOrderHandler.h"

class FWpProduction;

class AAIController;

class FWpReclaimOrderHandler final : public FWpOrderHandler
{
public:
	virtual void SetOwnerEntityComponent(UWpEntityComponent* const InOwnerEntityComponent) override;

	virtual void Reactivate() override;
	virtual void Tick() override;
	virtual void Deactivate() override;

private:
	void OnReclaimFinished();

	AAIController* AIController = nullptr;

	FWpProduction* Production = nullptr;

	FDelegateHandle OnReclaimFinishedDelegateHandle = {};
};
