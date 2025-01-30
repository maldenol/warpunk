#include "WpReclaimOrderHandler.h"

#include "Warpunk/Game/Entity/WpEntity.h"
#include "Warpunk/Game/Production/WpProductionComponent.h"

#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"

FWpReclaimOrderHandler::FWpReclaimOrderHandler()
{
	SetTickCooldown(0.25f);
}

void FWpReclaimOrderHandler::Initialize(AWpEntity* const InEntity)
{
	FWpOrderHandler::Initialize(InEntity);

	AIController = GetEntity()->GetController<AAIController>();
	ensureAlways(AIController);

	ProductionComponent = GetEntity()->GetProductionComponent();
	ensureAlways(ProductionComponent->IsActive());
}

void FWpReclaimOrderHandler::Reactivate()
{
	FWpOrderHandler::Reactivate();

	if (!GetOrder().TargetEntity)
	{
		Finish();
		return;
	}

	if (OnReclaimFinishedDelegateHandle.IsValid())
	{
		ProductionComponent->OnReclaimFinished().Remove(OnReclaimFinishedDelegateHandle);
	}

	ProductionComponent->SetPriorityTargetEntity(GetOrder().TargetEntity);

	OnReclaimFinishedDelegateHandle = ProductionComponent->OnReclaimFinished().AddRaw(this, &FWpReclaimOrderHandler::OnReclaimFinished);
}

void FWpReclaimOrderHandler::Tick(const float CurrentTime)
{
	FWpOrderHandler::Tick(CurrentTime);

	const bool bCanReach = ProductionComponent->CanReach(GetOrder().TargetEntity);
	const bool bIsMoving = AIController->GetMoveStatus() != EPathFollowingStatus::Type::Idle;
	if (!bCanReach && !bIsMoving)
	{
		AIController->MoveToActor(GetOrder().TargetEntity, ProductionComponent->GetRange(), false, true, false, nullptr, false);
	}
}

void FWpReclaimOrderHandler::Deactivate()
{
	ProductionComponent->OnRepairFinished().Remove(OnReclaimFinishedDelegateHandle);
	OnReclaimFinishedDelegateHandle.Reset();
	ProductionComponent->Stop();

	if (AIController)
	{
		AIController->StopMovement();
	}

	FWpOrderHandler::Deactivate();
}

void FWpReclaimOrderHandler::OnReclaimFinished()
{
	Finish();
}
