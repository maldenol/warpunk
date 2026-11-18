#include "WpLobbyOptionsWidget.h"

#include "Warpunk/Global/Audio/WpAudio.h"
#include "Warpunk/Lobby/GameMode/WpLobbyGameState.h"

#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/EditableTextBox.h"

UWpLobbyOptionsWidget::UWpLobbyOptionsWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
}

void UWpLobbyOptionsWidget::NativeConstruct()
{
	Super::NativeConstruct();

	AllowTeams->OnCheckStateChanged.AddDynamic(this, &UWpLobbyOptionsWidget::OnAllowTeamsChanged);
	MaxPlayerNumber->OnTextCommitted.AddDynamic(this, &UWpLobbyOptionsWidget::OnMaxPlayerNumberCommitted);
	MaxSpectatorNumber->OnTextCommitted.AddDynamic(this, &UWpLobbyOptionsWidget::OnMaxSpectatorNumberCommitted);
	PlayersDown->OnPressed.AddDynamic(this, &UWpLobbyOptionsWidget::OnPlayersDown);
	PlayersUp->OnPressed.AddDynamic(this, &UWpLobbyOptionsWidget::OnPlayersUp);
	SpectatorsDown->OnPressed.AddDynamic(this, &UWpLobbyOptionsWidget::OnSpectatorsDown);
	SpectatorsUp->OnPressed.AddDynamic(this, &UWpLobbyOptionsWidget::OnSpectatorsUp);
	UWpAudioSubsystem::ArmButton(this, PlayersDown);
	UWpAudioSubsystem::ArmButton(this, PlayersUp);
	UWpAudioSubsystem::ArmButton(this, SpectatorsDown);
	UWpAudioSubsystem::ArmButton(this, SpectatorsUp);
	UWpAudioSubsystem::ArmCheckBox(this, AllowTeams);
	MaxPlayerNumber->SetIsReadOnly(true);
	MaxSpectatorNumber->SetIsReadOnly(true);

	if (GetWorld()->GetAuthGameMode())
	{
		AWpLobbyGameState* const GameState = GetWorld()->GetGameState<AWpLobbyGameState>();
		const uint32 Players = static_cast<uint32>(FMath::Clamp(static_cast<int32>(GameState->GetMaxPlayerNumber()), 2, 4));
		if (Players != GameState->GetMaxPlayerNumber())
		{
			GameState->SetMaxPlayerNumber(Players);
		}
	}

	if (!GetWorld()->GetAuthGameMode())
	{
		AllowTeams->SetVisibility(ESlateVisibility::HitTestInvisible);
		MaxPlayerNumber->SetVisibility(ESlateVisibility::HitTestInvisible);
		MaxSpectatorNumber->SetVisibility(ESlateVisibility::HitTestInvisible);
		PlayersDown->SetVisibility(ESlateVisibility::Hidden);
		PlayersUp->SetVisibility(ESlateVisibility::Hidden);
		SpectatorsDown->SetVisibility(ESlateVisibility::Hidden);
		SpectatorsUp->SetVisibility(ESlateVisibility::Hidden);

		AWpLobbyGameState* const GameState = GetWorld()->GetGameState<AWpLobbyGameState>();
		GameState->OnLobbySettingsUpdated().AddUObject(this, &UWpLobbyOptionsWidget::OnLobbySettingsUpdated);
	}

	OnLobbySettingsUpdated();
}

void UWpLobbyOptionsWidget::OnAllowTeamsChanged(const bool bIsChecked)
{
	AWpLobbyGameState* const GameState = GetWorld()->GetGameState<AWpLobbyGameState>();
	GameState->SetTeamsAllowed(bIsChecked);
}

void UWpLobbyOptionsWidget::OnMaxPlayerNumberCommitted(const FText& Text, const ETextCommit::Type CommitMethod)
{
	AWpLobbyGameState* const GameState = GetWorld()->GetGameState<AWpLobbyGameState>();
	const int32 Requested = FCString::Atoi(*Text.ToString());
	GameState->SetMaxPlayerNumber(static_cast<uint32>(FMath::Clamp(Requested, 2, 4)));
	OnLobbySettingsUpdated();
}

void UWpLobbyOptionsWidget::OnMaxSpectatorNumberCommitted(const FText& Text, const ETextCommit::Type CommitMethod)
{
	AWpLobbyGameState* const GameState = GetWorld()->GetGameState<AWpLobbyGameState>();
	GameState->SetMaxSpectatorNumber(FCString::Atoi(*Text.ToString()));
}

void UWpLobbyOptionsWidget::OnPlayersDown()
{
	StepCount(true, -1);
}

void UWpLobbyOptionsWidget::OnPlayersUp()
{
	StepCount(true, 1);
}

void UWpLobbyOptionsWidget::OnSpectatorsDown()
{
	StepCount(false, -1);
}

void UWpLobbyOptionsWidget::OnSpectatorsUp()
{
	StepCount(false, 1);
}

void UWpLobbyOptionsWidget::StepCount(const bool bPlayers, const int32 Delta)
{
	AWpLobbyGameState* const GameState = GetWorld()->GetGameState<AWpLobbyGameState>();
	const int32 Current = static_cast<int32>(bPlayers ? GameState->GetMaxPlayerNumber() : GameState->GetMaxSpectatorNumber());
	if (bPlayers)
	{
		GameState->SetMaxPlayerNumber(static_cast<uint32>(FMath::Clamp(Current + Delta, 2, 4)));
	}
	else
	{
		GameState->SetMaxSpectatorNumber(static_cast<uint32>(FMath::Max(0, Current + Delta)));
	}

	OnLobbySettingsUpdated();
}

void UWpLobbyOptionsWidget::OnLobbySettingsUpdated()
{
	const AWpLobbyGameState* const GameState = GetWorld()->GetGameState<AWpLobbyGameState>();
	AllowTeams->SetCheckedState(GameState->AreTeamsAllowed() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
	MaxPlayerNumber->SetText(FText::FromString(FString::FromInt(GameState->GetMaxPlayerNumber())));
	MaxSpectatorNumber->SetText(FText::FromString(FString::FromInt(GameState->GetMaxSpectatorNumber())));
}
