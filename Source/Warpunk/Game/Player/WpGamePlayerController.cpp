#include "WpGamePlayerController.h"

#include "Warpunk/Game/Entity/WpCommander.h"
#include "Warpunk/Game/Player/WpGamePlayerState.h"
#include "Warpunk/Game/Player/WpStrategicViewPawn.h"
#include "Warpunk/Global/GameInstance/WpGameInstance.h"
#include "Warpunk/Global/LoadingScreen/WpLoadingScreen.h"

#include "EnhancedInputComponent.h"

AWpGamePlayerController::AWpGamePlayerController()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
}

void AWpGamePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalController())
	{
		GetGameInstance<UWpGameInstance>()->GetLoadingScreen()->NotifyObjectLoaded();
	}
}

void AWpGamePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	ensureAlways(ToggleViewIa);

	UEnhancedInputComponent* const EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	EnhancedInputComponent->BindAction(ToggleViewIa, ETriggerEvent::Triggered, this, &AWpGamePlayerController::ServerToggleView);
}

void AWpGamePlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (!bHasNotifiedObjectLoaded && GetPlayerState<AWpGamePlayerState>())
	{
		GetGameInstance<UWpGameInstance>()->GetLoadingScreen()->NotifyObjectLoaded();
		bHasNotifiedObjectLoaded = true;
	}
}

void AWpGamePlayerController::SetStrategicViewPawn(AWpStrategicViewPawn* const InStrategicViewPawn)
{
	StrategicViewPawn = InStrategicViewPawn;
}

void AWpGamePlayerController::SetCommander(AWpCommander* const InCommander)
{
	Commander = InCommander;
}

void AWpGamePlayerController::ClientSetExpectedLoadedObjectNum_Implementation(const uint32 ExpectedLoadedObjectNum)
{
	GetGameInstance<UWpGameInstance>()->GetLoadingScreen()->SetExpectedLoadedObjectNum(ExpectedLoadedObjectNum);
}

void AWpGamePlayerController::ServerToggleView_Implementation()
{
	if (!StrategicViewPawn || !Commander)
	{
		return;
	}

	const bool bStrategicView = (GetPawn() == StrategicViewPawn);

	SetView(!bStrategicView);
}

void AWpGamePlayerController::SetView(const bool bStrategicView)
{
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
