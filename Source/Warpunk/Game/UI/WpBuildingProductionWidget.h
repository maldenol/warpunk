#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"

#include "WpBuildingProductionWidget.generated.h"

enum class EWpOrderType : uint8;

class UButton;
class UHorizontalBox;
class UTextBlock;

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class UWpBuildingProductionEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWpBuildingProductionEntryWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void SetBuildingConfigID(const uint32 InBuildingConfigID);

private:
	UFUNCTION()
	void OnPressed();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text = nullptr;

	uint32 BuildingConfigID = 0;
};

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class UWpBuildingProductionTableWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWpBuildingProductionTableWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;

private:
	void OnAvailableOrdersChanged(const TSet<EWpOrderType>& AvailableOrders);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> Table = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UWpBuildingProductionEntryWidget> EntryClass = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TArray<uint32> BuildingConfigIDs = {};
};
