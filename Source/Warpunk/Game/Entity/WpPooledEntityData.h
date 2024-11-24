#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Pool/WpPooledObjectData.h"

struct FWpEntityConfig;

struct FWpPooledEntityData final : FWpPooledObjectData
{
	const FWpEntityConfig* EntityConfig = nullptr;
	uint32 EntityConfigId = 0;
	uint32 BelligerentId = 0;
};
