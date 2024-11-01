#include "WpProduceBuildingOrderHandler.h"

#include "Warpunk/Game/EntityComponent/WpEntityComponent.h"
#include "Warpunk/Game/Production/WpProduction.h"

#include "AIController.h"

void FWpProduceBuildingOrderHandler::SetOwnerEntityComponent(UWpEntityComponent* const InOwnerEntityComponent)
{
	FWpOrderHandler::SetOwnerEntityComponent(InOwnerEntityComponent);

	AIController = GetOwnerEntityComponent()->GetOwner<APawn>()->GetController<AAIController>();
	ensureAlways(AIController);

	Production = &WP_AS_PRODUCTION(GetOwnerEntityComponent());
	ensureAlways(Production->IsActive());
}

void FWpProduceBuildingOrderHandler::Reactivate()
{
	FWpOrderHandler::Reactivate();

	Production->StartProduce(GetOrder().BuildingToProduceConfigID, GetOrder().TargetLocation);

	if (OnRepairFinishedDelegateHandle.IsValid())
	{
		Production->OnRepairFinished.Remove(OnRepairFinishedDelegateHandle);
	}
	OnRepairFinishedDelegateHandle = Production->OnRepairFinished.AddRaw(this, &FWpProduceBuildingOrderHandler::OnRepairFinished);

	if (!Production->CanReach(GetOrder().TargetLocation))
	{
		AIController->MoveToLocation(GetOrder().TargetLocation, Production->GetRange(), false, true, false, false);
	}
}

void FWpProduceBuildingOrderHandler::Deactivate()
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

void FWpProduceBuildingOrderHandler::OnRepairFinished(UWpEntityComponent* const EntityComponent)
{
	Finish();
}
