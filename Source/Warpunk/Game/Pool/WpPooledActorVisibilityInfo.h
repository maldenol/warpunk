#pragma once

#include "CoreMinimal.h"

#include "Net/Serialization/FastArraySerializer.h"

#include "WpPooledActorVisibilityInfo.generated.h"

class AWpPooledActorVisibilityManager;
struct FWpPooledActorVisibilityInfoArray;

USTRUCT(NotBlueprintable, NotBlueprintType)
struct FWpPooledActorVisibilityInfo final : public FFastArraySerializerItem
{
	GENERATED_BODY()

	void PostReplicatedAdd(const FWpPooledActorVisibilityInfoArray& InArraySerializer);
	void PostReplicatedChange(const FWpPooledActorVisibilityInfoArray& InArraySerializer);

	UPROPERTY(VisibleInstanceOnly)
	uint64 Guid = 0;
	UPROPERTY(VisibleInstanceOnly)
	bool bIsVisible = false;
};

USTRUCT(NotBlueprintable, NotBlueprintType)
struct FWpPooledActorVisibilityInfoArray final : public FFastArraySerializer
{
	GENERATED_BODY()

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams);

	void SetVisibilityInfo(const uint64 Guid, const bool bIsVisible);
	void OnItemChanged(const FWpPooledActorVisibilityInfo& PooledActorVisibilityInfo) const;

	UPROPERTY(VisibleInstanceOnly)
	TArray<FWpPooledActorVisibilityInfo> Items = {};

	UPROPERTY(VisibleInstanceOnly)
	AWpPooledActorVisibilityManager* PooledActorVisibilityManager = nullptr;
};

template <>
struct TStructOpsTypeTraits<FWpPooledActorVisibilityInfoArray> final : TStructOpsTypeTraitsBase2<FWpPooledActorVisibilityInfoArray>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};
