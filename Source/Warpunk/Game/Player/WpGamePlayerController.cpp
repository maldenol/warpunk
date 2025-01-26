#include "WpGamePlayerController.h"

#include "Warpunk/Game/Entity/WpCommander.h"
#include "Warpunk/Game/MapLoad/WpMapLoadManager.h"
#include "Warpunk/Game/Player/WpStrategicViewPawn.h"

#include "EnhancedInputComponent.h"

AWpGamePlayerController::AWpGamePlayerController()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
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

	PlayerStateReplicatedEvent.Broadcast();
}

void AWpGamePlayerController::SetStrategicViewPawn(AWpStrategicViewPawn* const InStrategicViewPawn)
{
	StrategicViewPawn = InStrategicViewPawn;
}

void AWpGamePlayerController::SetCommander(AWpCommander* const InCommander)
{
	Commander = InCommander;
}

void AWpGamePlayerController::ClientSetExpectedActorNum_Implementation(const uint32 ExpectedActorNum)
{
	AWpMapLoadManager::Instance(this)->SetExpectedActorNum(ExpectedActorNum);
}

AWpGamePlayerController::FPlayerStateReplicatedEvent& AWpGamePlayerController::OnPlayerStateReplicated()
{
	return PlayerStateReplicatedEvent;
}

void AWpGamePlayerController::ServerToggleView_Implementation()
{
	const bool bStrategicView = GetPawn() == StrategicViewPawn;

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
