#include "WpGameHUD.h"

#include "Warpunk/Game/UI/WpGameHUDWidget.h"

AWpGameHUD::AWpGameHUD()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AWpGameHUD::BeginPlay()
{
	Super::BeginPlay();

	ensureAlways(GameHUDWidgetClass);

	GameHUDWidget = CreateWidget<UWpGameHUDWidget>(GetWorld(), GameHUDWidgetClass);
	GameHUDWidget->AddToViewport();
}
