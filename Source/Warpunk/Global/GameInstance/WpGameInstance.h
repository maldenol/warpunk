#pragma once

#include "CoreMinimal.h"

#include "Engine/GameInstance.h"

#include "WpGameInstance.generated.h"

class UWpGameEntityEnduranceWidget;
class UWpLoadingScreen;

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class UWpGameInstance final : public UGameInstance
{
	GENERATED_BODY()

public:
	UWpLoadingScreen* GetLoadingScreen();

	void SetPlayerIds(const TArray<int32>& InPlayerIds);
	const TArray<int32>& GetPlayerIds() const;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UWpGameEntityEnduranceWidget> EnduranceWidgetClass = nullptr;
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> SelectionWidgetClass = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<const UDataTable> EntityConfigs = nullptr;

private:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UWpLoadingScreen> LoadingScreenClass = nullptr;
	UPROPERTY(VisibleInstanceOnly)
	TObjectPtr<UWpLoadingScreen> LoadingScreen = nullptr;

	TArray<int32> PlayerIds = {};
};
