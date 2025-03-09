#pragma once

#include "CoreMinimal.h"

#include "WpEntityMovementType.generated.h"

UENUM(NotBlueprintable, NotBlueprintType)
enum class EWpEntityMovementType : uint8
{
	Undefined UMETA(Hidden),
	Legs,
	Tracks,
	Wheels,
};
