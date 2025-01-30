#include "WpProduceBuildingOrderHandler.h"

#include "Warpunk/Game/Entity/WpEntity.h"
#include "Warpunk/Game/Production/WpProductionComponent.h"

#include "AIController.h"

void FWpProduceBuildingOrderHandler::Initialize(AWpEntity* const InEntity)
{
	FWpOrderHandler::Initialize(InEntity);

	AIController = GetEntity()->GetController<AAIController>();
	ensureAlways(AIController);

	ProductionComponent = GetEntity()->GetProductionComponent();
	ensureAlways(ProductionComponent->IsActive());
}

void FWpProduceBuildingOrderHandler::Reactivate()
{
	FWpOrderHandler::Reactivate();

	if (OnRepairFinishedDelegateHandle.IsValid())
	{
		ProductionComponent->OnRepairFinished().Remove(OnRepairFinishedDelegateHandle);
	}

	ProductionComponent->StartProduce(GetOrder().BuildingToProduceConfigId, GetOrder().TargetLocation);

	OnRepairFinishedDelegateHandle = ProductionComponent->OnRepairFinished().AddRaw(this, &FWpProduceBuildingOrderHandler::OnRepairFinished);

	if (!ProductionComponent->CanReach(GetOrder().TargetLocation))
	{
		AIController->MoveToLocation(GetOrder().TargetLocation, ProductionComponent->GetRange(), false, true, false, false);
	}
}

void FWpProduceBuildingOrderHandler::Deactivate()
{
	ProductionComponent->OnRepairFinished().Remove(OnRepairFinishedDelegateHandle);
	OnRepairFinishedDelegateHandle.Reset();
	ProductionComponent->Stop();

	if (AIController)
	{
		AIController->StopMovement();
	}

	FWpOrderHandler::Deactivate();
}

void FWpProduceBuildingOrderHandler::OnRepairFinished(AWpEntity* const InEntity)
{
	Finish();
}
