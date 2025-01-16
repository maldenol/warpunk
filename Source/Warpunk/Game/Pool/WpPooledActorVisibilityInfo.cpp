#include "WpPooledActorVisibilityInfo.h"

#include "Warpunk/Game/Pool/WpPooledActorVisibilityManager.h"

void FWpPooledActorVisibilityInfo::PostReplicatedAdd(const FWpPooledActorVisibilityInfoArray& InArraySerializer)
{
	InArraySerializer.OnItemChanged(*this);
}

void FWpPooledActorVisibilityInfo::PostReplicatedChange(const FWpPooledActorVisibilityInfoArray& InArraySerializer)
{
	InArraySerializer.OnItemChanged(*this);
}

bool FWpPooledActorVisibilityInfoArray::NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
{
	return FastArrayDeltaSerialize<FWpPooledActorVisibilityInfo, FWpPooledActorVisibilityInfoArray>(Items, DeltaParams, *this);
}

void FWpPooledActorVisibilityInfoArray::SetVisibilityInfo(const uint64 Guid, const bool bIsVisible)
{
	if (FWpPooledActorVisibilityInfo* const ExistingPooledActorVisibilityInfo =
		Items.FindByPredicate([Guid](const FWpPooledActorVisibilityInfo& PooledActorVisibilityInfo) -> bool
		{
			return PooledActorVisibilityInfo.Guid == Guid;
		}))
	{
		if (ExistingPooledActorVisibilityInfo->bIsVisible != bIsVisible)
		{
			ExistingPooledActorVisibilityInfo->bIsVisible = bIsVisible;
			MarkItemDirty(*ExistingPooledActorVisibilityInfo);
			OnItemChanged(*ExistingPooledActorVisibilityInfo);
		}
	}
	else
	{
		FWpPooledActorVisibilityInfo NewPooledActorVisibilityInfo;
		NewPooledActorVisibilityInfo.Guid = Guid;
		NewPooledActorVisibilityInfo.bIsVisible = bIsVisible;
		const uint32 Index = Items.Add(NewPooledActorVisibilityInfo);
		MarkItemDirty(Items[Index]);
		OnItemChanged(Items[Index]);
	}
}

void FWpPooledActorVisibilityInfoArray::OnItemChanged(const FWpPooledActorVisibilityInfo& PooledActorVisibilityInfo) const
{
	const uint64 Guid = PooledActorVisibilityInfo.Guid;
	const bool bIsVisible = PooledActorVisibilityInfo.bIsVisible;
	PooledActorVisibilityManager->OnPooledActorVisibilityChanged(Guid, bIsVisible);
}
