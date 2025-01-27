#pragma once

#include "CoreMinimal.h"

#include "Engine/GameInstance.h"

#include "WpGameInstance.generated.h"

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class UWpGameInstance final : public UGameInstance
{
	GENERATED_BODY()

public:
	void SetPlayerIds(const TArray<int32>& InPlayerIds);
	const TArray<int32>& GetPlayerIds() const;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> EnduranceWidgetClass = nullptr;
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> SelectionWidgetClass = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<const UDataTable> EntityConfigs = nullptr;

private:
	TArray<int32> PlayerIds = {};
};
