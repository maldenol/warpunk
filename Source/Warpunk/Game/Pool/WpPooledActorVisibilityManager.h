#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Pool/WpPooledActorVisibilityInfo.h"

#include "GameFramework/Info.h"

#include "WpPooledActorVisibilityManager.generated.h"

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class AWpPooledActorVisibilityManager final : public AInfo
{
	GENERATED_BODY()

public:
	AWpPooledActorVisibilityManager();

	virtual void BeginPlay() override;
	virtual void Tick(const float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool IsNetRelevantFor(const AActor* const RealViewer, const AActor* const ViewTarget, const FVector& SrcLocation) const override;

	void SetTeamId(const uint32 InTeamId);
	uint32 GetTeamId() const;

	void InitializeActorVisibility(AActor* const Actor, const bool bIsVisible);
	void UpdateActorVisibility(AActor* const Actor, const bool bGainedSight);
	bool IsVisibleForTeam(const AActor* const Actor) const;

private:
	void NetUpdateActorVisibility(AActor* const Actor, const bool bIsVisible);

	void SetPooledActorVisibility(const uint64 Guid, const bool bIsVisible);

	uint64 GetActorGuid(AActor* const Actor) const;
	AActor* GetActorByGuid(const uint64 Guid) const;

	uint32 TeamId = 0;

	TMap<AActor*, uint32> ObserverNumberPerActor = {};

	UPROPERTY(Replicated)
	FWpPooledActorVisibilityInfoArray PooledActorVisibilityInfos = {};

	TSet<uint64> ShouldBeHiddenActorGuids = {};
	TSet<uint64> AlreadyHiddenActorGuids = {};
	TSet<uint64> ShouldBeVisibleActorGuids = {};
	TSet<uint64> AlreadyVisibleActorGuids = {};

	friend struct FWpPooledActorVisibilityInfoArray;

public:
	static AWpPooledActorVisibilityManager* Instance(const UWorld* const World, const uint32 TeamId);

	static void InitializeActorVisibility(const uint32 TeamId, AActor* const Actor);
	static void UpdateActorVisibility(const uint32 TeamId, AActor* const Actor, const bool bGainedSight);
	static bool IsVisibleForTeam(const uint32 TeamId, const AActor* const Actor);

	static void SetAuthorityTeamId(const UWorld* const World, const uint32 AuthorityTeamId);
	static uint32 GetAuthorityTeamId(const UWorld* const World);

private:
	using FWpInstanceKey = TPair<const UWorld*, uint32>;
	using FWpAuthorityTeamIdKey = const UWorld*;

	static TMap<FWpInstanceKey, AWpPooledActorVisibilityManager*> Instances;
	static TMap<FWpAuthorityTeamIdKey, uint32> AuthorityTeamIds;
};
