#include "WpRepairOrderHandler.h"

#include "Warpunk/Game/EntityComponent/WpEntityComponent.h"
#include "Warpunk/Game/Production/WpProduction.h"

#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"

void FWpRepairOrderHandler::SetOwnerEntityComponent(UWpEntityComponent* const InOwnerEntityComponent)
{
	FWpOrderHandler::SetOwnerEntityComponent(InOwnerEntityComponent);

	AIController = GetOwnerEntityComponent()->GetOwner<APawn>()->GetController<AAIController>();
	ensureAlways(AIController);

	Production = &WP_AS_PRODUCTION(GetOwnerEntityComponent());
	ensureAlways(Production->IsActive());
}

void FWpRepairOrderHandler::Reactivate()
{
	FWpOrderHandler::Reactivate();

	if (!GetOrder().TargetEntityComponent)
	{
		Finish();
		return;
	}

	Production->SetPriorityTarget(GetOrder().TargetEntityComponent);

	if (OnRepairFinishedDelegateHandle.IsValid())
	{
		Production->OnRepairFinished.Remove(OnRepairFinishedDelegateHandle);
	}
	OnRepairFinishedDelegateHandle = Production->OnRepairFinished.AddRaw(this, &FWpRepairOrderHandler::OnRepairFinished);
}

void FWpRepairOrderHandler::Tick()
{
	FWpOrderHandler::Tick();

	const bool bCanReach = Production->CanReach(GetOrder().TargetEntityComponent);
	const bool bIsMoving = AIController->GetMoveStatus() != EPathFollowingStatus::Type::Idle;
	if (!bCanReach && !bIsMoving)
	{
		AIController->MoveToActor(GetOrder().TargetEntityComponent->GetOwner(), Production->GetRange(), false, true, false, nullptr, false);
	}
}

void FWpRepairOrderHandler::Deactivate()
{
	Production->OnRepairFinished.Remove(OnRepairFinishedDelegateHandle);
	OnRepairFinishedDelegateHandle.Reset();
	Production->Stop();

	if (IsValid(AIController))
	{
		AIController->StopMovement();
	}

	FWpOrderHandler::Deactivate();
}

void FWpRepairOrderHandler::OnRepairFinished(UWpEntityComponent* const EntityComponent)
{
	Finish();
}
