#pragma once

#include "CoreMinimal.h"

#include "GameFramework/HUD.h"

#include "WpMenuHUD.generated.h"

class UWpMenuLANWidget;
class UWpMenuMainWidget;

UENUM(NotBlueprintable, NotBlueprintType)
enum class EWpMenuState : uint8
{
	Main,
	Singleplayer,
	Multiplayer,
	//Options,
	//Credits,
	Exit,
};

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class AWpMenuHUD final : public AHUD
{
	GENERATED_BODY()

public:
	AWpMenuHUD();

	virtual void BeginPlay() override;

	void SetMenuState(const EWpMenuState MenuState);

private:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UWpMenuMainWidget> MenuMainWidgetClass = nullptr;
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UWpMenuLANWidget> MenuLANWidgetClass = nullptr;

	UPROPERTY(VisibleInstanceOnly)
	TObjectPtr<UUserWidget> CurrentWidget = nullptr;
};
