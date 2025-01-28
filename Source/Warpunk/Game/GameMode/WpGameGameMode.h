#pragma once

#include "CoreMinimal.h"

#include "GameFramework/GameModeBase.h"

#include "WpGameGameMode.generated.h"

class AWpCommander;
class AWpGamePlayerController;

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class AWpGameGameMode final : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWpGameGameMode();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* const NewPlayer) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* const Player) override;

private:
	void InitializeActorPools() const;
	void InitializeCommander(AWpGamePlayerController* const PlayerController) const;
	void SpawnPooledActorVisibilityManager(const AWpGamePlayerController* const PlayerController) const;
	void SetExpectedLoadedObjectNum(AWpGamePlayerController* const PlayerController) const;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AWpCommander> CommanderClass = nullptr;
};
