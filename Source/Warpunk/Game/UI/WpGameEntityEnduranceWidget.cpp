#include "WpGameEntityEnduranceWidget.h"

#include "Warpunk/Game/Endurance/WpEnduranceComponent.h"

#include "Components/ProgressBar.h"

UWpGameEntityEnduranceWidget::UWpGameEntityEnduranceWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
}

void UWpGameEntityEnduranceWidget::SetEnduranceComponent(UWpEnduranceComponent* const EnduranceComponent)
{
	EnduranceComponent->OnCurrentEnduranceChanged().AddUObject(this, &UWpGameEntityEnduranceWidget::OnCurrentEnduranceChanged);
	OnCurrentEnduranceChanged(EnduranceComponent->GetCurrentEndurance(), EnduranceComponent->GetMaxEndurance());
}

void UWpGameEntityEnduranceWidget::OnCurrentEnduranceChanged(const float CurrentEndurance, const float MaxEndurance)
{
	if (!FMath::IsNearlyZero(MaxEndurance))
	{
		ProgressBar->SetPercent(CurrentEndurance / MaxEndurance);
	}
	else
	{
		ProgressBar->SetPercent(0.0f);
	}
}
