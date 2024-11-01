#pragma once

#include "CoreMinimal.h"

#include "GameFramework/GameMode.h"

#include "WpGameGameMode.generated.h"

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class AWpGameGameMode final : public AGameMode
{
	GENERATED_BODY()

public:
	AWpGameGameMode();

	virtual void BeginPlay() override;
};
