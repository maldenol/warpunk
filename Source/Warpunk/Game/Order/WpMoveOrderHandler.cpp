#include "WpMoveOrderHandler.h"

#include "Warpunk/Game/Entity/WpEntityInterface.h"
#include "Warpunk/Game/Navigation/WpUnitAIController.h"

void FWpMoveOrderHandler::Initialize(IWpEntityInterface* const InEntity)
{
	FWpOrderHandler::Initialize(InEntity);

	AIController = Cast<APawn>(GetEntity())->GetController<AWpUnitAIController>();
	ensureAlways(AIController);
}

void FWpMoveOrderHandler::Reactivate()
{
	FWpOrderHandler::Reactivate();

	if (OnMovementCompletedDelegateHandle.IsValid())
	{
		AIController->OnMovementCompleted().Remove(OnMovementCompletedDelegateHandle);
	}

	AIController->MoveToLocation(GetOrder().TargetLocation, 5.0f, false, true, false, false, nullptr, false);

	OnMovementCompletedDelegateHandle = AIController->OnMovementCompleted().AddRaw(this, &FWpMoveOrderHandler::OnMovementCompleted);
}

void FWpMoveOrderHandler::Deactivate()
{
	if (AIController)
	{
		AIController->OnMovementCompleted().Remove(OnMovementCompletedDelegateHandle);
		AIController->StopMovement();
	}
	OnMovementCompletedDelegateHandle.Reset();

	FWpOrderHandler::Deactivate();
}

void FWpMoveOrderHandler::OnMovementCompleted()
{
	Finish();
}
