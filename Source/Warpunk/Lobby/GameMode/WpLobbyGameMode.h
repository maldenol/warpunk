#pragma once

#include "CoreMinimal.h"

#include "GameFramework/GameModeBase.h"

#include "WpLobbyGameMode.generated.h"

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class AWpLobbyGameMode final : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWpLobbyGameMode();

	virtual void BeginPlay() override;

	virtual void OnPostLogin(AController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	void NotifyPlayerStateUpdated();
	void NotifyPlayerReadinessChanged();

private:
	void StartGame();

	FTimerHandle StartGameTimerHandle = {};

	static const float TimeUntilGameStart;
};
