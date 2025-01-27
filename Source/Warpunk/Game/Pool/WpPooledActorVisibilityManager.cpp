#include "WpPooledActorVisibilityManager.h"

#include "Warpunk/Game/Player/WpGamePlayerController.h"
#include "Warpunk/Game/Player/WpGamePlayerState.h"
#include "Warpunk/Game/Pool/WpPooledActorInterface.h"

#include "EngineUtils.h"
#include "Engine/PackageMapClient.h"
#include "Misc/NetworkGUID.h"
#include "Net/UnrealNetwork.h"

AWpPooledActorVisibilityManager::AWpPooledActorVisibilityManager()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.SetTickFunctionEnable(false);

	bReplicates = true;

	PooledActorVisibilityInfos.PooledActorVisibilityManager = this;
}

void AWpPooledActorVisibilityManager::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority())
	{
		SetActorTickEnabled(true);
		return;
	}

	if (TeamId == GetAuthorityTeamId())
	{
		SetActorTickEnabled(true);
	}

	for (TActorIterator<AActor> Iter{GetWorld()}; Iter; ++Iter)
	{
		AActor* const Actor = *Iter;

		if (!Actor->Implements<UWpPooledActorInterface>())
		{
			continue;
		}

		if (!Actor->GetIsReplicated())
		{
			continue;
		}

		NetUpdateActorVisibility(Actor, IsVisibleForTeam(Actor));
	}
}

void AWpPooledActorVisibilityManager::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (HasAuthority())
	{
		ensureAlways(TeamId == GetAuthorityTeamId());
	}

	for (TSet<uint64>::TIterator Iter = ShouldBeHiddenActorGuids.CreateIterator(); Iter; ++Iter)
	{
		const uint64 Guid = *Iter;
		if (AActor* const Actor = GetActorByGuid(Guid))
		{
			Cast<IWpPooledActorInterface>(Actor)->OnPooledActorVisibilityChanged(false);

			Iter.RemoveCurrent();

			bool bIsAlreadyInSet;
			AlreadyHiddenActorGuids.Add(Guid, &bIsAlreadyInSet);
			ensureAlways(!bIsAlreadyInSet);
		}
	}

	for (TSet<uint64>::TIterator Iter = ShouldBeVisibleActorGuids.CreateIterator(); Iter; ++Iter)
	{
		const uint64 Guid = *Iter;
		if (AActor* const Actor = GetActorByGuid(Guid))
		{
			Cast<IWpPooledActorInterface>(Actor)->OnPooledActorVisibilityChanged(true);

			Iter.RemoveCurrent();

			bool bIsAlreadyInSet;
			AlreadyVisibleActorGuids.Add(Guid, &bIsAlreadyInSet);
			ensureAlways(!bIsAlreadyInSet);
		}
	}
}

void AWpPooledActorVisibilityManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Instances.Remove(TeamId);

	Super::EndPlay(EndPlayReason);
}

void AWpPooledActorVisibilityManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWpPooledActorVisibilityManager, PooledActorVisibilityInfos);
}

bool AWpPooledActorVisibilityManager::IsNetRelevantFor(const AActor* const RealViewer, const AActor* const ViewTarget, const FVector& SrcLocation) const
{
	const AWpGamePlayerController* const ViewerPlayerController = Cast<AWpGamePlayerController>(RealViewer);
	if (!ViewerPlayerController)
	{
		return false;
	}

	const AWpGamePlayerState* const ViewerPlayerState = ViewerPlayerController->GetPlayerState<AWpGamePlayerState>();
	const uint32 ViewerTeamId = ViewerPlayerState->GetTeamId();

	return ViewerTeamId == TeamId;
}

void AWpPooledActorVisibilityManager::SetTeamId(const uint32 InTeamId)
{
	ensureAlways(HasAuthority());

	TeamId = InTeamId;

	ensureAlways(!Instances.Contains(TeamId));
	Instances.Add(TeamId, this);
}

uint32 AWpPooledActorVisibilityManager::GetTeamId() const
{
	ensureAlways(HasAuthority());

	return TeamId;
}

void AWpPooledActorVisibilityManager::InitializeActorVisibility(AActor* const Actor, const bool bIsVisible)
{
	ensureAlways(HasAuthority());

	if (!Actor->GetIsReplicated())
	{
		return;
	}

	if (!bIsVisible)
	{
		if (ObserverNumberPerActor.Contains(Actor))
		{
			return;
		}

		ObserverNumberPerActor.Emplace(Actor, 1);
	}

	UpdateActorVisibility(Actor, bIsVisible);
}

void AWpPooledActorVisibilityManager::UpdateActorVisibility(AActor* const Actor, const bool bGainedSight)
{
	ensureAlways(HasAuthority());

	if (!Actor->GetIsReplicated())
	{
		return;
	}

	bool bBecameVisible, bBecameHidden;

	if (uint32* const ObserverNumber = ObserverNumberPerActor.Find(Actor))
	{
		if (!bGainedSight)
		{
			ensureAlways(*ObserverNumber > 0);
		}

		*ObserverNumber += bGainedSight ? 1 : -1;

		bBecameVisible = bGainedSight && (*ObserverNumber == 1);
		bBecameHidden = (*ObserverNumber == 0);
	}
	else
	{
		ensureAlways(bGainedSight);

		ObserverNumberPerActor.Emplace(Actor, 1);

		bBecameVisible = true;
		bBecameHidden = false;
	}

	if (bBecameVisible || bBecameHidden)
	{
		NetUpdateActorVisibility(Actor, bBecameVisible);
	}
}

bool AWpPooledActorVisibilityManager::IsVisibleForTeam(const AActor* const Actor) const
{
	ensureAlways(HasAuthority());

	if (!Actor->GetIsReplicated())
	{
		return true;
	}

	if (const uint32* const ObserverNumber = ObserverNumberPerActor.Find(Actor))
	{
		return *ObserverNumber > 0;
	}

	return false;
}

void AWpPooledActorVisibilityManager::OnPooledActorVisibilityChanged(const uint64 Guid, const bool bIsVisible)
{
	if (HasAuthority() && TeamId != AuthorityTeamId)
	{
		return;
	}

	if (bIsVisible)
	{
		if (!AlreadyVisibleActorGuids.Contains(Guid))
		{
			ShouldBeVisibleActorGuids.Add(Guid);

			ShouldBeHiddenActorGuids.Remove(Guid);
			AlreadyHiddenActorGuids.Remove(Guid);
		}
	}
	else
	{
		if (!AlreadyHiddenActorGuids.Contains(Guid))
		{
			ShouldBeHiddenActorGuids.Add(Guid);

			ShouldBeVisibleActorGuids.Remove(Guid);
			AlreadyVisibleActorGuids.Remove(Guid);
		}
	}
}

void AWpPooledActorVisibilityManager::NetUpdateActorVisibility(AActor* const Actor, const bool bIsVisible)
{
	const uint64 Guid = GetActorGuid(Actor);
	PooledActorVisibilityInfos.SetVisibilityInfo(Guid, bIsVisible);

	if (bIsVisible)
	{
		Actor->FlushNetDormancy();
	}
}

uint64 AWpPooledActorVisibilityManager::GetActorGuid(AActor* const Actor) const
{
	if (GetNetMode() == NM_Standalone)
	{
		return Actor->GetUniqueID();
	}

	const FNetworkGUID NetworkGUID = GetWorld()->GetNetDriver()->GuidCache->GetOrAssignNetGUID(Actor);
	return NetworkGUID.ObjectId;
}

AActor* AWpPooledActorVisibilityManager::GetActorByGuid(const uint64 Guid) const
{
	if (GetNetMode() == NM_Standalone)
	{
		return Cast<AActor>(static_cast<UObject*>(GUObjectArray.IndexToObjectUnsafeForGC(Guid)->Object));
	}

	FNetworkGUID NetworkGUID;
	NetworkGUID.ObjectId = Guid;
	UObject* const Object = GetWorld()->GetNetDriver()->GuidCache->GetObjectFromNetGUID(NetworkGUID, true);
	return Cast<AActor>(Object);
}

AWpPooledActorVisibilityManager* AWpPooledActorVisibilityManager::Instance(const uint32 TeamId)
{
	if (AWpPooledActorVisibilityManager** const Ptr = Instances.Find(TeamId))
	{
		return *Ptr;
	}

	return nullptr;
}

void AWpPooledActorVisibilityManager::InitializeActorVisibility(const uint32 TeamId, AActor* const Actor)
{
	for (const TPair<uint32, AWpPooledActorVisibilityManager*>& TeamPooledActorVisibilityManager : Instances)
	{
		AWpPooledActorVisibilityManager* const PooledActorVisibilityManager = TeamPooledActorVisibilityManager.Value;
		const bool bIsVisible = (PooledActorVisibilityManager->GetTeamId() == TeamId);
		PooledActorVisibilityManager->InitializeActorVisibility(Actor, bIsVisible);
	}
}

void AWpPooledActorVisibilityManager::UpdateActorVisibility(const uint32 TeamId, AActor* const Actor, const bool bGainedSight)
{
	Instance(TeamId)->UpdateActorVisibility(Actor, bGainedSight);
}

bool AWpPooledActorVisibilityManager::IsVisibleForTeam(const uint32 TeamId, const AActor* const Actor)
{
	if (const AWpPooledActorVisibilityManager* const PooledActorVisibilityManager = Instance(TeamId))
	{
		return PooledActorVisibilityManager->IsVisibleForTeam(Actor);
	}

	return false;
}

void AWpPooledActorVisibilityManager::SetAuthorityTeamId(const uint32 InAuthorityTeamId)
{
	AuthorityTeamId = InAuthorityTeamId;
}

uint32 AWpPooledActorVisibilityManager::GetAuthorityTeamId()
{
	return AuthorityTeamId;
}

TMap<uint32, AWpPooledActorVisibilityManager*> AWpPooledActorVisibilityManager::Instances = {};

uint32 AWpPooledActorVisibilityManager::AuthorityTeamId = 0;
