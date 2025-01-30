#pragma once

#include "CoreMinimal.h"

#include "WpEntityType.generated.h"

UENUM(NotBlueprintable, NotBlueprintType)
enum class EWpEntityType : uint8
{
	Undefined UMETA(Hidden),
	Building,
	Unit,
};
