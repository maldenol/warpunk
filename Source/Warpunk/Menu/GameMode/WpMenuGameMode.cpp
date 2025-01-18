#include "WpMenuGameMode.h"

#include "Warpunk/Menu/UI/WpMenuHUD.h"

AWpMenuGameMode::AWpMenuGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AWpMenuGameMode::BeginPlay()
{
	Super::BeginPlay();

	ensureAlways(HUDClass->IsChildOf<AWpMenuHUD>());
}
