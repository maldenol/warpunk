#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Order/WpOrderHandler.h"

class AWpUnitAIController;
class UWpProductionComponent;

class FWpPatrolOrderHandler final : public FWpOrderHandler
{
public:
	virtual void Initialize(IWpEntityInterface* const InEntity) override;

	virtual void Reactivate() override;
	virtual void Deactivate() override;

private:
	void MoveToNextLocation();

	void OnMovementCompleted();

	AWpUnitAIController* AIController = nullptr;

	UWpProductionComponent* ProductionComponent = nullptr;
	bool bWasSupporting = true;

	FDelegateHandle OnMovementCompletedDelegateHandle = {};
};
