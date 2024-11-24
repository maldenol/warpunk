#include "WpReclaimOrderHandler.h"

#include "Warpunk/Game/Entity/WpEntityInterface.h"
#include "Warpunk/Game/Production/WpProductionComponent.h"

#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"

void FWpReclaimOrderHandler::Initialize(IWpEntityInterface* const InEntity)
{
	FWpOrderHandler::Initialize(InEntity);

	AIController = Cast<APawn>(GetEntity())->GetController<AAIController>();
	ensureAlways(AIController);

	ProductionComponent = GetEntity()->GetProductionComponent();
}

void FWpReclaimOrderHandler::Reactivate()
{
	FWpOrderHandler::Reactivate();

	if (!IsValid(GetOrder().TargetEntityActor))
	{
		Finish();
		return;
	}

	if (OnReclaimFinishedDelegateHandle.IsValid())
	{
		ProductionComponent->OnReclaimFinished().Remove(OnReclaimFinishedDelegateHandle);
	}

	ProductionComponent->SetPriorityTargetEntity(GetOrder().GetTargetEntity());

	OnReclaimFinishedDelegateHandle = ProductionComponent->OnReclaimFinished().AddRaw(this, &FWpReclaimOrderHandler::OnReclaimFinished);
}

void FWpReclaimOrderHandler::Tick()
{
	FWpOrderHandler::Tick();

	const bool bCanReach = ProductionComponent->CanReach(GetOrder().GetTargetEntity());
	const bool bIsMoving = AIController->GetMoveStatus() != EPathFollowingStatus::Type::Idle;
	if (!bCanReach && !bIsMoving)
	{
		AIController->MoveToActor(GetOrder().TargetEntityActor, ProductionComponent->GetRange(), false, true, false, nullptr, false);
	}
}

void FWpReclaimOrderHandler::Deactivate()
{
	ProductionComponent->OnRepairFinished().Remove(OnReclaimFinishedDelegateHandle);
	OnReclaimFinishedDelegateHandle.Reset();
	ProductionComponent->Stop();

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
