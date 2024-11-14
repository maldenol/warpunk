#pragma once

#include "CoreMinimal.h"

struct FWpEntityConfig;

struct FWpPooledEntityData final
{
	const FWpEntityConfig* EntityConfig = nullptr;
	uint32 EntityConfigId = 0;
	uint32 BelligerentId = 0;
};
