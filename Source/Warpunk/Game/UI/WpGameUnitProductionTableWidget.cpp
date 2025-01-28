#include "WpGameUnitProductionTableWidget.h"

#include "Warpunk/Game/Order/WpOrder.h"
#include "Warpunk/Game/Order/WpOrderSenderComponent.h"
#include "Warpunk/Game/Player/WpStrategicViewPawn.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"

const FName UWpGameUnitProductionTableEntryWidget::CancelUnitInputKeyName = TEXT("LeftShift");

UWpGameUnitProductionTableEntryWidget::UWpGameUnitProductionTableEntryWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
}

void UWpGameUnitProductionTableEntryWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const APlayerController* const PlayerController = GetPlayerContext().GetPlayerController();
	AWpStrategicViewPawn* const Pawn = PlayerController->GetPawn<AWpStrategicViewPawn>();
	UWpOrderSenderComponent* const OrderSenderComponent = Pawn->GetOrderSenderComponent();
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

	Name->SetText(FText::FromString(FString::FromInt(UnitConfigId)));
	Number->SetText(FText{});
}

void UWpGameUnitProductionTableEntryWidget::OnPressed()
{
	const APlayerController* const PlayerController = GetPlayerContext().GetPlayerController();
	AWpStrategicViewPawn* const Pawn = PlayerController->GetPawn<AWpStrategicViewPawn>();
	UWpOrderSenderComponent* const OrderSenderComponent = Pawn->GetOrderSenderComponent();
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

	const APlayerController* const PlayerController = GetPlayerContext().GetPlayerController();
	AWpStrategicViewPawn* const Pawn = PlayerController->GetPawn<AWpStrategicViewPawn>();
	UWpOrderSenderComponent* const OrderSenderComponent = Pawn->GetOrderSenderComponent();
	OrderSenderComponent->OnAvailableOrdersChanged().AddUObject(this, &UWpGameUnitProductionTableWidget::OnAvailableOrdersChanged);
}

void UWpGameUnitProductionTableWidget::OnAvailableOrdersChanged(const TSet<EWpOrderType>& AvailableOrders)
{
	SetVisibility(AvailableOrders.Contains(EWpOrderType::ProduceUnit) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}
