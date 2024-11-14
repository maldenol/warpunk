#include "WpReclaimOrderHandler.h"

#include "Warpunk/Game/EntityComponent/WpEntityComponent.h"
#include "Warpunk/Game/Production/WpProduction.h"

#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"

void FWpReclaimOrderHandler::SetOwnerEntityComponent(UWpEntityComponent* const InOwnerEntityComponent)
{
	FWpOrderHandler::SetOwnerEntityComponent(InOwnerEntityComponent);

	AIController = GetOwnerEntityComponent()->GetOwner<APawn>()->GetController<AAIController>();
	ensureAlways(AIController);

	Production = &WP_AS_PRODUCTION(GetOwnerEntityComponent());
	ensureAlways(Production->IsActive());
}

void FWpReclaimOrderHandler::Reactivate()
{
	FWpOrderHandler::Reactivate();

	if (!GetOrder().TargetEntityComponent)
	{
		Finish();
		return;
	}

	Production->SetPriorityTarget(GetOrder().TargetEntityComponent);

	if (OnReclaimFinishedDelegateHandle.IsValid())
	{
		Production->OnReclaimFinished().Remove(OnReclaimFinishedDelegateHandle);
	}
	OnReclaimFinishedDelegateHandle = Production->OnReclaimFinished().AddRaw(this, &FWpReclaimOrderHandler::OnReclaimFinished);
}

void FWpReclaimOrderHandler::Tick()
{
	FWpOrderHandler::Tick();

	const bool bCanReach = Production->CanReach(GetOrder().TargetEntityComponent);
	const bool bIsMoving = AIController->GetMoveStatus() != EPathFollowingStatus::Type::Idle;
	if (!bCanReach && !bIsMoving)
	{
		AIController->MoveToActor(GetOrder().TargetEntityComponent->GetOwner(), Production->GetRange(), false, true, false, nullptr, false);
	}
}

void FWpReclaimOrderHandler::Deactivate()
{
	Production->OnRepairFinished().Remove(OnReclaimFinishedDelegateHandle);
	OnReclaimFinishedDelegateHandle.Reset();
	Production->Stop();

	if (IsValid(AIController))
	{
		AIController->StopMovement();
	}

	FWpOrderHandler::Deactivate();
}

void FWpReclaimOrderHandler::OnReclaimFinished()
{
	Finish();
}
