#include "WpGameEntityEnduranceWidget.h"

#include "Warpunk/Game/Endurance/WpEndurance.h"

#include "Components/ProgressBar.h"

UWpGameEntityEnduranceWidget::UWpGameEntityEnduranceWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
}

void UWpGameEntityEnduranceWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ProgressBar->SetVisibility(ESlateVisibility::Collapsed);
}

void UWpGameEntityEnduranceWidget::SetEndurance(FWpEndurance* const Endurance)
{
	Endurance->OnCurrentEnduranceChanged().AddUObject(this, &UWpGameEntityEnduranceWidget::OnCurrentEnduranceChanged);
}

void UWpGameEntityEnduranceWidget::OnCurrentEnduranceChanged(const float CurrentEndurance, const float MaxEndurance)
{
	ensureAlways(CurrentEndurance >= 0.0f && CurrentEndurance <= MaxEndurance);

	if (CurrentEndurance == MaxEndurance)
	{
		ProgressBar->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	ProgressBar->SetVisibility(ESlateVisibility::Visible);
	ProgressBar->SetPercent(CurrentEndurance / MaxEndurance);
}
