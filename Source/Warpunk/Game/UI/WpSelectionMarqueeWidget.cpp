#include "WpSelectionMarqueeWidget.h"

#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"

void UWpSelectionMarqueeWidget::SetScreenRect(const FVector2D& From, const FVector2D& To)
{
	const FVector2D Min{FMath::Min(From.X, To.X), FMath::Min(From.Y, To.Y)};
	const FVector2D Max{FMath::Max(From.X, To.X), FMath::Max(From.Y, To.Y)};
	const FVector2D Size = Max - Min;
	if ((Size.X < 6.0f) && (Size.Y < 6.0f))
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	if (UCanvasPanelSlot* const Slot = Cast<UCanvasPanelSlot>(Frame->Slot))
	{
		Slot->SetAnchors(FAnchors{0.0f, 0.0f});
		Slot->SetAlignment(FVector2D::ZeroVector);
		Slot->SetPosition(Min);
		Slot->SetSize(Size);
		Slot->SetAutoSize(false);
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
}
