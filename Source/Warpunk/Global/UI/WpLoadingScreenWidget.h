#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"

#include "WpLoadingScreenWidget.generated.h"

class UImage;
class UProgressBar;

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class UWpLoadingScreenWidget final : public UUserWidget
{
	GENERATED_BODY()

public:
	UWpLoadingScreenWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, const float InDeltaTime) override;
	virtual void NativeDestruct() override;

	void EnableLoadingScreen(const bool bEnabled);

private:
	void UpdateProgress(const float DeltaTime);
	void UpdateFadeOut(const float DeltaTime);

	void SetLeafWidgetsOpacity(const float Opacity);

	void OnProgressChanged(const float Progress);
	void OnLoaded();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> DarkImage = nullptr;

	UPROPERTY(EditDefaultsOnly)
	float ProgressSmoothingTime = 1.0f;

	UPROPERTY(EditDefaultsOnly)
	float FadeOutTime = 3.0f;

	float ProgressDelta = 0.0f;
	float TargetProgress = 0.0f;

	bool bIsFadingOut = false;
	float CurrentFadeOutTime = 0.0f;
};
