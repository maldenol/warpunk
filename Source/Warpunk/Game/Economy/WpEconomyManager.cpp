#include "WpEconomyManager.h"

AWpEconomyManager::AWpEconomyManager()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AWpEconomyManager::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);
}
