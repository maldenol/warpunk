#pragma once

#include "CoreMinimal.h"

#include "IpConnection.h"

#include "WpIpConnection.generated.h"

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class UWpIpConnection final : public UIpConnection
{
	GENERATED_BODY()

public:
	virtual void FlushDormancy(AActor* const Actor) override;
};
