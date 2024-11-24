#include "WpLobbyHUD.h"

#include "Warpunk/Lobby/UI/WpLobbyHUDWidget.h"

AWpLobbyHUD::AWpLobbyHUD()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AWpLobbyHUD::BeginPlay()
{
	Super::BeginPlay();

	ensureAlways(LobbyHUDWidgetClass);

	LobbyHUDWidget = CreateWidget<UWpLobbyHUDWidget>(GetWorld(), LobbyHUDWidgetClass);
	LobbyHUDWidget->AddToViewport();

	APlayerController* const PlayerController = GetOwningPlayerController();
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	PlayerController->SetInputMode(InputMode);
	PlayerController->SetShowMouseCursor(true);
}
