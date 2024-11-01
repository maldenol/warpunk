#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"

#include "WpEconomyStatusWidget.generated.h"

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class UWpEconomyStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWpEconomyStatusWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
