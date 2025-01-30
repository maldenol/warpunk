#include "WpEntityAIController.h"

#include "Navigation/CrowdFollowingComponent.h"

AWpEntityAIController::AWpEntityAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	PrimaryActorTick.bCanEverTick = false;
}

void AWpEntityAIController::OnMoveCompleted(const FAIRequestID RequestId, const EPathFollowingResult::Type Result)
{
	MovementCompletedEvent.Broadcast();
}

AWpEntityAIController::FMovementCompletedEvent& AWpEntityAIController::OnMovementCompleted()
{
	return MovementCompletedEvent;
}
