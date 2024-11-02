#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"

#include "WpUnitProductionWidget.generated.h"

enum class EWpOrderType : uint8;

class UButton;
class UHorizontalBox;
class UTextBlock;

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class UWpUnitProductionEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWpUnitProductionEntryWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void NativeTick(const FGeometry& MyGeometry, const float InDeltaTime) override;

	void SetUnitConfigID(const uint32 InUnitConfigID);

	static const FName CancelUnitInputKeyName;

private:
	UFUNCTION()
	void OnPressed();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Name = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Number = nullptr;

	uint32 UnitConfigID = 0;
};

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class UWpUnitProductionTableWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWpUnitProductionTableWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;

private:
	void OnAvailableOrdersChanged(const TSet<EWpOrderType>& AvailableOrders);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> Table = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UWpUnitProductionEntryWidget> EntryClass = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TArray<uint32> UnitConfigIDs = {};
};
