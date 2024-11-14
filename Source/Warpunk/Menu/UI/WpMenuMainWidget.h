#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Menu/UI/WpMenuHUD.h"

#include "Blueprint/UserWidget.h"

#include "WpMenuMainWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class UWpMenuMainEntryWidget final : public UUserWidget
{
	GENERATED_BODY()

public:
	UWpMenuMainEntryWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	void SetMenuState(const EWpMenuState InMenuState);

private:
	UFUNCTION()
	void OnPressed();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text = nullptr;

	EWpMenuState MenuState = EWpMenuState::Main;
};

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class UWpMenuMainWidget final : public UUserWidget
{
	GENERATED_BODY()

public:
	UWpMenuMainWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;

private:
	void AddEntry(const EWpMenuState MenuState);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> List = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UWpMenuMainEntryWidget> EntryClass = nullptr;
};
