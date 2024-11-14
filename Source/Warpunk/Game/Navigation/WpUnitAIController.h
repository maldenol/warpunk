#pragma once

#include "CoreMinimal.h"

#include "AIController.h"

#include "WpUnitAIController.generated.h"

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class AWpUnitAIController final : public AAIController
{
	GENERATED_BODY()

public:
	DECLARE_EVENT(AWpUnitAIController, FMovementCompletedEvent);

	AWpUnitAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void OnMoveCompleted(const FAIRequestID RequestId, const EPathFollowingResult::Type Result) override;

	FMovementCompletedEvent& OnMovementCompleted();

private:
	FMovementCompletedEvent MovementCompletedEvent = {};
};
