#include "WpGameGameMode.h"

#include "Warpunk/Game/Endurance/WpEnduranceComponent.h"
#include "Warpunk/Game/Entity/WpBuilding.h"
#include "Warpunk/Game/Entity/WpCommander.h"
#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Entity/WpPooledEntityData.h"
#include "Warpunk/Game/Entity/WpUnit.h"
#include "Warpunk/Game/GameInstance/WpGameInstance.h"
#include "Warpunk/Game/GameMode/WpGameGameState.h"
#include "Warpunk/Game/Player/WpGamePlayerController.h"
#include "Warpunk/Game/Player/WpGamePlayerState.h"
#include "Warpunk/Game/Player/WpStrategicViewPawn.h"
#include "Warpunk/Game/Pool/WpActorPool.h"
#include "Warpunk/Game/Pool/WpPooledActorVisibilityManager.h"
#include "Warpunk/Game/UI/WpGameHUD.h"
#include "Warpunk/Game/Weapon/WpProjectile.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"

static constexpr uint32 UnitPoolSize = 50;
static constexpr uint32 BuildingPoolSize = 50;
static constexpr uint32 ProjectilePoolSize = 10;

AWpGameGameMode::AWpGameGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AWpGameGameMode::BeginPlay()
{
	Super::BeginPlay();

	ensureAlways(GameStateClass->IsChildOf<AWpGameGameState>());
	ensureAlways(PlayerControllerClass->IsChildOf<AWpGamePlayerController>());
	ensureAlways(PlayerStateClass->IsChildOf<AWpGamePlayerState>());
	ensureAlways(HUDClass->IsChildOf<AWpGameHUD>());
	ensureAlways(DefaultPawnClass->IsChildOf<AWpStrategicViewPawn>());

	AWpActorPool* const UnitPool = GetWorld()->SpawnActor<AWpActorPool>();
	UnitPool->Reserve(AWpUnit::StaticClass(), UnitPoolSize);

	AWpActorPool* const BuildingPool = GetWorld()->SpawnActor<AWpActorPool>();
	BuildingPool->Reserve(AWpBuilding::StaticClass(), BuildingPoolSize);

	AWpActorPool* const ProjectilePool = GetWorld()->SpawnActor<AWpActorPool>();
	ProjectilePool->Reserve(AWpProjectile::StaticClass(), ProjectilePoolSize);
}

void AWpGameGameMode::HandleStartingNewPlayer_Implementation(APlayerController* const NewPlayer)
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

	const uint32 TeamId = PlayerState->GetTeamId();
	if (!AWpPooledActorVisibilityManager::Instance(TeamId))
	{
		AWpPooledActorVisibilityManager* const PooledActorVisibilityManager = GetWorld()->SpawnActor<AWpPooledActorVisibilityManager>();
		PooledActorVisibilityManager->SetTeamId(TeamId);

		if (AWpPooledActorVisibilityManager::GetAuthorityTeamId() == 0)
		{
			AWpPooledActorVisibilityManager::SetAuthorityTeamId(TeamId);
		}
	}

	FWpPooledEntityData PooledEntityData;
	PooledEntityData.EntityConfig = &EntityConfig;
	PooledEntityData.EntityConfigId = ConfigId;
	PooledEntityData.BelligerentId = PlayerState->GetBelligerentId();
	PooledEntityData.TeamId = PlayerState->GetTeamId();

	Commander->AcquirePooledObject(PooledEntityData);

	UWpEnduranceComponent* const CommanderEnduranceComponent = Commander->GetEnduranceComponent();
	CommanderEnduranceComponent->ReceiveRepair(CommanderEnduranceComponent->GetMatterCost());

	PlayerController->SetCommander(Commander);

	const uint32 PlayerNum = GetGameInstance<UWpGameInstance>()->GetPlayerIds().Num();
	const uint32 ExpectedActorNum =
		1 // Game state
		+ 2 * PlayerNum // Player states and commanders
		+ (UnitPoolSize + BuildingPoolSize + ProjectilePoolSize); // Pooled actors
	PlayerController->ClientSetExpectedActorNum(ExpectedActorNum);
}

AActor* AWpGameGameMode::ChoosePlayerStart_Implementation(AController* const Player)
{
	for (TActorIterator<APlayerStart> Iter{GetWorld()}; Iter; ++Iter)
	{
		APlayerStart* const PlayerStart = *Iter;

		const AWpGamePlayerState* const PlayerState = Player->GetPlayerState<AWpGamePlayerState>();
		const uint32 BelligerentId = PlayerState->GetBelligerentId();

		if (PlayerStart->PlayerStartTag == FString::FromInt(BelligerentId))
		{
			return PlayerStart;
		}
	}

	return nullptr;
}
