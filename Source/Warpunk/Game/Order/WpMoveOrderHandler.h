#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Order/WpOrderHandler.h"

class AWpUnitAIController;

class FWpMoveOrderHandler final : public FWpOrderHandler
{
public:
	virtual void SetOwnerEntityComponent(UWpEntityComponent* const InOwnerEntityComponent) override;

	virtual void Reactivate() override;
	virtual void Deactivate() override;

private:
	void OnMovementCompleted();

	AWpUnitAIController* AIController = nullptr;

	FDelegateHandle OnMovementCompletedDelegateHandle = {};
};
