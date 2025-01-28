#include "WpGameGameState.h"

#include "Warpunk/Global/GameInstance/WpGameInstance.h"
#include "Warpunk/Global/LoadingScreen/WpLoadingScreen.h"

AWpGameGameState::AWpGameGameState()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
}

void AWpGameGameState::BeginPlay()
{
	Super::BeginPlay();

	GetGameInstance<UWpGameInstance>()->GetLoadingScreen()->NotifyObjectLoaded();
}
