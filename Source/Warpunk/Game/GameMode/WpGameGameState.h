#pragma once

#include "CoreMinimal.h"

#include "GameFramework/GameStateBase.h"

#include "WpGameGameState.generated.h"

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class AWpGameGameState final : public AGameStateBase
{
	GENERATED_BODY()

public:
	DECLARE_EVENT(AWpGameGameState, FBelligerentTeamsReplicatedEvent);

	AWpGameGameState();

	virtual void BeginPlay() override;

	const UDataTable* GetEntityConfigs() const;

	UPROPERTY(EditDefaultsOnly)
	uint32 CommanderConfigId = 0;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> EnduranceWidgetClass = nullptr;
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> SelectionWidgetClass = nullptr;

private:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UDataTable> EntityConfigs = nullptr;
};
