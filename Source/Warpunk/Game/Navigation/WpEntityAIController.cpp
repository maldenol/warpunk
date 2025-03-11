#include "WpEntityAIController.h"

#include "Warpunk/Game/Navigation/WpCrowdFollowingComponent.h"

AWpEntityAIController::AWpEntityAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UWpCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
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
