#include "WpMenuMainWidget.h"

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

	FString MenuStateName;
	const FString MenuStateFullName = UEnum::GetValueAsString(MenuState);
	MenuStateFullName.Split(TEXT("::"), nullptr, &MenuStateName);
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
