#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Order/WpOrderHandler.h"

class AWpEntityAIController;

class FWpMoveOrderHandler final : public FWpOrderHandler
{
public:
	virtual void Initialize(AWpEntity* const InEntity) override;

	virtual void Reactivate() override;
	virtual void Deactivate() override;

private:
	void OnMovementCompleted();

	AWpEntityAIController* AIController = nullptr;

	FDelegateHandle OnMovementCompletedDelegateHandle = {};
};
