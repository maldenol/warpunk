#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Order/WpOrderHandler.h"

class AWpEntityAIController;
class UWpProductionComponent;

class FWpPatrolOrderHandler final : public FWpOrderHandler
{
public:
	virtual void Initialize(AWpEntity* const InEntity) override;

	virtual void Reactivate() override;
	virtual void Deactivate() override;

private:
	void MoveToNextLocation();

	void OnMovementCompleted();

	AWpEntityAIController* AIController = nullptr;

	UWpProductionComponent* ProductionComponent = nullptr;
	bool bWasSupporting = true;

	FDelegateHandle OnMovementCompletedDelegateHandle = {};
};
