#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"

#include "WpLobbyPlayerListWidget.generated.h"

class AWpLobbyPlayerState;

class UButton;
class UComboBoxString;
class UEditableTextBox;
class UTextBlock;
class UVerticalBox;

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class UWpLobbyPlayerListEntryWidget final : public UUserWidget
{
	GENERATED_BODY()

public:
	UWpLobbyPlayerListEntryWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void NativeConstruct() override;

	void SetPlayerState(AWpLobbyPlayerState* const InPlayerState, const uint32 Slot);
	const AWpLobbyPlayerState* GetPlayerState() const;

private:
	UFUNCTION()
	void OnBelligerentDown();
	UFUNCTION()
	void OnBelligerentUp();
	UFUNCTION()
	void OnTeamDown();
	UFUNCTION()
	void OnTeamUp();
	UFUNCTION()
	void OnColorSelected(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION()
	void OnKickPressed();

	void OnPlayerSettingsUpdated();
	void StepId(const bool bBelligerent, const int32 Delta);
	void EnsureColorOptions();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PlayerName = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> BelligerentId = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BelligerentDown = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BelligerentUp = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> TeamId = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> TeamDown = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> TeamUp = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UComboBoxString> ColorList = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Status = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> KickButton = nullptr;

	bool bApplyingColor = false;

	UPROPERTY()
	TObjectPtr<AWpLobbyPlayerState> PlayerState = nullptr;
};

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class UWpLobbyPlayerListWidget final : public UUserWidget
{
	GENERATED_BODY()

public:
	UWpLobbyPlayerListWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;

private:
	UFUNCTION()
	void OnPlayerLoggedIn(APlayerState* const PlayerState);
	UFUNCTION()
	void OnPlayerLoggedOut(const APlayerState* const PlayerState);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> List = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UWpLobbyPlayerListEntryWidget> EntryClass = nullptr;
};
