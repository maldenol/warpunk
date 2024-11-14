#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"

#include "WpGameHUDWidget.generated.h"

class UWpGameBuildingProductionTableWidget;
class UWpGameOrderSelectionListWidget;
class UWpGameUnitProductionTableWidget;

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class UWpGameHUDWidget final : public UUserWidget
{
	GENERATED_BODY()

public:
	UWpGameHUDWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWpGameOrderSelectionListWidget> GameOrderSelectionList = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWpGameBuildingProductionTableWidget> GameBuildingProductionTable = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWpGameUnitProductionTableWidget> GameUnitProductionTable = nullptr;
};
