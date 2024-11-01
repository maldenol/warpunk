#include "WpUnitAIController.h"

#include "Navigation/CrowdFollowingComponent.h"

AWpUnitAIController::AWpUnitAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	PrimaryActorTick.bCanEverTick = false;
}

void AWpUnitAIController::OnMoveCompleted(const FAIRequestID RequestID, const EPathFollowingResult::Type Result)
{
	OnMovementCompleted.Broadcast();
}
