#include "WpGameOrderSelectionListWidget.h"

#include "Warpunk/Game/Player/WpStrategicViewPawn.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"

UWpGameOrderSelectionListEntryWidget::UWpGameOrderSelectionListEntryWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
}

void UWpGameOrderSelectionListEntryWidget::SetOrderType(const EWpOrderType InOrderType)
{
	ensureAlways(InOrderType != EWpOrderType::Undefined);
	ensureAlways(InOrderType != EWpOrderType::ProduceBuilding);
	ensureAlways(InOrderType != EWpOrderType::ProduceUnit);
	ensureAlways(InOrderType != EWpOrderType::ProductionProxy);

	OrderType = InOrderType;

	Button->OnPressed.AddDynamic(this, &UWpGameOrderSelectionListEntryWidget::OnPressed);

	FString OrderTypeName;
	const FString OrderTypeFullName = UEnum::GetValueAsString(OrderType);
	OrderTypeFullName.Split(TEXT("::"), nullptr, &OrderTypeName);
	Text->SetText(FText::FromString(OrderTypeName));
}

void UWpGameOrderSelectionListEntryWidget::OnPressed()
{
	const APlayerController* const PlayerController = GetPlayerContext().GetPlayerController();
	AWpStrategicViewPawn* const Pawn = PlayerController->GetPawn<AWpStrategicViewPawn>();
	Pawn->SelectOrderType(OrderType);
}

UWpGameOrderSelectionListWidget::UWpGameOrderSelectionListWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
}

void UWpGameOrderSelectionListWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ensureAlways(EntryClass);

	List->ClearChildren();

	SetVisibility(ESlateVisibility::Collapsed);

	const APlayerController* const PlayerController = GetPlayerContext().GetPlayerController();
	AWpStrategicViewPawn* const Pawn = PlayerController->GetPawn<AWpStrategicViewPawn>();
	Pawn->OnAvailableOrdersChanged().AddUObject(this, &UWpGameOrderSelectionListWidget::OnAvailableOrdersChanged);
}

void UWpGameOrderSelectionListWidget::OnAvailableOrdersChanged(const TSet<EWpOrderType>& AvailableOrderTypes)
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
		UWpGameOrderSelectionListEntryWidget* const Entry = WidgetTree->ConstructWidget<UWpGameOrderSelectionListEntryWidget>(EntryClass);
		Entry->SetOrderType(OrderType);
		List->AddChildToVerticalBox(Entry);
	}

	SetVisibility(ESlateVisibility::Visible);
}
