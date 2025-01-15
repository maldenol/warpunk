#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Info.h"

#include "WpPooledActorVisibilityManager.generated.h"

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class AWpPooledActorVisibilityManager final : public AInfo
{
	GENERATED_BODY()

public:
	AWpPooledActorVisibilityManager();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool IsNetRelevantFor(const AActor* const RealViewer, const AActor* const ViewTarget, const FVector& SrcLocation) const override;

	void SetTeamId(const uint32 InTeamId);
	uint32 GetTeamId() const;

	void InitializeActorVisibility(AActor* const Actor, const bool bIsVisible);
	void UpdateActorVisibility(AActor* const Actor, const bool bGainedSight);
	void ClearActorVisibility(AActor* const Actor);
	bool IsVisibleForTeam(const AActor* const Actor) const;

private:
	void NetUpdateActorVisibility(AActor* const Actor, const bool bIsVisible);

	uint64 GetActorGuid(AActor* const Actor) const;
	AActor* GetActorByGuid(const uint64 Guid) const;

	UFUNCTION()
	void OnRep_HiddenActorGuids();
	UFUNCTION()
	void OnRep_VisibleActorGuids();

	uint32 TeamId = 0;

	TMap<AActor*, uint32> ObserverNumberPerActor = {};

	UPROPERTY(ReplicatedUsing=OnRep_HiddenActorGuids)
	TArray<uint64> HiddenActorGuids = {};
	UPROPERTY(ReplicatedUsing=OnRep_VisibleActorGuids)
	TArray<uint64> VisibleActorGuids = {};

	TSet<uint64> ShouldBeHiddenActorGuids = {};
	TSet<uint64> AlreadyHiddenActorGuids = {};
	TSet<uint64> ShouldBeVisibleActorGuids = {};
	TSet<uint64> AlreadyVisibleActorGuids = {};

public:
	static AWpPooledActorVisibilityManager* Instance(const uint32 TeamId);

	static void InitializeActorVisibilityAll(const uint32 TeamId, AActor* const Actor);
	static void UpdateActorVisibility(const uint32 TeamId, AActor* const Actor, const bool bGainedSight);
	static void ClearActorVisibilityAll(AActor* const Actor);
	static bool IsVisibleForTeam(const uint32 TeamId, const AActor* const Actor);

	static void SetAuthorityTeamId(const uint32 InAuthorityTeamId);
	static uint32 GetAuthorityTeamId();

private:
	static TMap<uint32, AWpPooledActorVisibilityManager*> Instances;

	static uint32 AuthorityTeamId;
};
