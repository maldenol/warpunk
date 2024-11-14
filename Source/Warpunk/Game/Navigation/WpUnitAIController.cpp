#include "WpUnitAIController.h"

#include "Navigation/CrowdFollowingComponent.h"

AWpUnitAIController::AWpUnitAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	PrimaryActorTick.bCanEverTick = false;
}

void AWpUnitAIController::OnMoveCompleted(const FAIRequestID RequestId, const EPathFollowingResult::Type Result)
{
	MovementCompletedEvent.Broadcast();
}

AWpUnitAIController::FMovementCompletedEvent& AWpUnitAIController::OnMovementCompleted()
{
	return MovementCompletedEvent;
}
