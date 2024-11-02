#include "WpUnitProductionWidget.h"

#include "Warpunk/Game/Player/WpStrategicViewPawn.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/TextBlock.h"

const FName UWpUnitProductionEntryWidget::CancelUnitInputKeyName = TEXT("LeftShift");

UWpUnitProductionEntryWidget::UWpUnitProductionEntryWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
}

void UWpUnitProductionEntryWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const APlayerController* const PlayerController = GetPlayerContext().GetPlayerController();
	AWpStrategicViewPawn* const Pawn = PlayerController->GetPawn<AWpStrategicViewPawn>();
	TMap<uint32, uint32> UnitToProduceConfigIDs = Pawn->GetUnitToProduceConfigIDs();
	if (const uint32* const UnitToProduceNumber = UnitToProduceConfigIDs.Find(UnitConfigID))
	{
		Number->SetText(FText::FromString(FString::FromInt(*UnitToProduceNumber)));
	}
	else
	{
		Number->SetText(FText{});
	}
}

void UWpUnitProductionEntryWidget::SetUnitConfigID(const uint32 InUnitConfigID)
{
	ensureAlways(InUnitConfigID > 0);

	UnitConfigID = InUnitConfigID;

	Button->OnPressed.AddDynamic(this, &UWpUnitProductionEntryWidget::OnPressed);

	Name->SetText(FText::FromString(FString::FromInt(UnitConfigID)));
	Number->SetText(FText{});
}

void UWpUnitProductionEntryWidget::OnPressed()
{
	const APlayerController* const PlayerController = GetPlayerContext().GetPlayerController();
	AWpStrategicViewPawn* const Pawn = PlayerController->GetPawn<AWpStrategicViewPawn>();
	if (PlayerController->IsInputKeyDown(CancelUnitInputKeyName))
	{
		Pawn->RemoveUnitToProduce(UnitConfigID);
	}
	else
	{
		Pawn->AddUnitToProduce(UnitConfigID);
	}
}

UWpUnitProductionTableWidget::UWpUnitProductionTableWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
}

void UWpUnitProductionTableWidget::NativeConstruct()
{
	SetVisibility(ESlateVisibility::Collapsed);

	const APlayerController* const PlayerController = GetPlayerContext().GetPlayerController();
	AWpStrategicViewPawn* const Pawn = PlayerController->GetPawn<AWpStrategicViewPawn>();
	Pawn->OnAvailableOrdersChanged.AddUObject(this, &UWpUnitProductionTableWidget::OnAvailableOrdersChanged);

	for (const uint32 UnitConfigID : UnitConfigIDs)
	{
		UWpUnitProductionEntryWidget* const Entry = WidgetTree->ConstructWidget<UWpUnitProductionEntryWidget>(EntryClass);
		Entry->SetUnitConfigID(UnitConfigID);
		Table->AddChildToHorizontalBox(Entry);
	}
}

void UWpUnitProductionTableWidget::OnAvailableOrdersChanged(const TSet<EWpOrderType>& AvailableOrders)
{
	SetVisibility(AvailableOrders.Contains(EWpOrderType::ProduceUnit) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}
