#include "WpGamePlayerState.h"

#include "Warpunk/Game/MapLoad/WpMapLoadManager.h"

#include "Net/UnrealNetwork.h"

AWpGamePlayerState::AWpGamePlayerState()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
}

void AWpGamePlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWpGamePlayerState, Name);
	DOREPLIFETIME(AWpGamePlayerState, BelligerentId);
	DOREPLIFETIME(AWpGamePlayerState, TeamId);
	DOREPLIFETIME(AWpGamePlayerState, Color);
}

void AWpGamePlayerState::SetName(const FString& InName)
{
	Name = InName;
}

const FString& AWpGamePlayerState::GetName() const
{
	return Name;
}

void AWpGamePlayerState::SetBelligerentId(const uint32 InBelligerentId)
{
	ensureAlways(InBelligerentId > 0);
	BelligerentId = InBelligerentId;
}

uint32 AWpGamePlayerState::GetBelligerentId() const
{
	return BelligerentId;
}

void AWpGamePlayerState::SetTeamId(const uint32 InTeamId)
{
	ensureAlways(InTeamId > 0);
	TeamId = InTeamId;
	OnRep_TeamId();
}

uint32 AWpGamePlayerState::GetTeamId() const
{
	return TeamId;
}

void AWpGamePlayerState::SetColor(const FColor& InColor)
{
	Color = InColor;
}

const FColor& AWpGamePlayerState::GetColor() const
{
	return Color;
}

void AWpGamePlayerState::OnRep_TeamId()
{
	if (bHasNotifiedIsLoaded || !TeamId)
	{
		return;
	}

	AWpMapLoadManager::Instance(this)->NotifyActorLoaded();

	bHasNotifiedIsLoaded = true;
}
