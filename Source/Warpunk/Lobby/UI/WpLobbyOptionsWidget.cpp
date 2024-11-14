#include "WpLobbyOptionsWidget.h"

#include "Warpunk/Lobby/GameMode/WpLobbyGameState.h"

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

	if (!GetWorld()->GetAuthGameMode())
	{
		AllowTeams->SetVisibility(ESlateVisibility::HitTestInvisible);
		MaxPlayerNumber->SetVisibility(ESlateVisibility::HitTestInvisible);
		MaxSpectatorNumber->SetVisibility(ESlateVisibility::HitTestInvisible);

		AWpLobbyGameState* const GameState = GetWorld()->GetGameState<AWpLobbyGameState>();
		GameState->OnLobbySettingsUpdated().AddUObject(this, &UWpLobbyOptionsWidget::OnLobbySettingsUpdated);
	}

	OnLobbySettingsUpdated();
}

void UWpLobbyOptionsWidget::OnAllowTeamsChanged(const bool bIsChecked)
{
	AWpLobbyGameState* const GameState = GetWorld()->GetGameState<AWpLobbyGameState>();
	GameState->SetAllowTeams(bIsChecked);
}

void UWpLobbyOptionsWidget::OnMaxPlayerNumberCommitted(const FText& Text, const ETextCommit::Type CommitMethod)
{
	AWpLobbyGameState* const GameState = GetWorld()->GetGameState<AWpLobbyGameState>();
	GameState->SetMaxPlayerNumber(FCString::Atoi(*Text.ToString()));
}

void UWpLobbyOptionsWidget::OnMaxSpectatorNumberCommitted(const FText& Text, const ETextCommit::Type CommitMethod)
{
	AWpLobbyGameState* const GameState = GetWorld()->GetGameState<AWpLobbyGameState>();
	GameState->SetMaxSpectatorNumber(FCString::Atoi(*Text.ToString()));
}

void UWpLobbyOptionsWidget::OnLobbySettingsUpdated()
{
	const AWpLobbyGameState* const GameState = GetWorld()->GetGameState<AWpLobbyGameState>();
	AllowTeams->SetCheckedState(GameState->GetAllowTeams() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
	MaxPlayerNumber->SetText(FText::FromString(FString::FromInt(GameState->GetMaxPlayerNumber())));
	MaxSpectatorNumber->SetText(FText::FromString(FString::FromInt(GameState->GetMaxSpectatorNumber())));
}
