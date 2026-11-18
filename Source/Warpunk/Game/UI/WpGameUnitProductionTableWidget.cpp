#include "WpGameUnitProductionTableWidget.h"

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

const FName UWpGameUnitProductionTableEntryWidget::CancelUnitInputKeyName = TEXT("LeftShift");

UWpGameUnitProductionTableEntryWidget::UWpGameUnitProductionTableEntryWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
}

void UWpGameUnitProductionTableEntryWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	AWpGamePlayerController* const PlayerController = GetPlayerContext().GetPlayerController<AWpGamePlayerController>();
	UWpOrderSenderComponent* const OrderSenderComponent = PlayerController->GetOrderSenderComponent();
	TMap<uint32, uint32> UnitToProduceConfigIds = OrderSenderComponent->GetUnitsToProduce();
	if (const uint32* const UnitToProduceNumber = UnitToProduceConfigIds.Find(UnitConfigId))
	{
		Number->SetText(FText::FromString(FString::FromInt(*UnitToProduceNumber)));
	}
	else
	{
		Number->SetText(FText{});
	}
}

void UWpGameUnitProductionTableEntryWidget::SetUnitConfigId(const uint32 InUnitConfigId)
{
	ensureAlways(InUnitConfigId > 0);

	UnitConfigId = InUnitConfigId;

	Button->OnPressed.AddDynamic(this, &UWpGameUnitProductionTableEntryWidget::OnPressed);
	UWpAudioSubsystem::ArmButton(this, Button);

	const FWpEntityConfig EntityConfig = GetWorld()->GetGameInstance<UWpGameInstance>()->GetEntityConfig(UnitConfigId);
	const FString DisplayName = EntityConfig.Name.IsEmpty() ? FString::FromInt(UnitConfigId) : EntityConfig.Name;
	Name->SetAutoWrapText(true);
	Name->SetJustification(ETextJustify::Center);
	Name->SetText(FText::FromString(DisplayName));
	Number->SetText(FText{});

	const FString TexturePath = FString::Printf(TEXT("/Game/Models/Entities/%s/T_%s.T_%s"), *DisplayName, *DisplayName, *DisplayName);
	if (UTexture2D* const PortraitTexture = LoadObject<UTexture2D>(nullptr, *TexturePath))
	{
		Portrait->SetBrushFromTexture(PortraitTexture, false);
	}
}

void UWpGameUnitProductionTableEntryWidget::OnPressed()
{
	AWpGamePlayerController* const PlayerController = GetPlayerContext().GetPlayerController<AWpGamePlayerController>();
	UWpOrderSenderComponent* const OrderSenderComponent = PlayerController->GetOrderSenderComponent();
	if (PlayerController->IsInputKeyDown(CancelUnitInputKeyName))
	{
		OrderSenderComponent->RemoveUnitToProduce(UnitConfigId);
	}
	else
	{
		OrderSenderComponent->AddUnitToProduce(UnitConfigId);
	}
}

UWpGameUnitProductionTableWidget::UWpGameUnitProductionTableWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
}

void UWpGameUnitProductionTableWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ensureAlways(EntryClass);

	Table->ClearChildren();

	SetVisibility(ESlateVisibility::Collapsed);

	for (const uint32 UnitConfigId : UnitConfigIds)
	{
		UWpGameUnitProductionTableEntryWidget* const Entry = WidgetTree->ConstructWidget<UWpGameUnitProductionTableEntryWidget>(EntryClass);
		Entry->SetUnitConfigId(UnitConfigId);
		Table->AddChildToHorizontalBox(Entry);
	}

	AWpGamePlayerController* const PlayerController = GetPlayerContext().GetPlayerController<AWpGamePlayerController>();
	UWpOrderSenderComponent* const OrderSenderComponent = PlayerController->GetOrderSenderComponent();
	OrderSenderComponent->OnAvailableOrdersChanged().AddUObject(this, &UWpGameUnitProductionTableWidget::OnAvailableOrdersChanged);
}

void UWpGameUnitProductionTableWidget::OnAvailableOrdersChanged(const TSet<EWpOrderType>& AvailableOrders)
{
	SetVisibility(AvailableOrders.Contains(EWpOrderType::ProduceUnit) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}
