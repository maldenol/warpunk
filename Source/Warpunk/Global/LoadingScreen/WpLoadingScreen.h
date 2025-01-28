#pragma once

#include "CoreMinimal.h"

#include "WpLoadingScreen.generated.h"

class UWpLoadingScreenWidget;

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class UWpLoadingScreen final : public UObject
{
	GENERATED_BODY()

public:
	DECLARE_EVENT(UWpLoadingScreen, FLoadedEvent);

	DECLARE_EVENT_OneParam(UWpLoadingScreen, FProgressChangedEvent, float);

	void EnableLoadingScreen(const bool bEnable);

	void NotifyObjectLoaded();
	void SetExpectedLoadedObjectNum(const uint32 InExpectedLoadedObjectNum);

	bool IsLoaded() const;
	float GetProgress() const;

	FLoadedEvent& OnLoaded();
	FProgressChangedEvent& OnProgressChanged();

private:
	void CheckLoaded();
	void UpdateProgress();

	uint32 ExpectedLoadedObjectNum = 0;
	uint32 LoadedObjectNum = 0;

	bool bMapIsLoaded = false;
	float Progress = 0.0f;

	FLoadedEvent LoadedEvent = {};
	FProgressChangedEvent ProgressChangedEvent = {};

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UWpLoadingScreenWidget> LoadingScreenWidgetClass = nullptr;
	UPROPERTY(VisibleInstanceOnly)
	UWpLoadingScreenWidget* LoadingScreenWidget = nullptr;
};
