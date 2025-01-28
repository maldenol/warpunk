#include "WpLoadingScreen.h"

#include "Warpunk/Global/UI/WpLoadingScreenWidget.h"

void UWpLoadingScreen::EnableLoadingScreen(const bool bEnable)
{
	ExpectedLoadedObjectNum = 0;
	LoadedObjectNum = 0;
	bMapIsLoaded = false;
	Progress = 0.0f;

	if (!LoadingScreenWidget)
	{
		ensureAlways(LoadingScreenWidgetClass);
		LoadingScreenWidget = CreateWidget<UWpLoadingScreenWidget>(GetWorld(), LoadingScreenWidgetClass);
		GEngine->GameViewport->AddViewportWidgetContent(LoadingScreenWidget->TakeWidget(), 100);
	}

	LoadingScreenWidget->EnableLoadingScreen(bEnable);
}

void UWpLoadingScreen::NotifyObjectLoaded()
{
	ensureAlways(!bMapIsLoaded);

	++LoadedObjectNum;

	UpdateProgress();
	CheckLoaded();
}

void UWpLoadingScreen::SetExpectedLoadedObjectNum(const uint32 InExpectedLoadedObjectNum)
{
	ensureAlways(InExpectedLoadedObjectNum != 0);
	ensureAlways(InExpectedLoadedObjectNum >= LoadedObjectNum);
	ensureAlways(ExpectedLoadedObjectNum == 0);

	ExpectedLoadedObjectNum = InExpectedLoadedObjectNum;

	UpdateProgress();
	CheckLoaded();
}

bool UWpLoadingScreen::IsLoaded() const
{
	return bMapIsLoaded;
}

float UWpLoadingScreen::GetProgress() const
{
	return Progress;
}

UWpLoadingScreen::FLoadedEvent& UWpLoadingScreen::OnLoaded()
{
	return LoadedEvent;
}

UWpLoadingScreen::FProgressChangedEvent& UWpLoadingScreen::OnProgressChanged()
{
	return ProgressChangedEvent;
}

void UWpLoadingScreen::CheckLoaded()
{
	if ((ExpectedLoadedObjectNum != 0) && (LoadedObjectNum == ExpectedLoadedObjectNum))
	{
		bMapIsLoaded = true;
		LoadedEvent.Broadcast();
	}
}

void UWpLoadingScreen::UpdateProgress()
{
	Progress = (ExpectedLoadedObjectNum != 0) ? (static_cast<float>(LoadedObjectNum) / static_cast<float>(ExpectedLoadedObjectNum)) : 0.0f;
	ProgressChangedEvent.Broadcast(Progress);
}
