#include "WpGameEntityEnduranceWidget.h"

#include "Warpunk/Game/Endurance/WpEnduranceComponent.h"

#include "Components/ProgressBar.h"

UWpGameEntityEnduranceWidget::UWpGameEntityEnduranceWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
}

void UWpGameEntityEnduranceWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ProgressBar->SetVisibility(ESlateVisibility::Collapsed);
}

void UWpGameEntityEnduranceWidget::SetEnduranceComponent(UWpEnduranceComponent* const EnduranceComponent)
{
	EnduranceComponent->OnCurrentEnduranceChanged().AddUObject(this, &UWpGameEntityEnduranceWidget::OnCurrentEnduranceChanged);
}

void UWpGameEntityEnduranceWidget::OnCurrentEnduranceChanged(const float CurrentEndurance, const float MaxEndurance)
{
	const bool bError = FMath::IsNearlyZero(MaxEndurance) || (CurrentEndurance > MaxEndurance);
	const bool bFull = FMath::IsNearlyEqual(CurrentEndurance, MaxEndurance);
	if (bError || bFull)
	{
		ProgressBar->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	ProgressBar->SetVisibility(ESlateVisibility::Visible);
	ProgressBar->SetPercent(CurrentEndurance / MaxEndurance);
}
