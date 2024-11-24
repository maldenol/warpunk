#include "WpGameBuildingProductionTableWidget.h"

#include "Warpunk/Game/Order/WpOrder.h"
#include "Warpunk/Game/Order/WpOrderSenderComponent.h"
#include "Warpunk/Game/Player/WpStrategicViewPawn.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"

UWpGameBuildingProductionTableEntryWidget::UWpGameBuildingProductionTableEntryWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
}

void UWpGameBuildingProductionTableEntryWidget::SetBuildingConfigId(const uint32 InBuildingConfigId)
{
	ensureAlways(InBuildingConfigId > 0);

	BuildingConfigId = InBuildingConfigId;

	Button->OnPressed.AddDynamic(this, &UWpGameBuildingProductionTableEntryWidget::OnPressed);

	Text->SetText(FText::FromString(FString::FromInt(BuildingConfigId)));
}

void UWpGameBuildingProductionTableEntryWidget::OnPressed()
{
	const APlayerController* const PlayerController = GetPlayerContext().GetPlayerController();
	AWpStrategicViewPawn* const Pawn = PlayerController->GetPawn<AWpStrategicViewPawn>();
	UWpOrderSenderComponent* const OrderSenderComponent = Pawn->GetOrderSenderComponent();
	OrderSenderComponent->SelectBuildingToProduce(BuildingConfigId);
}

UWpGameBuildingProductionTableWidget::UWpGameBuildingProductionTableWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
}

void UWpGameBuildingProductionTableWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ensureAlways(EntryClass);

	Table->ClearChildren();

	SetVisibility(ESlateVisibility::Collapsed);

	const APlayerController* const PlayerController = GetPlayerContext().GetPlayerController();
	AWpStrategicViewPawn* const Pawn = PlayerController->GetPawn<AWpStrategicViewPawn>();
	UWpOrderSenderComponent* const OrderSenderComponent = Pawn->GetOrderSenderComponent();
	OrderSenderComponent->OnAvailableOrdersChanged().AddUObject(this, &UWpGameBuildingProductionTableWidget::OnAvailableOrdersChanged);

	for (const uint32 BuildingConfigId : BuildingConfigIds)
	{
		UWpGameBuildingProductionTableEntryWidget* const Entry = WidgetTree->ConstructWidget<UWpGameBuildingProductionTableEntryWidget>(EntryClass);
		Entry->SetBuildingConfigId(BuildingConfigId);
		Table->AddChildToHorizontalBox(Entry);
	}
}

void UWpGameBuildingProductionTableWidget::OnAvailableOrdersChanged(const TSet<EWpOrderType>& AvailableOrders)
{
	SetVisibility(AvailableOrders.Contains(EWpOrderType::ProduceBuilding) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}
