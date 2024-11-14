#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"

#include "WpLobbyHUDWidget.generated.h"

class UWpLobbyOptionsWidget;
class UWpLobbyPlayerListWidget;

class UButton;
class UTextBlock;

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class UWpLobbyHUDWidget final : public UUserWidget
{
	GENERATED_BODY()

public:
	UWpLobbyHUDWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void NativeConstruct() override;

private:
	UFUNCTION()
	void OnReadyPressed();
	UFUNCTION()
	void OnBackPressed();

	UFUNCTION()
	void OnPlayerStateReplicated();
	UFUNCTION()
	void OnPlayerSettingsUpdated();
	UFUNCTION()
	void OnTimeUntilGameStartUpdated(const float TimeUntilGameStart);

	void SetElementsVisibility(const bool bIsReady);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWpLobbyOptionsWidget> LobbyOptionsWidget = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWpLobbyPlayerListWidget> LobbyPlayerListWidget = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ReadyButton = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ReadyButtonText = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BackButton = nullptr;
};
