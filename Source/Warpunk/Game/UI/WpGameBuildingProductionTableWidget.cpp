#include "WpGameBuildingProductionTableWidget.h"

#include "Warpunk/Game/Order/WpOrder.h"
#include "Warpunk/Game/Order/WpOrderSenderComponent.h"
#include "Warpunk/Game/Player/WpGamePlayerController.h"

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
	AWpGamePlayerController* const PlayerController = GetPlayerContext().GetPlayerController<AWpGamePlayerController>();
	UWpOrderSenderComponent* const OrderSenderComponent = PlayerController->GetOrderSenderComponent();
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

	for (const uint32 BuildingConfigId : BuildingConfigIds)
	{
		UWpGameBuildingProductionTableEntryWidget* const Entry = WidgetTree->ConstructWidget<UWpGameBuildingProductionTableEntryWidget>(EntryClass);
		Entry->SetBuildingConfigId(BuildingConfigId);
		Table->AddChildToHorizontalBox(Entry);
	}

	AWpGamePlayerController* const PlayerController = GetPlayerContext().GetPlayerController<AWpGamePlayerController>();
	UWpOrderSenderComponent* const OrderSenderComponent = PlayerController->GetOrderSenderComponent();
	OrderSenderComponent->OnAvailableOrdersChanged().AddUObject(this, &UWpGameBuildingProductionTableWidget::OnAvailableOrdersChanged);
}

void UWpGameBuildingProductionTableWidget::OnAvailableOrdersChanged(const TSet<EWpOrderType>& AvailableOrders)
{
	SetVisibility(AvailableOrders.Contains(EWpOrderType::ProduceBuilding) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}
