#pragma once

#include "CoreMinimal.h"

#include "GameFramework/HUD.h"

#include "WpLobbyHUD.generated.h"

class UWpLobbyHUDWidget;

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class AWpLobbyHUD final : public AHUD
{
	GENERATED_BODY()

public:
	AWpLobbyHUD();

	virtual void BeginPlay() override;

private:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UWpLobbyHUDWidget> LobbyHUDWidgetClass = nullptr;

	UPROPERTY(VisibleInstanceOnly)
	TObjectPtr<UWpLobbyHUDWidget> LobbyHUDWidget = nullptr;
};
