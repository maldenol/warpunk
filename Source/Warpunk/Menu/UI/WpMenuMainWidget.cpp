#include "WpMenuMainWidget.h"

#include "Warpunk/Global/Audio/WpAudio.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

UWpMenuMainEntryWidget::UWpMenuMainEntryWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
}

void UWpMenuMainEntryWidget::SetMenuState(const EWpMenuState InMenuState)
{
	MenuState = InMenuState;

	Button->OnPressed.AddDynamic(this, &UWpMenuMainEntryWidget::OnPressed);
	UWpAudioSubsystem::ArmButton(this, Button);

	FString MenuStateName;
	switch (MenuState)
	{
	case EWpMenuState::Singleplayer:
		MenuStateName = TEXT("Single Player");
		break;
	case EWpMenuState::Multiplayer:
		MenuStateName = TEXT("Multiplayer");
		break;
	case EWpMenuState::Exit:
		MenuStateName = TEXT("Exit");
		break;
	default:
		MenuStateName = TEXT("Menu");
		break;
	}
	Text->SetText(FText::FromString(MenuStateName));
}

void UWpMenuMainEntryWidget::OnPressed()
{
	AWpMenuHUD* const MenuHUD = GetWorld()->GetFirstPlayerController()->GetHUD<AWpMenuHUD>();
	MenuHUD->SetMenuState(MenuState);
}

UWpMenuMainWidget::UWpMenuMainWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
}

void UWpMenuMainWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ensureAlways(EntryClass);

	List->ClearChildren();

	AddEntry(EWpMenuState::Singleplayer);
	AddEntry(EWpMenuState::Multiplayer);
	AddEntry(EWpMenuState::Exit);
}

void UWpMenuMainWidget::AddEntry(const EWpMenuState MenuState)
{
	UWpMenuMainEntryWidget* const Entry = WidgetTree->ConstructWidget<UWpMenuMainEntryWidget>(EntryClass);
	Entry->SetMenuState(MenuState);
	List->AddChildToVerticalBox(Entry);
}
