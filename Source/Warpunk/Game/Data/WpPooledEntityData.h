#pragma once

#include "CoreMinimal.h"

struct FWpEntityConfig;

struct FWpPooledEntityData final
{
	const FWpEntityConfig* EntityConfig = nullptr;
	uint32 EntityConfigID = 0;
	uint32 BelligerentID = 0;
};
