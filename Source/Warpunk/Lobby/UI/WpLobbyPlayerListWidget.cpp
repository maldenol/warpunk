#include "WpLobbyPlayerListWidget.h"

#include "Warpunk/Lobby/GameMode/WpLobbyGameState.h"
#include "Warpunk/Lobby/Player/WpLobbyPlayerController.h"
#include "Warpunk/Lobby/Player/WpLobbyPlayerState.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "GameFramework/GameSession.h"

UWpLobbyPlayerListEntryWidget::UWpLobbyPlayerListEntryWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
}

void UWpLobbyPlayerListEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BelligerentId->OnTextCommitted.AddDynamic(this, &UWpLobbyPlayerListEntryWidget::OnBelligerentIdCommitted);
	TeamId->OnTextCommitted.AddDynamic(this, &UWpLobbyPlayerListEntryWidget::OnTeamIdCommitted);
	ColorText->OnTextCommitted.AddDynamic(this, &UWpLobbyPlayerListEntryWidget::OnColorTextCommitted);

	if (GetWorld()->GetAuthGameMode())
	{
		KickButton->OnPressed.AddDynamic(this, &UWpLobbyPlayerListEntryWidget::OnKickPressed);
	}
	else
	{
		KickButton->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UWpLobbyPlayerListEntryWidget::SetPlayerState(AWpLobbyPlayerState* const InPlayerState)
{
	PlayerState = InPlayerState;

	const APlayerController* const PlayerController = PlayerState->GetPlayerController();

	const bool bCanEdit = PlayerController && PlayerController->IsLocalController();
	if (!bCanEdit)
	{
		BelligerentId->SetVisibility(ESlateVisibility::HitTestInvisible);
		TeamId->SetVisibility(ESlateVisibility::HitTestInvisible);
		ColorText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	const bool bCanKick = PlayerController && !PlayerController->IsLocalController();
	if (!bCanKick)
	{
		KickButton->SetVisibility(ESlateVisibility::Hidden);
	}

	PlayerState->OnPlayerSettingsUpdated().AddUObject(this, &UWpLobbyPlayerListEntryWidget::OnPlayerSettingsUpdated);
	OnPlayerSettingsUpdated();
}

const AWpLobbyPlayerState* UWpLobbyPlayerListEntryWidget::GetPlayerState() const
{
	return PlayerState;
}

void UWpLobbyPlayerListEntryWidget::OnBelligerentIdCommitted(const FText& Text, const ETextCommit::Type CommitMethod)
{
	AWpLobbyPlayerController* const InnerPlayerController = Cast<AWpLobbyPlayerController>(PlayerState->GetPlayerController());
	InnerPlayerController->ServerSetBelligerentId(FCString::Atoi(*Text.ToString()));
}

void UWpLobbyPlayerListEntryWidget::OnTeamIdCommitted(const FText& Text, const ETextCommit::Type CommitMethod)
{
	AWpLobbyPlayerController* const InnerPlayerController = Cast<AWpLobbyPlayerController>(PlayerState->GetPlayerController());
	InnerPlayerController->ServerSetTeamId(FCString::Atoi(*Text.ToString()));
}

void UWpLobbyPlayerListEntryWidget::OnColorTextCommitted(const FText& Text, const ETextCommit::Type CommitMethod)
{
	AWpLobbyPlayerController* const InnerPlayerController = Cast<AWpLobbyPlayerController>(PlayerState->GetPlayerController());
	InnerPlayerController->ServerSetColor(FColor::FromHex(Text.ToString()));
}

void UWpLobbyPlayerListEntryWidget::OnKickPressed()
{
	static const FText Reason = FText::FromString(TEXT("Kicked by host"));
	GetWorld()->GetAuthGameMode()->GameSession->KickPlayer(PlayerState->GetPlayerController(), Reason);
}

void UWpLobbyPlayerListEntryWidget::OnPlayerSettingsUpdated()
{
	PlayerName->SetText(FText::FromString(PlayerState->GetName()));
	BelligerentId->SetText(FText::FromString(FString::FromInt(PlayerState->GetBelligerentId())));
	TeamId->SetText(FText::FromString(FString::FromInt(PlayerState->GetTeamId())));
	ColorText->SetText(FText::FromString(PlayerState->GetColor().ToString()));
	Status->SetText(FText::FromString(PlayerState->IsReady() ? TEXT("Ready") : TEXT("Not Ready")));
}

UWpLobbyPlayerListWidget::UWpLobbyPlayerListWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
}

void UWpLobbyPlayerListWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ensureAlways(EntryClass);

	List->ClearChildren();

	AWpLobbyGameState* const GameState = GetWorld()->GetGameState<AWpLobbyGameState>();
	GameState->OnPlayerLoggedIn().AddUObject(this, &UWpLobbyPlayerListWidget::OnPlayerLoggedIn);
	GameState->OnPlayerLoggedOut().AddUObject(this, &UWpLobbyPlayerListWidget::OnPlayerLoggedOut);

	if (GetWorld()->GetAuthGameMode())
	{
		OnPlayerLoggedIn(GetOwningPlayer()->PlayerState);
	}
}

void UWpLobbyPlayerListWidget::OnPlayerLoggedIn(APlayerState* const PlayerState)
{
	const bool bWorldIsTearingDown = GetOwningPlayer()->GetPawn() == nullptr;
	if (bWorldIsTearingDown)
	{
		return;
	}

	UWpLobbyPlayerListEntryWidget* const Entry = WidgetTree->ConstructWidget<UWpLobbyPlayerListEntryWidget>(EntryClass);
	Entry->SetPlayerState(Cast<AWpLobbyPlayerState>(PlayerState));
	List->AddChildToVerticalBox(Entry);
}

void UWpLobbyPlayerListWidget::OnPlayerLoggedOut(const APlayerState* const PlayerState)
{
	for (UWidget* const Child : List->GetAllChildren())
	{
		UWpLobbyPlayerListEntryWidget* const Entry = Cast<UWpLobbyPlayerListEntryWidget>(Child);
		if (Entry->GetPlayerState() == PlayerState)
		{
			List->RemoveChild(Child);
			break;
		}
	}
}
