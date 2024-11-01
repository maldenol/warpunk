#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"

#include "WpEntityEnduranceWidget.generated.h"

class FWpEndurance;

class UProgressBar;

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class UWpEntityEnduranceWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWpEntityEnduranceWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void SetEndurance(FWpEndurance* const Endurance);

private:
	void OnEnduranceChanged(const float Endurance, const float MaxEndurance);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar = nullptr;
};
