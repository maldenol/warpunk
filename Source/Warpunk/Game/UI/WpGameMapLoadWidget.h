#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"

#include "WpGameMapLoadWidget.generated.h"

class AWpMapLoadManager;

class UProgressBar;

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class UWpGameMapLoadWidget final : public UUserWidget
{
	GENERATED_BODY()

public:
	UWpGameMapLoadWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void SetMapLoadManager(AWpMapLoadManager* const MapLoadManager);

private:
	void OnProgressChanged(const float Progress);
	void OnMapLoaded();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar = nullptr;
};
