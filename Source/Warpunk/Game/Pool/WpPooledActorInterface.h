#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"

#include "UObject/Interface.h"

#include "WpPooledActorInterface.generated.h"

UINTERFACE(MinimalAPI, NotBlueprintable, NotBlueprintType)
class UWpPooledActorInterface : public UInterface
{
	GENERATED_BODY()
};

class IWpPooledActorInterface : public IWpPooledObjectInterface
{
	GENERATED_BODY()

public:
	virtual void OnPooledActorVisibilityChanged(const bool bIsVisible) = 0;
};
