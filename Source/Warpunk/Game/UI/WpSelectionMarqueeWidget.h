#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"

#include "WpSelectionMarqueeWidget.generated.h"

class UImage;

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class UWpSelectionMarqueeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetScreenRect(const FVector2D& From, const FVector2D& To);

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Frame = nullptr;
};
