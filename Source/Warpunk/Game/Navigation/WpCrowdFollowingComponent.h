#pragma once

#include "CoreMinimal.h"

#include "Navigation/CrowdFollowingComponent.h"

#include "WpCrowdFollowingComponent.generated.h"

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class UWpCrowdFollowingComponent final : public UCrowdFollowingComponent
{
	GENERATED_BODY()

public:
	UWpCrowdFollowingComponent();

	virtual void Reset() override;
	virtual void Cleanup() override;

	virtual void GetCrowdAgentCollisions(float& CylinderRadius, float& CylinderHalfHeight) const override;
	virtual void ApplyCrowdAgentVelocity(const FVector& NewVelocity, const FVector& DestPathCorner, const bool bTraversingLink, const bool bIsNearEndOfPath) override;
};
