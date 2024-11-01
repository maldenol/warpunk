#include "WpGamePlayerController.h"

#include "Warpunk/Game/Data/WpEntityConfig.h"
#include "Warpunk/Game/Data/WpPooledEntityData.h"
#include "Warpunk/Game/Entity/WpCommander.h"
#include "Warpunk/Game/EntityComponent/WpEntityComponent.h"
#include "Warpunk/Game/GameMode/WpGameGameState.h"
#include "Warpunk/Game/Player/WpStrategicViewPawn.h"

#include "EnhancedInputComponent.h"

AWpGamePlayerController::AWpGamePlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AWpGamePlayerController::BeginPlay()
{
	Super::BeginPlay();

	StrategicViewPawn = GetPawn<AWpStrategicViewPawn>();
	ensureAlways(StrategicViewPawn);

	const uint32 ConfigID = GetWorld()->GetGameState<AWpGameGameState>()->CommanderConfigID;
	ensureAlways(ConfigID > 0);
	const UDataTable* const EntityConfigs = GetWorld()->GetGameState<AWpGameGameState>()->GetEntityConfigs();
	const FName RowName = FName{FString::FromInt(ConfigID)};
	const FWpEntityConfig& EntityConfig = *EntityConfigs->FindRow<FWpEntityConfig>(RowName, nullptr);

	FActorSpawnParameters ActorSpawnParameters;
	ActorSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Commander = GetWorld()->SpawnActor<AWpCommander>(EntityConfig.EntityClass, StrategicViewPawn->GetActorTransform(), ActorSpawnParameters);
	ensureAlways(Commander);

	FWpPooledEntityData PooledEntityData;
	PooledEntityData.EntityConfig = &EntityConfig;
	PooledEntityData.EntityConfigID = ConfigID;
	PooledEntityData.BelligerentID = StrategicViewPawn->GetBelligerentID();

	Commander->AcquirePooledObject(&PooledEntityData);

	FWpEndurance& CommanderEndurance = WP_AS_ENDURANCE(Commander->GetEntityComponent());
	CommanderEndurance.ReceiveRepair(CommanderEndurance.GetMatterCost());

	SetView(bStrategicView);
}

void AWpGamePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	ensureAlways(ToggleViewIa);

	UEnhancedInputComponent* const EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	EnhancedInputComponent->BindAction(ToggleViewIa, ETriggerEvent::Triggered, this, &AWpGamePlayerController::ToggleView);
}

void AWpGamePlayerController::SetView(const bool bInStrategicView)
{
	bStrategicView = bInStrategicView;

	ensureAlways(StrategicViewPawn);
	ensureAlways(Commander);

	if (bStrategicView)
	{
		Possess(StrategicViewPawn);
	}
	else
	{
		Possess(Commander);
	}
}

void AWpGamePlayerController::ToggleView()
{
	SetView(!bStrategicView);
}
