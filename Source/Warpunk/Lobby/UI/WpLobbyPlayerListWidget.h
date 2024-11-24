#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"

#include "WpLobbyPlayerListWidget.generated.h"

class AWpLobbyPlayerState;

class UButton;
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

	void SetPlayerState(AWpLobbyPlayerState* const InPlayerState);
	const AWpLobbyPlayerState* GetPlayerState() const;

private:
	UFUNCTION()
	void OnBelligerentIdCommitted(const FText& Text, const ETextCommit::Type CommitMethod);
	UFUNCTION()
	void OnTeamIdCommitted(const FText& Text, const ETextCommit::Type CommitMethod);
	UFUNCTION()
	void OnColorTextCommitted(const FText& Text, const ETextCommit::Type CommitMethod);
	UFUNCTION()
	void OnKickPressed();

	void OnPlayerSettingsUpdated();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PlayerName = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> BelligerentId = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> TeamId = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> ColorText = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Status = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> KickButton = nullptr;

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
