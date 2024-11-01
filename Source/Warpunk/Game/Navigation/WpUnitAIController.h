#pragma once

#include "CoreMinimal.h"

#include "AIController.h"

#include "WpUnitAIController.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnMovementCompleted);

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class AWpUnitAIController final : public AAIController
{
	GENERATED_BODY()

public:
	AWpUnitAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void OnMoveCompleted(const FAIRequestID RequestID, const EPathFollowingResult::Type Result) override;

	FOnMovementCompleted OnMovementCompleted = {};
};
