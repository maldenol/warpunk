#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"

#include "WpGameEconomyStatusWidget.generated.h"

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class UWpGameEconomyStatusWidget final : public UUserWidget
{
	GENERATED_BODY()

public:
	UWpGameEconomyStatusWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
