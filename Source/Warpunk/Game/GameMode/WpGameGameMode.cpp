#include "WpGameGameMode.h"

#include "Warpunk/Game/Entity/WpBuilding.h"
#include "Warpunk/Game/Entity/WpUnit.h"
#include "Warpunk/Game/GameMode/WpGameGameState.h"
#include "Warpunk/Game/Player/WpGamePlayerController.h"
#include "Warpunk/Game/Player/WpStrategicViewPawn.h"
#include "Warpunk/Game/Pool/WpActorPool.h"
#include "Warpunk/Game/UI/WpGameHUD.h"
#include "Warpunk/Game/Weapon/WpProjectile.h"

AWpGameGameMode::AWpGameGameMode()
{
	PrimaryActorTick.bCanEverTick = false;

	GameStateClass = AWpGameGameState::StaticClass();
	PlayerControllerClass = AWpGamePlayerController::StaticClass();
	HUDClass = AWpGameHUD::StaticClass();
	DefaultPawnClass = AWpStrategicViewPawn::StaticClass();
}

void AWpGameGameMode::BeginPlay()
{
	Super::BeginPlay();

	AWpActorPool* const UnitPool = GetWorld()->SpawnActor<AWpActorPool>();
	UnitPool->Reserve(AWpUnit::StaticClass(), 50);

	AWpActorPool* const BuildingPool = GetWorld()->SpawnActor<AWpActorPool>();
	BuildingPool->Reserve(AWpBuilding::StaticClass(), 50);

	AWpActorPool* const ProjectilePool = GetWorld()->SpawnActor<AWpActorPool>();
	ProjectilePool->Reserve(AWpProjectile::StaticClass(), 10);
}
