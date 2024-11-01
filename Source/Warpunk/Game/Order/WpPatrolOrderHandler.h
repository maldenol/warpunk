#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Order/WpOrderHandler.h"

class AWpUnitAIController;
class FWpProduction;

class FWpPatrolOrderHandler final : public FWpOrderHandler
{
public:
	virtual void SetOwnerEntityComponent(UWpEntityComponent* const InOwnerEntityComponent) override;

	virtual void Reactivate() override;
	virtual void Deactivate() override;

private:
	void MoveToNextLocation();

	void OnMovementCompleted();

	AWpUnitAIController* AIController = nullptr;

	FWpProduction* Production = nullptr;
	bool bWasSupporting = true;

	FDelegateHandle OnMovementCompletedDelegateHandle = {};
};
