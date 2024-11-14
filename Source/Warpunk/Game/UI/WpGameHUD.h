#pragma once

#include "CoreMinimal.h"

#include "GameFramework/HUD.h"

#include "WpGameHUD.generated.h"

class UWpGameHUDWidget;

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class AWpGameHUD final : public AHUD
{
	GENERATED_BODY()

public:
	AWpGameHUD();

	virtual void BeginPlay() override;

private:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UWpGameHUDWidget> GameHUDWidgetClass = nullptr;

	UPROPERTY(VisibleInstanceOnly)
	TObjectPtr<UWpGameHUDWidget> GameHUDWidget = nullptr;
};
