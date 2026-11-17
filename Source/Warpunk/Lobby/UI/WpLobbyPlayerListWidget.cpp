#include "WpLobbyPlayerListWidget.h"

#include "Warpunk/Lobby/GameMode/WpLobbyGameState.h"
#include "Warpunk/Lobby/Player/WpLobbyPlayerController.h"
#include "Warpunk/Lobby/Player/WpLobbyPlayerState.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "GameFramework/GameSession.h"

namespace
{
	struct FLobbyColor
	{
		const TCHAR* Name;
		FColor Color;
	};

	const FLobbyColor LobbyColors[] = {
		{TEXT("Teal"), FColor{62, 224, 197}},
		{TEXT("Red"), FColor{226, 91, 74}},
		{TEXT("Gold"), FColor{240, 196, 74}},
		{TEXT("Blue"), FColor{90, 154, 232}},
		{TEXT("Purple"), FColor{186, 120, 232}},
		{TEXT("Green"), FColor{61, 220, 151}},
		{TEXT("Orange"), FColor{232, 148, 74}},
		{TEXT("White"), FColor{226, 226, 232}},
	};

	FString ColorNameFor(const FColor& Color)
	{
		const FLobbyColor* Best = &LobbyColors[0];
		int32 BestDistance = MAX_int32;
		for (const FLobbyColor& Choice : LobbyColors)
		{
			const int32 Distance = FMath::Abs(Choice.Color.R - Color.R) + FMath::Abs(Choice.Color.G - Color.G) + FMath::Abs(Choice.Color.B - Color.B);
			if (Distance < BestDistance)
			{
				BestDistance = Distance;
				Best = &Choice;
			}
		}

		return Best->Name;
	}
}

UWpLobbyPlayerListEntryWidget::UWpLobbyPlayerListEntryWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
}

void UWpLobbyPlayerListEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BelligerentId->SetIsReadOnly(true);
	TeamId->SetIsReadOnly(true);
	BelligerentDown->OnPressed.AddDynamic(this, &UWpLobbyPlayerListEntryWidget::OnBelligerentDown);
	BelligerentUp->OnPressed.AddDynamic(this, &UWpLobbyPlayerListEntryWidget::OnBelligerentUp);
	TeamDown->OnPressed.AddDynamic(this, &UWpLobbyPlayerListEntryWidget::OnTeamDown);
	TeamUp->OnPressed.AddDynamic(this, &UWpLobbyPlayerListEntryWidget::OnTeamUp);
	ColorList->OnSelectionChanged.AddDynamic(this, &UWpLobbyPlayerListEntryWidget::OnColorSelected);
	EnsureColorOptions();

	if (GetWorld()->GetAuthGameMode())
	{
		KickButton->OnPressed.AddDynamic(this, &UWpLobbyPlayerListEntryWidget::OnKickPressed);
	}
	else
	{
		KickButton->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UWpLobbyPlayerListEntryWidget::SetPlayerState(AWpLobbyPlayerState* const InPlayerState, const uint32 Slot)
{
	PlayerState = InPlayerState;

	APlayerController* const PlayerController = PlayerState->GetPlayerController();

	const bool bCanEdit = PlayerController && PlayerController->IsLocalController();
	if (!bCanEdit)
	{
		BelligerentId->SetVisibility(ESlateVisibility::HitTestInvisible);
		TeamId->SetVisibility(ESlateVisibility::HitTestInvisible);
		ColorList->SetVisibility(ESlateVisibility::HitTestInvisible);
		BelligerentDown->SetVisibility(ESlateVisibility::Hidden);
		BelligerentUp->SetVisibility(ESlateVisibility::Hidden);
		TeamDown->SetVisibility(ESlateVisibility::Hidden);
		TeamUp->SetVisibility(ESlateVisibility::Hidden);
	}
	else
	{
		AWpLobbyPlayerController* const LocalController = Cast<AWpLobbyPlayerController>(PlayerController);
		if ((PlayerState->GetBelligerentId() == 0) && LocalController)
		{
			LocalController->ServerSetBelligerentId(Slot);
		}
		if ((PlayerState->GetTeamId() == 0) && LocalController)
		{
			LocalController->ServerSetTeamId(Slot);
		}
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

void UWpLobbyPlayerListEntryWidget::OnBelligerentDown()
{
	StepId(true, -1);
}

void UWpLobbyPlayerListEntryWidget::OnBelligerentUp()
{
	StepId(true, 1);
}

void UWpLobbyPlayerListEntryWidget::OnTeamDown()
{
	StepId(false, -1);
}

void UWpLobbyPlayerListEntryWidget::OnTeamUp()
{
	StepId(false, 1);
}

void UWpLobbyPlayerListEntryWidget::StepId(const bool bBelligerent, const int32 Delta)
{
	AWpLobbyPlayerController* const InnerPlayerController = Cast<AWpLobbyPlayerController>(PlayerState->GetPlayerController());
	if (!InnerPlayerController)
	{
		return;
	}

	const int32 Current = static_cast<int32>(bBelligerent ? PlayerState->GetBelligerentId() : PlayerState->GetTeamId());
	const uint32 Next = static_cast<uint32>(FMath::Max(1, Current + Delta));
	if (bBelligerent)
	{
		InnerPlayerController->ServerSetBelligerentId(Next);
	}
	else
	{
		InnerPlayerController->ServerSetTeamId(Next);
	}
}

void UWpLobbyPlayerListEntryWidget::EnsureColorOptions()
{
	if (ColorList->GetOptionCount() > 0)
	{
		return;
	}

	ColorList->AddOption(TEXT("Teal"));
	ColorList->AddOption(TEXT("Red"));
	ColorList->AddOption(TEXT("Gold"));
	ColorList->AddOption(TEXT("Blue"));
	ColorList->AddOption(TEXT("Purple"));
	ColorList->AddOption(TEXT("Green"));
	ColorList->AddOption(TEXT("Orange"));
	ColorList->AddOption(TEXT("White"));
}

void UWpLobbyPlayerListEntryWidget::OnColorSelected(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	static_cast<void>(SelectionType);
	if (bApplyingColor || SelectedItem.IsEmpty() || !PlayerState)
	{
		return;
	}

	FColor Color = FColor{62, 224, 197};
	if (SelectedItem == TEXT("Red"))
	{
		Color = FColor{226, 91, 74};
	}
	else if (SelectedItem == TEXT("Gold"))
	{
		Color = FColor{240, 196, 74};
	}
	else if (SelectedItem == TEXT("Blue"))
	{
		Color = FColor{90, 154, 232};
	}
	else if (SelectedItem == TEXT("Purple"))
	{
		Color = FColor{186, 120, 232};
	}
	else if (SelectedItem == TEXT("Green"))
	{
		Color = FColor{61, 220, 151};
	}
	else if (SelectedItem == TEXT("Orange"))
	{
		Color = FColor{232, 148, 74};
	}
	else if (SelectedItem == TEXT("White"))
	{
		Color = FColor{226, 226, 232};
	}

	AWpLobbyPlayerController* const InnerPlayerController = Cast<AWpLobbyPlayerController>(PlayerState->GetPlayerController());
	if (InnerPlayerController)
	{
		InnerPlayerController->ServerSetColor(Color);
	}
}

void UWpLobbyPlayerListEntryWidget::OnKickPressed()
{
	static const FText Reason = FText::FromString(TEXT("Kicked by host"));
	GetWorld()->GetAuthGameMode()->GameSession->KickPlayer(PlayerState->GetPlayerController(), Reason);
}

void UWpLobbyPlayerListEntryWidget::OnPlayerSettingsUpdated()
{
	EnsureColorOptions();
	PlayerName->SetText(FText::FromString(PlayerState->GetName()));
	BelligerentId->SetText(FText::FromString(FString::FromInt(FMath::Max(1u, PlayerState->GetBelligerentId()))));
	TeamId->SetText(FText::FromString(FString::FromInt(FMath::Max(1u, PlayerState->GetTeamId()))));
	const FColor PlayerColor = PlayerState->GetColor();
	const FString ColorName = ColorNameFor(PlayerColor);
	bApplyingColor = true;
	ColorList->SetSelectedOption(ColorName);
	bApplyingColor = false;
	PlayerName->SetColorAndOpacity(FSlateColor{FLinearColor{PlayerColor}});
	Status->SetText(FText::FromString(PlayerState->IsReady() ? TEXT("Ready") : TEXT("Waiting")));
	Status->SetColorAndOpacity(FSlateColor{PlayerState->IsReady() ? FLinearColor{0.24f, 0.86f, 0.59f, 1.0f} : FLinearColor{0.54f, 0.63f, 0.68f, 1.0f}});
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
	const uint32 Slot = static_cast<uint32>(List->GetChildrenCount() + 1);
	Entry->SetPlayerState(Cast<AWpLobbyPlayerState>(PlayerState), Slot);
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
