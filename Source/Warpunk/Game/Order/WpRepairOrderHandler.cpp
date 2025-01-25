#include "WpRepairOrderHandler.h"

#include "Warpunk/Game/Entity/WpEntityInterface.h"
#include "Warpunk/Game/Production/WpProductionComponent.h"

#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"

FWpRepairOrderHandler::FWpRepairOrderHandler()
{
	SetTickCooldown(0.25f);
}

void FWpRepairOrderHandler::Initialize(IWpEntityInterface* const InEntity)
{
	FWpOrderHandler::Initialize(InEntity);

	AIController = Cast<APawn>(GetEntity())->GetController<AAIController>();
	ensureAlways(AIController);

	ProductionComponent = GetEntity()->GetProductionComponent();
	ensureAlways(ProductionComponent->IsActive());
}

void FWpRepairOrderHandler::Reactivate()
{
	FWpOrderHandler::Reactivate();

	if (!GetOrder().TargetEntityActor)
	{
		Finish();
		return;
	}

	if (OnRepairFinishedDelegateHandle.IsValid())
	{
		ProductionComponent->OnRepairFinished().Remove(OnRepairFinishedDelegateHandle);
	}

	ProductionComponent->SetPriorityTargetEntity(GetOrder().GetTargetEntity());

	OnRepairFinishedDelegateHandle = ProductionComponent->OnRepairFinished().AddRaw(this, &FWpRepairOrderHandler::OnRepairFinished);
}

void FWpRepairOrderHandler::Tick(const float CurrentTime)
{
	FWpOrderHandler::Tick(CurrentTime);

	const bool bCanReach = ProductionComponent->CanReach(GetOrder().GetTargetEntity());
	const bool bIsMoving = AIController->GetMoveStatus() != EPathFollowingStatus::Type::Idle;
	if (!bCanReach && !bIsMoving)
	{
		AIController->MoveToActor(GetOrder().TargetEntityActor, ProductionComponent->GetRange(), false, true, false, nullptr, false);
	}
}

void FWpRepairOrderHandler::Deactivate()
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

void FWpRepairOrderHandler::OnRepairFinished(IWpEntityInterface* const InEntity)
{
	Finish();
}
