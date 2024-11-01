#pragma once

#include "CoreMinimal.h"

#include "UObject/Interface.h"

#include "WpEntityInterface.generated.h"

class UWpEntityComponent;

UINTERFACE(MinimalAPI, NotBlueprintable, NotBlueprintType)
class UWpEntityInterface : public UInterface
{
	GENERATED_BODY()
};

class IWpEntityInterface
{
	GENERATED_BODY()

public:
	virtual UWpEntityComponent* GetEntityComponent() = 0;
	virtual const UWpEntityComponent* GetEntityComponent() const = 0;
};
