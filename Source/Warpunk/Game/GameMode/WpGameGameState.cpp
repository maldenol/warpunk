#include "WpGameGameState.h"

#include "Net/UnrealNetwork.h"

bool FWpBelligerentTeam::operator==(const FWpBelligerentTeam& Other) const
{
	return BelligerentId == Other.BelligerentId;
}

bool FWpBelligerentTeam::operator==(const uint32 OtherBelligerentId) const
{
	return BelligerentId == OtherBelligerentId;
}

AWpGameGameState::AWpGameGameState()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
}

void AWpGameGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWpGameGameState, BelligerentTeams);
}

void AWpGameGameState::AddToTeam(const uint32 BelligerentId, const uint32 TeamId)
{
	ensureAlways(BelligerentId > 0);
	ensureAlways(TeamId > 0);

	FWpBelligerentTeam BelligerentTeam;
	BelligerentTeam.BelligerentId = BelligerentId;
	BelligerentTeam.TeamId = TeamId;
	BelligerentTeams.AddUnique(BelligerentTeam);

	OnRep_BelligerentTeams();
}

bool AWpGameGameState::BelongToSameTeam(const uint32 BelligerentId1, const uint32 BelligerentId2) const
{
	if (BelligerentId1 == BelligerentId2)
	{
		return true;
	}

	const FWpBelligerentTeam* const BelligerentTeam1 = BelligerentTeams.FindByKey(BelligerentId1);
	if (!BelligerentTeam1)
	{
		return false;
	}
	const uint32 TeamId1 = BelligerentTeam1->TeamId;

	const FWpBelligerentTeam* const BelligerentTeam2 = BelligerentTeams.FindByKey(BelligerentId2);
	if (!BelligerentTeam2)
	{
		return false;
	}
	const uint32 TeamId2 = BelligerentTeam2->TeamId;

	return TeamId1 == TeamId2;
}

AWpGameGameState::FBelligerentTeamsReplicatedEvent& AWpGameGameState::OnBelligerentTeamsReplicated()
{
	return BelligerentTeamsReplicatedEvent;
}

const UDataTable* AWpGameGameState::GetEntityConfigs() const
{
	return EntityConfigs;
}

void AWpGameGameState::OnRep_BelligerentTeams()
{
	BelligerentTeamsReplicatedEvent.Broadcast();
}
