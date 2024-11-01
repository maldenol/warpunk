#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Order/WpOrder.h"

#include "Blueprint/UserWidget.h"

#include "WpOrderSelectionWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class UWpOrderSelectionEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWpOrderSelectionEntryWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void SetOrderType(const EWpOrderType InOrderType);

private:
	UFUNCTION()
	void OnPressed();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text = nullptr;

	EWpOrderType OrderType = EWpOrderType::Undefined;
};

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class UWpOrderSelectionListWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWpOrderSelectionListWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;

private:
	void OnAvailableOrdersChanged(const TSet<EWpOrderType>& AvailableOrders);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> List = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UWpOrderSelectionEntryWidget> EntryClass = nullptr;
};
