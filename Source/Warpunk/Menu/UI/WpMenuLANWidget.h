#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"

#include "WpMenuLANWidget.generated.h"

class UButton;
class UEditableTextBox;

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class UWpMenuLANWidget final : public UUserWidget
{
	GENERATED_BODY()

public:
	UWpMenuLANWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void NativeConstruct() override;

private:
	UFUNCTION()
	void OnBackPressed();
	UFUNCTION()
	void OnConnectPressed();
	UFUNCTION()
	void OnHostPressed();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> UrlText = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BackButton = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ConnectButton = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> HostButton = nullptr;
};
