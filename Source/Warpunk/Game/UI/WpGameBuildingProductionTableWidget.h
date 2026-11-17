#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"

#include "WpGameBuildingProductionTableWidget.generated.h"

enum class EWpOrderType : uint8;

class UButton;
class UHorizontalBox;
class UImage;
class UTextBlock;

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class UWpGameBuildingProductionTableEntryWidget final : public UUserWidget
{
	GENERATED_BODY()

public:
	UWpGameBuildingProductionTableEntryWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void SetBuildingConfigId(const uint32 InBuildingConfigId);

private:
	UFUNCTION()
	void OnPressed();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Portrait = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text = nullptr;

	uint32 BuildingConfigId = 0;
};

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class UWpGameBuildingProductionTableWidget final : public UUserWidget
{
	GENERATED_BODY()

public:
	UWpGameBuildingProductionTableWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;

private:
	void OnAvailableOrdersChanged(const TSet<EWpOrderType>& AvailableOrders);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> Table = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UWpGameBuildingProductionTableEntryWidget> EntryClass = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TArray<uint32> BuildingConfigIds = {};
};
