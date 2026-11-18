#include "WpGameBuildingProductionTableWidget.h"

#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Order/WpOrder.h"
#include "Warpunk/Game/Order/WpOrderSenderComponent.h"
#include "Warpunk/Game/Player/WpGamePlayerController.h"
#include "Warpunk/Global/GameInstance/WpGameInstance.h"

#include "Warpunk/Global/Audio/WpAudio.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

UWpGameBuildingProductionTableEntryWidget::UWpGameBuildingProductionTableEntryWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
}

void UWpGameBuildingProductionTableEntryWidget::SetBuildingConfigId(const uint32 InBuildingConfigId)
{
	ensureAlways(InBuildingConfigId > 0);

	BuildingConfigId = InBuildingConfigId;

	Button->OnPressed.AddDynamic(this, &UWpGameBuildingProductionTableEntryWidget::OnPressed);
	UWpAudioSubsystem::ArmButton(this, Button);

	const FWpEntityConfig EntityConfig = GetWorld()->GetGameInstance<UWpGameInstance>()->GetEntityConfig(BuildingConfigId);
	const FString DisplayName = EntityConfig.Name.IsEmpty() ? FString::FromInt(BuildingConfigId) : EntityConfig.Name;
	Text->SetAutoWrapText(true);
	Text->SetJustification(ETextJustify::Center);
	Text->SetText(FText::FromString(DisplayName));

	const FString TexturePath = FString::Printf(TEXT("/Game/Models/Entities/%s/T_%s.T_%s"), *DisplayName, *DisplayName, *DisplayName);
	if (UTexture2D* const PortraitTexture = LoadObject<UTexture2D>(nullptr, *TexturePath))
	{
		Portrait->SetBrushFromTexture(PortraitTexture, false);
	}
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
