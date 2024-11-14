#include "WpGamePlayerController.h"

#include "Warpunk/Game/Entity/WpCommander.h"
#include "Warpunk/Game/Player/WpStrategicViewPawn.h"

#include "EnhancedInputComponent.h"

AWpGamePlayerController::AWpGamePlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AWpGamePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	ensureAlways(ToggleViewIa);

	UEnhancedInputComponent* const EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	EnhancedInputComponent->BindAction(ToggleViewIa, ETriggerEvent::Triggered, this, &AWpGamePlayerController::ToggleView);
}

void AWpGamePlayerController::SetStrategicViewPawn(AWpStrategicViewPawn* const InStrategicViewPawn)
{
	StrategicViewPawn = InStrategicViewPawn;
}

void AWpGamePlayerController::SetCommander(AWpCommander* const InCommander)
{
	Commander = InCommander;
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
