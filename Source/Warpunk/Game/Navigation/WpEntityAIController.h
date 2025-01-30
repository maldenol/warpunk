#pragma once

#include "CoreMinimal.h"

#include "AIController.h"

#include "WpEntityAIController.generated.h"

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class AWpEntityAIController final : public AAIController
{
	GENERATED_BODY()

public:
	DECLARE_EVENT(AWpEntityAIController, FMovementCompletedEvent);

	AWpEntityAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void OnMoveCompleted(const FAIRequestID RequestId, const EPathFollowingResult::Type Result) override;

	FMovementCompletedEvent& OnMovementCompleted();

private:
	FMovementCompletedEvent MovementCompletedEvent = {};
};
