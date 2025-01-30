#include "WpPatrolOrderHandler.h"

#include "Warpunk/Game/Entity/WpEntity.h"
#include "Warpunk/Game/Navigation/WpEntityAIController.h"
#include "Warpunk/Game/Production/WpProductionComponent.h"

#include "NavigationSystem.h"
#include "AI/NavigationSystemBase.h"

void FWpPatrolOrderHandler::Initialize(AWpEntity* const InEntity)
{
	FWpOrderHandler::Initialize(InEntity);

	AIController = GetEntity()->GetController<AWpEntityAIController>();
	ensureAlways(AIController);

	ProductionComponent = GetEntity()->GetProductionComponent();
}

void FWpPatrolOrderHandler::Reactivate()
{
	FWpOrderHandler::Reactivate();

	if (GetOrder().Radius < 0.001f)
	{
		Finish();
		return;
	}

	if (ProductionComponent->IsActive() && !ProductionComponent->ShouldSupport())
	{
		bWasSupporting = false;
		ProductionComponent->SetShouldSupport(true);
	}

	if (OnMovementCompletedDelegateHandle.IsValid())
	{
		AIController->OnMovementCompleted().Remove(OnMovementCompletedDelegateHandle);
	}

	MoveToNextLocation();

	OnMovementCompletedDelegateHandle = AIController->OnMovementCompleted().AddRaw(this, &FWpPatrolOrderHandler::OnMovementCompleted);
}

void FWpPatrolOrderHandler::Deactivate()
{
	if (AIController)
	{
		AIController->OnMovementCompleted().Remove(OnMovementCompletedDelegateHandle);
		AIController->StopMovement();
	}
	OnMovementCompletedDelegateHandle.Reset();

	if (ProductionComponent->IsActive() && !bWasSupporting)
	{
		ProductionComponent->SetShouldSupport(false);
	}

	bWasSupporting = true;

	FWpOrderHandler::Deactivate();
}

void FWpPatrolOrderHandler::MoveToNextLocation()
{
	FNavLocation RandomLocation;
	UNavigationSystemV1* const NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetEntity()->GetWorld());
	if (GetOrder().Radius > 0.001f)
	{
		NavigationSystem->GetRandomReachablePointInRadius(GetOrder().TargetLocation, GetOrder().Radius, RandomLocation);
	}

	AIController->MoveToLocation(RandomLocation.Location, 5.0f, false, true, false, false, nullptr, false);
}

void FWpPatrolOrderHandler::OnMovementCompleted()
{
	MoveToNextLocation();
}
