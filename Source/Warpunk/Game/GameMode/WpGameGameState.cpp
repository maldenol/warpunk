#include "WpGameGameState.h"

#include "Warpunk/Game/MapLoad/WpMapLoadManager.h"

AWpGameGameState::AWpGameGameState()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
}

void AWpGameGameState::BeginPlay()
{
	Super::BeginPlay();

	AWpMapLoadManager::Instance(this)->NotifyActorLoaded();
}
