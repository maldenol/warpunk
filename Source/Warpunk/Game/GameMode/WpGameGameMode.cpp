#include "WpGameGameMode.h"

#include "Warpunk/Game/Data/WpEntityConfig.h"
#include "Warpunk/Game/Data/WpPooledEntityData.h"
#include "Warpunk/Game/Entity/WpBuilding.h"
#include "Warpunk/Game/Entity/WpCommander.h"
#include "Warpunk/Game/Entity/WpUnit.h"
#include "Warpunk/Game/EntityComponent/WpEntityComponent.h"
#include "Warpunk/Game/GameMode/WpGameGameState.h"
#include "Warpunk/Game/Player/WpGamePlayerController.h"
#include "Warpunk/Game/Player/WpGamePlayerState.h"
#include "Warpunk/Game/Player/WpStrategicViewPawn.h"
#include "Warpunk/Game/Pool/WpActorPool.h"
#include "Warpunk/Game/UI/WpGameHUD.h"
#include "Warpunk/Game/Weapon/WpProjectile.h"

AWpGameGameMode::AWpGameGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AWpGameGameMode::BeginPlay()
{
	Super::BeginPlay();

	ensureAlways(GameStateClass->IsChildOf(AWpGameGameState::StaticClass()));
	ensureAlways(PlayerControllerClass->IsChildOf(AWpGamePlayerController::StaticClass()));
	ensureAlways(PlayerStateClass->IsChildOf(AWpGamePlayerState::StaticClass()));
	ensureAlways(HUDClass->IsChildOf(AWpGameHUD::StaticClass()));
	ensureAlways(DefaultPawnClass->IsChildOf(AWpStrategicViewPawn::StaticClass()));

	AWpActorPool* const UnitPool = GetWorld()->SpawnActor<AWpActorPool>();
	UnitPool->Reserve(AWpUnit::StaticClass(), 50);

	AWpActorPool* const BuildingPool = GetWorld()->SpawnActor<AWpActorPool>();
	BuildingPool->Reserve(AWpBuilding::StaticClass(), 50);

	AWpActorPool* const ProjectilePool = GetWorld()->SpawnActor<AWpActorPool>();
	ProjectilePool->Reserve(AWpProjectile::StaticClass(), 10);
}

void AWpGameGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	AWpGamePlayerController* const PlayerController = Cast<AWpGamePlayerController>(NewPlayer);

	AWpStrategicViewPawn* const StrategicViewPawn = NewPlayer->GetPawn<AWpStrategicViewPawn>();
	ensureAlways(StrategicViewPawn);

	PlayerController->SetStrategicViewPawn(StrategicViewPawn);

	const uint32 ConfigId = GetWorld()->GetGameState<AWpGameGameState>()->CommanderConfigId;
	ensureAlways(ConfigId > 0);
	const UDataTable* const EntityConfigs = GetWorld()->GetGameState<AWpGameGameState>()->GetEntityConfigs();
	const FName RowName = FName{FString::FromInt(ConfigId)};
	const FWpEntityConfig& EntityConfig = *EntityConfigs->FindRow<FWpEntityConfig>(RowName, nullptr);

	FActorSpawnParameters ActorSpawnParameters;
	ActorSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AWpCommander* const Commander = GetWorld()->SpawnActor<AWpCommander>(EntityConfig.EntityClass, StrategicViewPawn->GetActorTransform(), ActorSpawnParameters);
	ensureAlways(Commander);

	const AWpGamePlayerState* const PlayerState = PlayerController->GetPlayerState<AWpGamePlayerState>();
	const uint32 BelligerentId = PlayerState->GetBelligerentId();

	FWpPooledEntityData PooledEntityData;
	PooledEntityData.EntityConfig = &EntityConfig;
	PooledEntityData.EntityConfigId = ConfigId;
	PooledEntityData.BelligerentId = BelligerentId;

	Commander->InitializePooledObject();
	Commander->AcquirePooledObject(&PooledEntityData);

	FWpEndurance& CommanderEndurance = WP_AS_ENDURANCE(Commander->GetEntityComponent());
	CommanderEndurance.ReceiveRepair(CommanderEndurance.GetMatterCost());

	PlayerController->SetCommander(Commander);
}
