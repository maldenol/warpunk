#pragma once

#include "CoreMinimal.h"

#include "GameFramework/GameModeBase.h"

#include "WpMenuGameMode.generated.h"

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class AWpMenuGameMode final : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWpMenuGameMode();

	virtual void BeginPlay() override;
};
