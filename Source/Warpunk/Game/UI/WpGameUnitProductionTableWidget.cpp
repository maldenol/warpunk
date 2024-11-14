#include "WpGameUnitProductionTableWidget.h"

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
	TMap<uint32, uint32> UnitToProduceConfigIds = Pawn->GetUnitToProduceConfigIds();
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
	if (PlayerController->IsInputKeyDown(CancelUnitInputKeyName))
	{
		Pawn->RemoveUnitToProduce(UnitConfigId);
	}
	else
	{
		Pawn->AddUnitToProduce(UnitConfigId);
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

	const APlayerController* const PlayerController = GetPlayerContext().GetPlayerController();
	AWpStrategicViewPawn* const Pawn = PlayerController->GetPawn<AWpStrategicViewPawn>();
	Pawn->OnAvailableOrdersChanged().AddUObject(this, &UWpGameUnitProductionTableWidget::OnAvailableOrdersChanged);

	for (const uint32 UnitConfigId : UnitConfigIds)
	{
		UWpGameUnitProductionTableEntryWidget* const Entry = WidgetTree->ConstructWidget<UWpGameUnitProductionTableEntryWidget>(EntryClass);
		Entry->SetUnitConfigId(UnitConfigId);
		Table->AddChildToHorizontalBox(Entry);
	}
}

void UWpGameUnitProductionTableWidget::OnAvailableOrdersChanged(const TSet<EWpOrderType>& AvailableOrders)
{
	SetVisibility(AvailableOrders.Contains(EWpOrderType::ProduceUnit) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}
