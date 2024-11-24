#include "WpBelligerentComponent.h"

#include "Warpunk/Game/Entity/WpPooledEntityData.h"
#include "Warpunk/Game/GameMode/WpGameGameState.h"

#include "Net/UnrealNetwork.h"

UWpBelligerentComponent::UWpBelligerentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
}

void UWpBelligerentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UWpBelligerentComponent, BelligerentId);
}

void UWpBelligerentComponent::OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
{
	if (bIsAcquired)
	{
		const FWpPooledEntityData& PooledEntityData = static_cast<const FWpPooledEntityData&>(PooledObjectData);

		ensureAlways(PooledEntityData.BelligerentId > 0);

		BelligerentId = PooledEntityData.BelligerentId;
	}
}

bool UWpBelligerentComponent::BelongsToSameBelligerent(const uint32 OtherBelligerentId) const
{
	return BelligerentId == OtherBelligerentId;
}

bool UWpBelligerentComponent::BelongsToSameBelligerent(const UWpBelligerentComponent* const Other) const
{
	const uint32 OtherBelligerentId = Other->BelligerentId;
	return BelongsToSameBelligerent(OtherBelligerentId);
}

bool UWpBelligerentComponent::BelongsToSameTeam(const uint32 OtherBelligerentId) const
{
	const AWpGameGameState* const GameState = GetWorld()->GetGameState<AWpGameGameState>();
	return GameState->BelongToSameTeam(BelligerentId, OtherBelligerentId);
}

bool UWpBelligerentComponent::BelongsToSameTeam(const UWpBelligerentComponent* const Other) const
{
	const uint32 OtherBelligerentId = Other->BelligerentId;
	return BelongsToSameTeam(OtherBelligerentId);
}

void UWpBelligerentComponent::SetBelligerentId(const uint32 OtherBelligerentId)
{
	BelligerentId = OtherBelligerentId;
}

void UWpBelligerentComponent::SetBelligerentId(const UWpBelligerentComponent* const Other)
{
	const uint32 OtherBelligerentId = Other->BelligerentId;
	SetBelligerentId(OtherBelligerentId);
}

uint32 UWpBelligerentComponent::GetBelligerentId() const
{
	return BelligerentId;
}
