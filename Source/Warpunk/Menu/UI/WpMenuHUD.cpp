#include "WpMenuHUD.h"

#include "Warpunk/Menu/UI/WpMenuLANWidget.h"
#include "Warpunk/Menu/UI/WpMenuMainWidget.h"

#include "Kismet/KismetSystemLibrary.h"

AWpMenuHUD::AWpMenuHUD()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AWpMenuHUD::BeginPlay()
{
	Super::BeginPlay();

	ensureAlways(MenuMainWidgetClass);
	ensureAlways(MenuLANWidgetClass);

	SetMenuState(EWpMenuState::Main);

	APlayerController* const PlayerController = GetOwningPlayerController();
	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	PlayerController->SetInputMode(InputMode);
	PlayerController->SetShowMouseCursor(true);
}

void AWpMenuHUD::SetMenuState(const EWpMenuState MenuState)
{
	if (CurrentWidget)
	{
		CurrentWidget->RemoveFromParent();
	}

	switch (MenuState)
	{
	case EWpMenuState::Main:
		CurrentWidget = CreateWidget<UUserWidget>(GetWorld(), MenuMainWidgetClass);
		break;
	case EWpMenuState::Singleplayer:
		{
			FWorldContext& WorldContext = *GEngine->GetWorldContextFromWorld(GetWorld());
			FURL Url;
			Url.Map = TEXT("LobbyMap");
			FString Error;
			GEngine->Browse(WorldContext, Url, Error);
			return;
		}
		break;
	case EWpMenuState::Multiplayer:
		CurrentWidget = CreateWidget<UUserWidget>(GetWorld(), MenuLANWidgetClass);
		break;
	case EWpMenuState::Exit:
		{
			UKismetSystemLibrary::QuitGame(GetWorld(), GetOwningPlayerController(), EQuitPreference::Type::Quit, false);
			return;
		}
		break;
	default:
		ensureAlways(false);
		break;
	}

	CurrentWidget->AddToViewport();
}
