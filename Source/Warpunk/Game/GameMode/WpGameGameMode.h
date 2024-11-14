#pragma once

#include "CoreMinimal.h"

#include "GameFramework/GameModeBase.h"

#include "WpGameGameMode.generated.h"

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class AWpGameGameMode final : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWpGameGameMode();

	virtual void BeginPlay() override;

	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
};
