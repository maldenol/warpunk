#include "WpBelligerentComponent.h"

#include "Warpunk/Game/Entity/WpPooledEntityData.h"
#include "Warpunk/Game/Pool/WpPooledActorVisibilityManager.h"

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
	DOREPLIFETIME(UWpBelligerentComponent, TeamId);
}

void UWpBelligerentComponent::OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
{
	if (bIsAcquired)
	{
		const FWpPooledEntityData& PooledEntityData = static_cast<const FWpPooledEntityData&>(PooledObjectData);

		ensureAlways(PooledEntityData.BelligerentId > 0);
		ensureAlways(PooledEntityData.TeamId > 0);

		BelligerentId = PooledEntityData.BelligerentId;
		TeamId = PooledEntityData.TeamId;

		if (GetWorld()->GetNetMode() != NM_Client)
		{
			AWpPooledActorVisibilityManager::InitializeActorVisibilityAll(TeamId, GetOwner());
		}
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

bool UWpBelligerentComponent::BelongsToSameTeam(const uint32 OtherTeamId) const
{
	return TeamId == OtherTeamId;
}

bool UWpBelligerentComponent::BelongsToSameTeam(const UWpBelligerentComponent* const Other) const
{
	const uint32 OtherTeamId = Other->TeamId;
	return BelongsToSameTeam(OtherTeamId);
}

uint32 UWpBelligerentComponent::GetBelligerentId() const
{
	return BelligerentId;
}

uint32 UWpBelligerentComponent::GetTeamId() const
{
	return TeamId;
}
