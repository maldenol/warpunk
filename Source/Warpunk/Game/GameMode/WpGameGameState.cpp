#include "WpGameGameState.h"

AWpGameGameState::AWpGameGameState()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
}

const UDataTable* AWpGameGameState::GetEntityConfigs() const
{
	return EntityConfigs;
}
