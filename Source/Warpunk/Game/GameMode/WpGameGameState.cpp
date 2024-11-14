#include "WpGameGameState.h"

AWpGameGameState::AWpGameGameState()
{
	PrimaryActorTick.bCanEverTick = false;
}

const UDataTable* AWpGameGameState::GetEntityConfigs() const
{
	return EntityConfigs;
}
