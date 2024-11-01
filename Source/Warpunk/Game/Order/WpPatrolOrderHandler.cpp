#include "WpPatrolOrderHandler.h"

#include "Warpunk/Game/EntityComponent/WpEntityComponent.h"
#include "Warpunk/Game/Navigation/WpUnitAIController.h"
#include "Warpunk/Game/Production/WpProduction.h"

#include "NavigationSystem.h"
#include "AI/NavigationSystemBase.h"

void FWpPatrolOrderHandler::SetOwnerEntityComponent(UWpEntityComponent* const InOwnerEntityComponent)
{
	FWpOrderHandler::SetOwnerEntityComponent(InOwnerEntityComponent);

	AIController = GetOwnerEntityComponent()->GetOwner<APawn>()->GetController<AWpUnitAIController>();
	ensureAlways(AIController);

	Production = &WP_AS_PRODUCTION(GetOwnerEntityComponent());
}

void FWpPatrolOrderHandler::Reactivate()
{
	FWpOrderHandler::Reactivate();

	if (GetOrder().Radius < 0.001f)
	{
		Finish();
		return;
	}

	if (Production->IsActive() && !Production->ShouldSupport())
	{
		bWasSupporting = false;
		Production->SetShouldSupport(true);
	}

	if (OnMovementCompletedDelegateHandle.IsValid())
	{
		AIController->OnMovementCompleted.Remove(OnMovementCompletedDelegateHandle);
	}

	MoveToNextLocation();

	OnMovementCompletedDelegateHandle = AIController->OnMovementCompleted.AddRaw(this, &FWpPatrolOrderHandler::OnMovementCompleted);
}

void FWpPatrolOrderHandler::Deactivate()
{
	if (IsValid(AIController))
	{
		AIController->OnMovementCompleted.Remove(OnMovementCompletedDelegateHandle);
		AIController->StopMovement();
	}
	OnMovementCompletedDelegateHandle.Reset();

	if (Production->IsActive() && !bWasSupporting)
	{
		Production->SetShouldSupport(false);
	}

	bWasSupporting = true;

	FWpOrderHandler::Deactivate();
}

void FWpPatrolOrderHandler::MoveToNextLocation()
{
	FNavLocation RandomLocation;
	UNavigationSystemV1* const NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetOwnerEntityComponent()->GetWorld());
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
