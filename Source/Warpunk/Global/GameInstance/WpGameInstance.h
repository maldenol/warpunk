#pragma once

#include "CoreMinimal.h"

#include "Engine/GameInstance.h"

#include "WpGameInstance.generated.h"

struct FWpEntityConfig;
class UWpGameEntityEnduranceWidget;
class UWpLoadingScreen;

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class UWpGameInstance final : public UGameInstance
{
	GENERATED_BODY()

public:
	UWpLoadingScreen* GetLoadingScreen();

	FWpEntityConfig GetEntityConfig(const uint32 EntityConfigId) const;
	const TSubclassOf<UWpGameEntityEnduranceWidget>& GetEnduranceWidgetClass() const;
	const TSubclassOf<UUserWidget>& GetSelectionWidgetClass() const;

	void SetPlayerIds(const TArray<int32>& InPlayerIds);
	const TArray<int32>& GetPlayerIds() const;

private:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UWpLoadingScreen> LoadingScreenClass = nullptr;
	UPROPERTY(VisibleInstanceOnly)
	TObjectPtr<UWpLoadingScreen> LoadingScreen = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<const UDataTable> EntityConfigs = nullptr;
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UWpGameEntityEnduranceWidget> EnduranceWidgetClass = nullptr;
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> SelectionWidgetClass = nullptr;

	TArray<int32> PlayerIds = {};

public:
	static const FName LoadingScreenName;
};
