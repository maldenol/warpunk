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
#include "Warpunk/Game/Pool/WpPooledActorVisibilityManager.h"
#include "Warpunk/Game/UI/WpGameHUD.h"
#include "Warpunk/Game/Weapon/WpProjectile.h"
#include "Warpunk/Global/GameInstance/WpGameInstance.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"

static constexpr uint32 UnitPoolSize = 50;
static constexpr uint32 BuildingPoolSize = 50;
static constexpr uint32 ProjectilePoolSize = 10;

AWpGameGameMode::AWpGameGameMode()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AWpGameGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	ensureAlways(GameStateClass->IsChildOf<AWpGameGameState>());
	ensureAlways(PlayerControllerClass->IsChildOf<AWpGamePlayerController>());
	ensureAlways(PlayerStateClass->IsChildOf<AWpGamePlayerState>());
	ensureAlways(HUDClass->IsChildOf<AWpGameHUD>());
	ensureAlways(DefaultPawnClass->IsChildOf<AWpStrategicViewPawn>());

	InitializeActorPools();
}

void AWpGameGameMode::HandleStartingNewPlayer_Implementation(APlayerController* const NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	AWpGamePlayerController* const PlayerController = Cast<AWpGamePlayerController>(NewPlayer);

	AWpStrategicViewPawn* const StrategicViewPawn = NewPlayer->GetPawn<AWpStrategicViewPawn>();
	ensureAlways(StrategicViewPawn);
	PlayerController->SetStrategicViewPawn(StrategicViewPawn);

	SpawnPooledActorVisibilityManager(PlayerController);

	InitializeCommander(PlayerController);

	SetExpectedLoadedObjectNum(PlayerController);
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

void AWpGameGameMode::InitializeActorPools() const
{
	AWpActorPool* const CommanderPool = GetWorld()->SpawnActor<AWpActorPool>();
	const uint32 PlayerNum = GetGameInstance<UWpGameInstance>()->GetPlayerIds().Num();
	CommanderPool->Reserve(CommanderClass, PlayerNum);

	AWpActorPool* const UnitPool = GetWorld()->SpawnActor<AWpActorPool>();
	UnitPool->Reserve(AWpUnit::StaticClass(), UnitPoolSize);

	AWpActorPool* const BuildingPool = GetWorld()->SpawnActor<AWpActorPool>();
	BuildingPool->Reserve(AWpBuilding::StaticClass(), BuildingPoolSize);

	AWpActorPool* const ProjectilePool = GetWorld()->SpawnActor<AWpActorPool>();
	ProjectilePool->Reserve(AWpProjectile::StaticClass(), ProjectilePoolSize);
}

void AWpGameGameMode::InitializeCommander(AWpGamePlayerController* const PlayerController) const
{
	constexpr uint32 ConfigId = 1;
	ensureAlways(ConfigId > 0);

	const UDataTable* const EntityConfigs = GetGameInstance<UWpGameInstance>()->EntityConfigs;
	const FName RowName = FName{FString::FromInt(ConfigId)};
	const FWpEntityConfig& EntityConfig = *EntityConfigs->FindRow<FWpEntityConfig>(RowName, nullptr);

	const AWpGamePlayerState* const PlayerState = PlayerController->GetPlayerState<AWpGamePlayerState>();
	FWpPooledEntityData PooledEntityData;
	PooledEntityData.EntityConfig = &EntityConfig;
	PooledEntityData.EntityConfigId = ConfigId;
	PooledEntityData.BelligerentId = PlayerState->GetBelligerentId();
	PooledEntityData.TeamId = PlayerState->GetTeamId();

	const FTransform Transform = PlayerController->GetPawn()->GetActorTransform();

	AWpActorPool* const CommanderPool = AWpActorPool::Instance(CommanderClass);
	AWpCommander* const Commander = Cast<AWpCommander>(CommanderPool->Acquire(Transform.GetLocation(), Transform.Rotator(), PooledEntityData));

	UWpEnduranceComponent* const EnduranceComponent = Commander->GetEnduranceComponent();
	EnduranceComponent->ReceiveRepair(EnduranceComponent->GetMatterCost());

	PlayerController->SetCommander(Commander);
}

void AWpGameGameMode::SpawnPooledActorVisibilityManager(const AWpGamePlayerController* const PlayerController) const
{
	const AWpGamePlayerState* const PlayerState = PlayerController->GetPlayerState<AWpGamePlayerState>();
	const uint32 TeamId = PlayerState->GetTeamId();

	if (!AWpPooledActorVisibilityManager::Instance(TeamId))
	{
		if (AWpPooledActorVisibilityManager::GetAuthorityTeamId() == 0)
		{
			AWpPooledActorVisibilityManager::SetAuthorityTeamId(TeamId);
		}

		AWpPooledActorVisibilityManager* const PooledActorVisibilityManager = GetWorld()->SpawnActor<AWpPooledActorVisibilityManager>();
		PooledActorVisibilityManager->SetTeamId(TeamId);
	}
}

void AWpGameGameMode::SetExpectedLoadedObjectNum(AWpGamePlayerController* const PlayerController) const
{
	const uint32 PlayerNum = GetGameInstance<UWpGameInstance>()->GetPlayerIds().Num();
	const uint32 ExpectedLoadedObjectNum =
		1 // Game state
		+ 1 // PlayerState field of local player controller
		+ 1 // Local strategic view pawn possession
		+ 2 * PlayerNum // For each player: player state and commander
		+ (UnitPoolSize + BuildingPoolSize + ProjectilePoolSize); // Pooled actors
	PlayerController->ClientSetExpectedLoadedObjectNum(ExpectedLoadedObjectNum);
}
