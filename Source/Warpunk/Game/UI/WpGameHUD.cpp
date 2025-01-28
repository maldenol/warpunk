#include "WpGameHUD.h"

#include "Warpunk/Game/UI/WpGameHUDWidget.h"
#include "Warpunk/Global/GameInstance/WpGameInstance.h"
#include "Warpunk/Global/LoadingScreen/WpLoadingScreen.h"

AWpGameHUD::AWpGameHUD()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AWpGameHUD::BeginPlay()
{
	Super::BeginPlay();

	UWpLoadingScreen* const LoadingScreen = GetGameInstance<UWpGameInstance>()->GetLoadingScreen();
	LoadingScreen->OnLoaded().AddUObject(this, &AWpGameHUD::OnLoaded);
	if (LoadingScreen->IsLoaded())
	{
		OnLoaded();
	}
}

void AWpGameHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetGameInstance<UWpGameInstance>()->GetLoadingScreen()->OnLoaded().RemoveAll(this);

	Super::EndPlay(EndPlayReason);
}

void AWpGameHUD::OnLoaded()
{
	ensureAlways(GameHUDWidgetClass);
	GameHUDWidget = CreateWidget<UWpGameHUDWidget>(GetWorld(), GameHUDWidgetClass);
	GameHUDWidget->AddToViewport();

	GetGameInstance<UWpGameInstance>()->GetLoadingScreen()->OnLoaded().RemoveAll(this);
}
