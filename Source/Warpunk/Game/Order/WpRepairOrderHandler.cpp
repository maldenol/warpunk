#include "WpRepairOrderHandler.h"

#include "Warpunk/Game/Entity/WpEntity.h"
#include "Warpunk/Game/Production/WpProductionComponent.h"

#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"

FWpRepairOrderHandler::FWpRepairOrderHandler()
{
	SetTickCooldown(0.25f);
}

void FWpRepairOrderHandler::Initialize(AWpEntity* const InEntity)
{
	FWpOrderHandler::Initialize(InEntity);

	AIController = GetEntity()->GetController<AAIController>();
	ensureAlways(AIController);

	ProductionComponent = GetEntity()->GetProductionComponent();
	ensureAlways(ProductionComponent->IsActive());
}

void FWpRepairOrderHandler::Reactivate()
{
	FWpOrderHandler::Reactivate();

	if (!GetOrder().TargetEntity)
	{
		Finish();
		return;
	}

	if (OnRepairFinishedDelegateHandle.IsValid())
	{
		ProductionComponent->OnRepairFinished().Remove(OnRepairFinishedDelegateHandle);
	}

	ProductionComponent->SetPriorityTargetEntity(GetOrder().TargetEntity);

	OnRepairFinishedDelegateHandle = ProductionComponent->OnRepairFinished().AddRaw(this, &FWpRepairOrderHandler::OnRepairFinished);
}

void FWpRepairOrderHandler::Tick(const float CurrentTime)
{
	FWpOrderHandler::Tick(CurrentTime);

	const bool bCanReach = ProductionComponent->CanReach(GetOrder().TargetEntity);
	const bool bIsMoving = AIController->GetMoveStatus() != EPathFollowingStatus::Type::Idle;
	if (!bCanReach && !bIsMoving)
	{
		AIController->MoveToActor(GetOrder().TargetEntity, ProductionComponent->GetRange(), false, true, false, nullptr, false);
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

void FWpRepairOrderHandler::OnRepairFinished(AWpEntity* const InEntity)
{
	Finish();
}
