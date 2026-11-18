#include "WpMenuLANWidget.h"

#include "Warpunk/Global/Audio/WpAudio.h"
#include "Warpunk/Menu/UI/WpMenuHUD.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"

UWpMenuLANWidget::UWpMenuLANWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
}

void UWpMenuLANWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BackButton->OnPressed.AddDynamic(this, &UWpMenuLANWidget::OnBackPressed);
	ConnectButton->OnPressed.AddDynamic(this, &UWpMenuLANWidget::OnConnectPressed);
	HostButton->OnPressed.AddDynamic(this, &UWpMenuLANWidget::OnHostPressed);
	UWpAudioSubsystem::ArmButton(this, BackButton);
	UWpAudioSubsystem::ArmButton(this, ConnectButton);
	UWpAudioSubsystem::ArmButton(this, HostButton);
}

void UWpMenuLANWidget::OnBackPressed()
{
	AWpMenuHUD* const MenuHUD = GetWorld()->GetFirstPlayerController()->GetHUD<AWpMenuHUD>();
	MenuHUD->SetMenuState(EWpMenuState::Main);
}

void UWpMenuLANWidget::OnConnectPressed()
{
	SetVisibility(ESlateVisibility::HitTestInvisible);

	const FString Url = UrlText->GetText().ToString();
	APlayerController* const PlayerController = GetWorld()->GetFirstPlayerController();
	PlayerController->ClientTravel(Url, TRAVEL_Absolute);
}

void UWpMenuLANWidget::OnHostPressed()
{
	SetVisibility(ESlateVisibility::HitTestInvisible);

	FWorldContext& WorldContext = *GEngine->GetWorldContextFromWorld(GetWorld());
	FURL Url;
	Url.Map = TEXT("/Game/Maps/LobbyMap");
	Url.AddOption(TEXT("listen"));
	FString Error;
	GEngine->Browse(WorldContext, Url, Error);
}
