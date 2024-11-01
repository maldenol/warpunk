#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Info.h"

#include "WpEconomyManager.generated.h"

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class AWpEconomyManager final : public AInfo
{
	GENERATED_BODY()

public:
	AWpEconomyManager();

	virtual void Tick(const float DeltaTime) override;
};
