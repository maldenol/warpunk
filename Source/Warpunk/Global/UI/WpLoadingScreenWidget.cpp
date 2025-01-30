#include "WpLoadingScreenWidget.h"

#include "Warpunk/Global/GameInstance/WpGameInstance.h"
#include "Warpunk/Global/LoadingScreen/WpLoadingScreen.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"

UWpLoadingScreenWidget::UWpLoadingScreenWidget(const FObjectInitializer& ObjectInitializer) : UUserWidget(ObjectInitializer)
{
}

void UWpLoadingScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UWpLoadingScreen* const LoadingScreen = GetGameInstance<UWpGameInstance>()->GetLoadingScreen();
	LoadingScreen->OnLoaded().AddUObject(this, &UWpLoadingScreenWidget::OnLoaded);
	LoadingScreen->OnProgressChanged().AddUObject(this, &UWpLoadingScreenWidget::OnProgressChanged);

	SetVisibility(ESlateVisibility::Collapsed);
}

void UWpLoadingScreenWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	UpdateProgress(InDeltaTime);
	UpdateFadeOut(InDeltaTime);
}

void UWpLoadingScreenWidget::NativeDestruct()
{
	UWpLoadingScreen* const LoadingScreen = GetGameInstance<UWpGameInstance>()->GetLoadingScreen();
	LoadingScreen->OnLoaded().RemoveAll(this);
	LoadingScreen->OnProgressChanged().RemoveAll(this);

	Super::NativeDestruct();
}

void UWpLoadingScreenWidget::EnableLoadingScreen(const bool bEnabled)
{
	const bool bWasEnabled = (GetVisibility() == ESlateVisibility::Visible);
	if (bEnabled == bWasEnabled)
	{
		return;
	}

	if (bEnabled)
	{
		SetVisibility(ESlateVisibility::Visible);

		ProgressDelta = 0.0f;
		TargetProgress = 0.0f;
		bIsFadingOut = false;
		CurrentFadeOutTime = 0.0f;
		SetLeafWidgetsOpacity(1.0);
		DarkImage->SetRenderOpacity(0.0f);

		const UWpLoadingScreen* const LoadingScreen = GetGameInstance<UWpGameInstance>()->GetLoadingScreen();
		OnProgressChanged(LoadingScreen->GetProgress());
	}
	else
	{
		bIsFadingOut = true;
	}
}

void UWpLoadingScreenWidget::UpdateProgress(const float DeltaTime)
{
	if (FMath::IsNearlyZero(ProgressDelta))
	{
		return;
	}

	if (DeltaTime < ProgressSmoothingTime)
	{
		ProgressDelta *= 1.0f - DeltaTime / ProgressSmoothingTime;
	}
	else
	{
		ProgressDelta = 0.0f;
	}

	const float CurrentProgress = TargetProgress - ProgressDelta;
	ProgressBar->SetPercent(CurrentProgress);
}

void UWpLoadingScreenWidget::UpdateFadeOut(const float DeltaTime)
{
	if (!bIsFadingOut)
	{
		return;
	}

	CurrentFadeOutTime += DeltaTime;

	const float HalfFadeOutTime = FadeOutTime / 2.0f;
	if (CurrentFadeOutTime < HalfFadeOutTime)
	{
		const float DarkTime = CurrentFadeOutTime;
		const float DarkImageOpacity = DarkTime / HalfFadeOutTime;
		DarkImage->SetRenderOpacity(DarkImageOpacity);
	}
	else
	{
		SetLeafWidgetsOpacity(0.0f);

		const float TransparencyTime = CurrentFadeOutTime - HalfFadeOutTime;
		const float DarkImageOpacity = 1.0f - TransparencyTime / HalfFadeOutTime;
		DarkImage->SetRenderOpacity(DarkImageOpacity);
	}

	if (CurrentFadeOutTime >= FadeOutTime)
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UWpLoadingScreenWidget::SetLeafWidgetsOpacity(const float Opacity)
{
	TArray<UWidget*> Widgets;
	WidgetTree->GetAllWidgets(Widgets);
	for (UWidget* const Widget : Widgets)
	{
		TArray<UWidget*> WidgetChildren;
		UWidgetTree::GetChildWidgets(Widget, WidgetChildren);
		const bool bIsLeafWidget = WidgetChildren.IsEmpty();
		if (bIsLeafWidget)
		{
			Widget->SetRenderOpacity(Opacity);
		}
	}
}

void UWpLoadingScreenWidget::OnProgressChanged(const float Progress)
{
	ProgressDelta += Progress - TargetProgress;
	TargetProgress = Progress;

	ProgressBar->SetVisibility(FMath::IsNearlyZero(Progress) ? ESlateVisibility::Hidden : ESlateVisibility::Visible);
}

void UWpLoadingScreenWidget::OnLoaded()
{
	EnableLoadingScreen(false);
}
