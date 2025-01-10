#include "WpGameGameMode.h"

#include "Warpunk/Game/Endurance/WpEnduranceComponent.h"
#include "Warpunk/Game/Entity/WpBuilding.h"
#include "Warpunk/Game/Entity/WpCommander.h"
#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Entity/WpPooledEntityData.h"
#include "Warpunk/Game/Entity/WpUnit.h"
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

	AWpGameGameState* const InnerGameState = GetWorld()->GetGameState<AWpGameGameState>();

	const uint32 ConfigId = InnerGameState->CommanderConfigId;
	ensureAlways(ConfigId > 0);
	const UDataTable* const EntityConfigs = InnerGameState->GetEntityConfigs();
	const FName RowName = FName{FString::FromInt(ConfigId)};
	const FWpEntityConfig& EntityConfig = *EntityConfigs->FindRow<FWpEntityConfig>(RowName, nullptr);

	FActorSpawnParameters ActorSpawnParameters;
	ActorSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AWpCommander* const Commander = GetWorld()->SpawnActor<AWpCommander>(EntityConfig.EntityClass, StrategicViewPawn->GetActorTransform(), ActorSpawnParameters);
	ensureAlways(Commander);

	const AWpGamePlayerState* const PlayerState = PlayerController->GetPlayerState<AWpGamePlayerState>();

	FWpPooledEntityData PooledEntityData;
	PooledEntityData.EntityConfig = &EntityConfig;
	PooledEntityData.EntityConfigId = ConfigId;
	PooledEntityData.BelligerentId = PlayerState->GetBelligerentId();
	PooledEntityData.TeamId = PlayerState->GetTeamId();

	Commander->AcquirePooledObject(PooledEntityData);

	UWpEnduranceComponent* const CommanderEnduranceComponent = Commander->GetEnduranceComponent();
	CommanderEnduranceComponent->ReceiveRepair(CommanderEnduranceComponent->GetMatterCost());

	PlayerController->SetCommander(Commander);
}
