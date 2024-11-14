#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"

#include "WpLobbyOptionsWidget.generated.h"

class UCheckBox;
class UEditableTextBox;

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class UWpLobbyOptionsWidget final : public UUserWidget
{
	GENERATED_BODY()

public:
	UWpLobbyOptionsWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void NativeConstruct() override;

private:
	UFUNCTION()
	void OnAllowTeamsChanged(const bool bIsChecked);
	UFUNCTION()
	void OnMaxPlayerNumberCommitted(const FText& Text, const ETextCommit::Type CommitMethod);
	UFUNCTION()
	void OnMaxSpectatorNumberCommitted(const FText& Text, const ETextCommit::Type CommitMethod);

	UFUNCTION()
	void OnLobbySettingsUpdated();

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> AllowTeams = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> MaxPlayerNumber = nullptr;
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> MaxSpectatorNumber = nullptr;
};
