#pragma once

#include "CoreMinimal.h"

#include "Engine/ActorChannel.h"

#include "WpActorChannel.generated.h"

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class UWpActorChannel final : public UActorChannel
{
	GENERATED_BODY()

public:
	virtual bool ReadyForDormancy(const bool SuppressLogs) override;
};
