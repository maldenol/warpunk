#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"

#include "WpGameHUDWidget.generated.h"

class UWpBuildingProductionTableWidget;
class UWpOrderSelectionListWidget;
class UWpUnitProductionTableWidget;

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class UWpGameHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWpGameHUDWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWpOrderSelectionListWidget> OrderSelectionWidget = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWpBuildingProductionTableWidget> BuildingProductionWidget = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWpUnitProductionTableWidget> UnitProductionWidget = nullptr;
};
