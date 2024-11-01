#pragma once

#include "CoreMinimal.h"

#include "GameFramework/GameState.h"

#include "WpGameGameState.generated.h"

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class AWpGameGameState final : public AGameState
{
	GENERATED_BODY()

public:
	AWpGameGameState();

	const UDataTable* GetEntityConfigs() const;

	uint32 GetPlayerBelligerentID(const APlayerController* const PlayerController) const;

	UPROPERTY(EditDefaultsOnly)
	uint32 CommanderConfigID = 0;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> EnduranceWidgetClass = nullptr;
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> SelectionWidgetClass = nullptr;

private:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UDataTable> EntityConfigs = nullptr;

	TMap<const APlayerController*, uint32> PlayerBelligerentIDs = {};
};
