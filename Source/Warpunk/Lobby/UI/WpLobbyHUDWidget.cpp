#include "WpLobbyHUDWidget.h"

#include "Warpunk/Lobby/GameMode/WpLobbyGameState.h"
#include "Warpunk/Lobby/Player/WpLobbyPlayerController.h"
#include "Warpunk/Lobby/Player/WpLobbyPlayerState.h"
#include "Warpunk/Global/Audio/WpAudio.h"
#include "Warpunk/Lobby/UI/WpLobbyOptionsWidget.h"
#include "Warpunk/Lobby/UI/WpLobbyPlayerListWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

UWpLobbyHUDWidget::UWpLobbyHUDWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
}

void UWpLobbyHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ReadyButton->OnPressed.AddDynamic(this, &UWpLobbyHUDWidget::OnReadyPressed);
	BackButton->OnPressed.AddDynamic(this, &UWpLobbyHUDWidget::OnBackPressed);
	UWpAudioSubsystem::ArmButton(this, ReadyButton);
	UWpAudioSubsystem::ArmButton(this, BackButton);

	if (AWpLobbyPlayerState* const PlayerState = GetOwningPlayer()->GetPlayerState<AWpLobbyPlayerState>())
	{
		PlayerState->OnPlayerSettingsUpdated().AddUObject(this, &UWpLobbyHUDWidget::OnPlayerSettingsUpdated);
	}
	else
	{
		AWpLobbyPlayerController* const PlayerController = GetOwningPlayer<AWpLobbyPlayerController>();
		PlayerController->OnPlayerStateReplicated().AddUObject(this, &UWpLobbyHUDWidget::OnPlayerStateReplicated);
	}

	AWpLobbyGameState* const GameState = GetWorld()->GetGameState<AWpLobbyGameState>();
	GameState->OnTimeUntilGameStartUpdated().AddUObject(this, &UWpLobbyHUDWidget::OnTimeUntilGameStartUpdated);
	OnTimeUntilGameStartUpdated(-1.0f);
}

void UWpLobbyHUDWidget::OnReadyPressed()
{
	AWpLobbyPlayerController* const PlayerController = GetOwningPlayer<AWpLobbyPlayerController>();
	const AWpLobbyPlayerState* const PlayerState = PlayerController->GetPlayerState<AWpLobbyPlayerState>();
	bLocalReady = !PlayerState->IsReady();
	SetElementsVisibility(bLocalReady);
	RefreshReadyLabel();

	PlayerController->ServerSetReady(bLocalReady);
}

void UWpLobbyHUDWidget::OnBackPressed()
{
	FWorldContext& WorldContext = *GEngine->GetWorldContextFromWorld(GetWorld());
	FURL Url;
	Url.Map = TEXT("/Game/Maps/MenuMap");
	FString Error;
	GEngine->Browse(WorldContext, Url, Error);
}

void UWpLobbyHUDWidget::OnPlayerStateReplicated()
{
	AWpLobbyPlayerState* const PlayerState = GetOwningPlayer()->GetPlayerState<AWpLobbyPlayerState>();
	PlayerState->OnPlayerSettingsUpdated().AddUObject(this, &UWpLobbyHUDWidget::OnPlayerSettingsUpdated);
}

void UWpLobbyHUDWidget::OnPlayerSettingsUpdated()
{
	const AWpLobbyPlayerState* const PlayerState = GetOwningPlayer()->GetPlayerState<AWpLobbyPlayerState>();
	bLocalReady = PlayerState->IsReady();
	SetElementsVisibility(bLocalReady);
	RefreshReadyLabel();
}

void UWpLobbyHUDWidget::OnTimeUntilGameStartUpdated(const float InTimeUntilGameStart)
{
	TimeUntilGameStart = InTimeUntilGameStart;
	RefreshReadyLabel();
}

void UWpLobbyHUDWidget::RefreshReadyLabel()
{
	FString ReadyButtonTitle = TEXT("Ready");
	if (TimeUntilGameStart >= 0.0f)
	{
		ReadyButtonTitle = FString::Printf(TEXT("Launch in %d"), FMath::CeilToInt(TimeUntilGameStart));
	}
	else if (bLocalReady)
	{
		ReadyButtonTitle = TEXT("Cancel");
	}

	ReadyButtonText->SetText(FText::FromString(ReadyButtonTitle));
}

void UWpLobbyHUDWidget::SetElementsVisibility(const bool bIsReady)
{
	const ESlateVisibility ElementVisibility = bIsReady ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Visible;
	BackButton->SetVisibility(ElementVisibility);
	LobbyOptionsWidget->SetVisibility(ElementVisibility);
	LobbyPlayerListWidget->SetVisibility(ElementVisibility);
}
