#include "WpGameOrderSelectionListWidget.h"

#include "Warpunk/Game/Order/WpCommandMarker.h"
#include "Warpunk/Game/Order/WpOrderSenderComponent.h"
#include "Warpunk/Game/Player/WpGamePlayerController.h"

#include "Warpunk/Global/Audio/WpAudio.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Engine/Texture2D.h"

namespace
{
	FString OrderLabel(const EWpOrderType OrderType)
	{
		switch (OrderType)
		{
		case EWpOrderType::Move:
			return TEXT("Move");
		case EWpOrderType::Patrol:
			return TEXT("Patrol");
		case EWpOrderType::Assist:
			return TEXT("Assist");
		case EWpOrderType::Attack:
			return TEXT("Attack");
		case EWpOrderType::Strike:
			return TEXT("Strike");
		case EWpOrderType::Repair:
			return TEXT("Repair");
		case EWpOrderType::Reclaim:
			return TEXT("Reclaim");
		case EWpOrderType::InfiniteUnitProductionOn:
			return TEXT("Repeat");
		case EWpOrderType::InfiniteUnitProductionOff:
			return TEXT("Once");
		case EWpOrderType::SelfDestruct:
			return TEXT("Detonate");
		case EWpOrderType::Stop:
			return TEXT("Stop");
		default:
			return TEXT("Order");
		}
	}

	UTexture2D* OrderIcon(const EWpOrderType OrderType)
	{
		const FString Label = OrderLabel(OrderType);
		return LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/Images/Orders/T_%s.T_%s"), *Label, *Label));
	}

	void ApplyOrderButtonColor(UButton* const Button, const FLinearColor& Color)
	{
		FButtonStyle Style = Button->GetStyle();
		const FLinearColor Hovered{
			FMath::Min(Color.R + 0.16f, 1.0f),
			FMath::Min(Color.G + 0.16f, 1.0f),
			FMath::Min(Color.B + 0.16f, 1.0f),
			1.0f};
		const FLinearColor Pressed{Color.R * 0.55f, Color.G * 0.55f, Color.B * 0.55f, 1.0f};
		Style.Normal.TintColor = FSlateColor{Color};
		Style.Hovered.TintColor = FSlateColor{Hovered};
		Style.Pressed.TintColor = FSlateColor{Pressed};
		Style.NormalPadding = FMargin{12.0f, 6.0f};
		Style.PressedPadding = FMargin{12.0f, 9.0f, 12.0f, 3.0f};
		Button->SetStyle(Style);
		Button->SetBackgroundColor(FLinearColor::White);
	}
}

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
	UWpAudioSubsystem::ArmButton(this, Button);

	Text->SetText(FText::FromString(OrderLabel(OrderType)));
	ApplyOrderButtonColor(Button, AWpCommandMarker::ColorForOrder(OrderType));
	if (UTexture2D* const IconTexture = OrderIcon(OrderType))
	{
		Icon->SetBrushFromTexture(IconTexture, false);
	}
}

void UWpGameOrderSelectionListEntryWidget::OnPressed()
{
	AWpGamePlayerController* const PlayerController = GetPlayerContext().GetPlayerController<AWpGamePlayerController>();
	UWpOrderSenderComponent* const OrderSenderComponent = PlayerController->GetOrderSenderComponent();
	OrderSenderComponent->SelectOrderType(OrderType);
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

	AWpGamePlayerController* const PlayerController = GetPlayerContext().GetPlayerController<AWpGamePlayerController>();
	UWpOrderSenderComponent* const OrderSenderComponent = PlayerController->GetOrderSenderComponent();
	OrderSenderComponent->OnAvailableOrdersChanged().AddUObject(this, &UWpGameOrderSelectionListWidget::OnAvailableOrdersChanged);
}

void UWpGameOrderSelectionListWidget::OnAvailableOrdersChanged(const TSet<EWpOrderType>& AvailableOrderTypes)
{
	List->ClearChildren();

	if (AvailableOrderTypes.IsEmpty())
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const TArray<EWpOrderType> SortedOrderTypes = {
		EWpOrderType::Move,
		EWpOrderType::Patrol,
		EWpOrderType::Attack,
		EWpOrderType::Strike,
		EWpOrderType::Assist,
		EWpOrderType::Repair,
		EWpOrderType::Reclaim,
		EWpOrderType::InfiniteUnitProductionOn,
		EWpOrderType::InfiniteUnitProductionOff,
		EWpOrderType::SelfDestruct,
		EWpOrderType::Stop,
	};

	TSet<EWpOrderType> ShownOrderTypes = {};
	for (const EWpOrderType OrderType : SortedOrderTypes)
	{
		if ((OrderType != EWpOrderType::Stop) && !AvailableOrderTypes.Contains(OrderType))
		{
			continue;
		}

		UWpGameOrderSelectionListEntryWidget* const Entry = WidgetTree->ConstructWidget<UWpGameOrderSelectionListEntryWidget>(EntryClass);
		Entry->SetOrderType(OrderType);
		List->AddChildToVerticalBox(Entry);
		ShownOrderTypes.Add(OrderType);
	}

	for (const EWpOrderType OrderType : AvailableOrderTypes)
	{
		if (ShownOrderTypes.Contains(OrderType) || (OrderType == EWpOrderType::ProduceBuilding) || (OrderType == EWpOrderType::ProduceUnit) || (OrderType == EWpOrderType::ProductionProxy))
		{
			continue;
		}

		UWpGameOrderSelectionListEntryWidget* const Entry = WidgetTree->ConstructWidget<UWpGameOrderSelectionListEntryWidget>(EntryClass);
		Entry->SetOrderType(OrderType);
		List->AddChildToVerticalBox(Entry);
	}

	SetVisibility(ESlateVisibility::Visible);
}
