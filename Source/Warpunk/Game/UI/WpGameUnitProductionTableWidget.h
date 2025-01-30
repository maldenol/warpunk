#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"

#include "WpGameUnitProductionTableWidget.generated.h"

enum class EWpOrderType : uint8;

class UButton;
class UHorizontalBox;
class UTextBlock;

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class UWpGameUnitProductionTableEntryWidget final : public UUserWidget
{
	GENERATED_BODY()

public:
	UWpGameUnitProductionTableEntryWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void NativeTick(const FGeometry& MyGeometry, const float InDeltaTime) override;

	void SetUnitConfigId(const uint32 InUnitConfigId);

private:
	UFUNCTION()
	void OnPressed();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Name = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Number = nullptr;

	uint32 UnitConfigId = 0;

public:
	static const FName CancelUnitInputKeyName;
};

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class UWpGameUnitProductionTableWidget final : public UUserWidget
{
	GENERATED_BODY()

public:
	UWpGameUnitProductionTableWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;

private:
	void OnAvailableOrdersChanged(const TSet<EWpOrderType>& AvailableOrders);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UHorizontalBox> Table = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UWpGameUnitProductionTableEntryWidget> EntryClass = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TArray<uint32> UnitConfigIds = {};
};
