#include "WpEntityEnduranceWidget.h"

#include "Warpunk/Game/Endurance/WpEndurance.h"

#include "Components/ProgressBar.h"

UWpEntityEnduranceWidget::UWpEntityEnduranceWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
}

void UWpEntityEnduranceWidget::SetEndurance(FWpEndurance* const Endurance)
{
	Endurance->OnEnduranceChanged.AddUObject(this, &UWpEntityEnduranceWidget::OnEnduranceChanged);
	OnEnduranceChanged(Endurance->GetCurrentEndurance(), Endurance->GetMaxEndurance());
}

void UWpEntityEnduranceWidget::OnEnduranceChanged(const float Endurance, const float MaxEndurance)
{
	ensureAlways(Endurance >= 0.0f && Endurance <= MaxEndurance);

	if (Endurance == MaxEndurance)
	{
		ProgressBar->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	ProgressBar->SetVisibility(ESlateVisibility::Visible);
	ProgressBar->SetPercent(Endurance / MaxEndurance);
}
