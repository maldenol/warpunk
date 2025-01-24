#pragma once

#include "CoreMinimal.h"

#include "WpProjectileConfig.generated.h"

USTRUCT(NotBlueprintable, NotBlueprintType)
struct FWpProjectileConfig final
{
	GENERATED_BODY()

	UPROPERTY()
	uint32 EntityConfigId = 0;
	UPROPERTY()
	uint32 WeaponConfigIndex = 0;
};
