#include "WpBuildingProductionWidget.h"

#include "Warpunk/Game/Player/WpStrategicViewPawn.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"

UWpBuildingProductionEntryWidget::UWpBuildingProductionEntryWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
}

void UWpBuildingProductionEntryWidget::SetBuildingConfigID(const uint32 InBuildingConfigID)
{
	ensureAlways(InBuildingConfigID > 0);

	BuildingConfigID = InBuildingConfigID;

	Button->OnPressed.AddDynamic(this, &UWpBuildingProductionEntryWidget::OnPressed);

	Text->SetText(FText::FromString(FString::FromInt(BuildingConfigID)));
}

void UWpBuildingProductionEntryWidget::OnPressed()
{
	const APlayerController* const PlayerController = GetPlayerContext().GetPlayerController();
	AWpStrategicViewPawn* const Pawn = PlayerController->GetPawn<AWpStrategicViewPawn>();
	Pawn->SelectBuildingToProduce(BuildingConfigID);
}

UWpBuildingProductionTableWidget::UWpBuildingProductionTableWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
}

void UWpBuildingProductionTableWidget::NativeConstruct()
{
	SetVisibility(ESlateVisibility::Collapsed);

	const APlayerController* const PlayerController = GetPlayerContext().GetPlayerController();
	AWpStrategicViewPawn* const Pawn = PlayerController->GetPawn<AWpStrategicViewPawn>();
	Pawn->OnAvailableOrdersChanged.AddUObject(this, &UWpBuildingProductionTableWidget::OnAvailableOrdersChanged);

	for (const uint32 BuildingConfigID : BuildingConfigIDs)
	{
		UWpBuildingProductionEntryWidget* const Entry = WidgetTree->ConstructWidget<UWpBuildingProductionEntryWidget>(EntryClass);
		Entry->SetBuildingConfigID(BuildingConfigID);
		Table->AddChildToHorizontalBox(Entry);
	}
}

void UWpBuildingProductionTableWidget::OnAvailableOrdersChanged(const TSet<EWpOrderType>& AvailableOrders)
{
	SetVisibility(AvailableOrders.Contains(EWpOrderType::ProduceBuilding) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}
