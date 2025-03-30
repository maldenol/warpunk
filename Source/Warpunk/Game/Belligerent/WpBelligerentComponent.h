#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"

#include "Components/ActorComponent.h"

#include "WpBelligerentComponent.generated.h"

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class UWpBelligerentComponent final : public UActorComponent, public IWpPooledObjectInterface
{
	GENERATED_BODY()

public:
	UWpBelligerentComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData) override;
	virtual void OnPooledObjectVisibilityChanged(const bool bIsVisible) override;

	bool BelongsToSameBelligerent(const uint32 OtherBelligerentId) const;
	bool BelongsToSameBelligerent(const UWpBelligerentComponent* const Other) const;
	bool BelongsToSameTeam(const uint32 OtherTeamId) const;
	bool BelongsToSameTeam(const UWpBelligerentComponent* const Other) const;

	uint32 GetBelligerentId() const;
	uint32 GetTeamId() const;

private:
	UPROPERTY(VisibleInstanceOnly, Replicated)
	uint32 BelligerentId = 0;
	UPROPERTY(VisibleInstanceOnly, Replicated)
	uint32 TeamId = 0;
};
