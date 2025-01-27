#include "WpGameMapLoadWidget.h"

#include "Warpunk/Game/MapLoad/WpMapLoadManager.h"

#include "Components/ProgressBar.h"

UWpGameMapLoadWidget::UWpGameMapLoadWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
}

void UWpGameMapLoadWidget::SetMapLoadManager(AWpMapLoadManager* const MapLoadManager)
{
	MapLoadManager->OnProgressChanged().AddUObject(this, &UWpGameMapLoadWidget::OnProgressChanged);
	OnProgressChanged(MapLoadManager->GetProgress());

	MapLoadManager->OnMapLoaded().AddUObject(this, &UWpGameMapLoadWidget::OnMapLoaded);
	if (MapLoadManager->IsMapLoaded())
	{
		OnMapLoaded();
	}
}

void UWpGameMapLoadWidget::OnProgressChanged(const float Progress)
{
	ProgressBar->SetPercent(Progress);
}

void UWpGameMapLoadWidget::OnMapLoaded()
{
	SetVisibility(ESlateVisibility::Collapsed);
}
