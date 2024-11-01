#include "WpOrderSelectionWidget.h"

#include "Warpunk/Game/Player/WpStrategicViewPawn.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

UWpOrderSelectionEntryWidget::UWpOrderSelectionEntryWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
}

void UWpOrderSelectionEntryWidget::SetOrderType(const EWpOrderType InOrderType)
{
	ensureAlways(InOrderType != EWpOrderType::Undefined);
	ensureAlways(InOrderType != EWpOrderType::ProduceBuilding);
	ensureAlways(InOrderType != EWpOrderType::ProduceUnit);
	ensureAlways(InOrderType != EWpOrderType::ProductionProxy);

	OrderType = InOrderType;

	Button->OnPressed.AddDynamic(this, &UWpOrderSelectionEntryWidget::OnPressed);

	FString OrderTypeName;
	const FString OrderTypeFullName = UEnum::GetValueAsString(InOrderType);
	OrderTypeFullName.Split(TEXT("::"), nullptr, &OrderTypeName);
	Text->SetText(FText::FromString(OrderTypeName));
}

void UWpOrderSelectionEntryWidget::OnPressed()
{
	const APlayerController* const PlayerController = GetPlayerContext().GetPlayerController();
	AWpStrategicViewPawn* const Pawn = PlayerController->GetPawn<AWpStrategicViewPawn>();
	Pawn->SelectOrderType(OrderType);
}

UWpOrderSelectionListWidget::UWpOrderSelectionListWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
}

void UWpOrderSelectionListWidget::NativeConstruct()
{
	SetVisibility(ESlateVisibility::Collapsed);

	const APlayerController* const PlayerController = GetPlayerContext().GetPlayerController();
	AWpStrategicViewPawn* const Pawn = PlayerController->GetPawn<AWpStrategicViewPawn>();
	Pawn->OnAvailableOrdersChanged.AddUObject(this, &UWpOrderSelectionListWidget::OnAvailableOrdersChanged);
}

void UWpOrderSelectionListWidget::OnAvailableOrdersChanged(const TSet<EWpOrderType>& AvailableOrderTypes)
{
	List->ClearChildren();

	if (AvailableOrderTypes.IsEmpty())
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	TArray<EWpOrderType> SortedOrderTypes = AvailableOrderTypes.Array();
	SortedOrderTypes.RemoveSingle(EWpOrderType::ProduceBuilding);
	SortedOrderTypes.RemoveSingle(EWpOrderType::ProduceUnit);
	SortedOrderTypes.Add(EWpOrderType::Stop);
	SortedOrderTypes.Sort();

	for (const EWpOrderType OrderType : SortedOrderTypes)
	{
		UWpOrderSelectionEntryWidget* const Entry = WidgetTree->ConstructWidget<UWpOrderSelectionEntryWidget>(EntryClass);
		Entry->SetOrderType(OrderType);
		List->AddChildToVerticalBox(Entry);
	}

	SetVisibility(ESlateVisibility::Visible);
}
