#pragma once

#include "CoreMinimal.h"

#include "Engine/GameInstance.h"

#include "WpGameInstance.generated.h"

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class UWpGameInstance final : public UGameInstance
{
	GENERATED_BODY()

public:
	void SetPlayerIds(const TArray<int32>& InPlayerIds);
	const TArray<int32>& GetPlayerIds() const;

private:
	TArray<int32> PlayerIds = {};
};
