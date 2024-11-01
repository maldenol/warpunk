#include "WpGameGameState.h"

AWpGameGameState::AWpGameGameState()
{
	PrimaryActorTick.bCanEverTick = false;
}

const UDataTable* AWpGameGameState::GetEntityConfigs() const
{
	return EntityConfigs;
}

uint32 AWpGameGameState::GetPlayerBelligerentID(const APlayerController* const PlayerController) const
{
	return PlayerBelligerentIDs[PlayerController];
}
