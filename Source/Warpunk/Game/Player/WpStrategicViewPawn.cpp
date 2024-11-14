#include "WpStrategicViewPawn.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/FloatingPawnMovement.h"

const FName AWpStrategicViewPawn::MovementComponentName = TEXT("FloatingPawnMovement");

AWpStrategicViewPawn::AWpStrategicViewPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	AutoPossessAI = EAutoPossessAI::Disabled;

	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(MovementComponentName);
}

void AWpStrategicViewPawn::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	FWpOrderSender::Tick();
}

void AWpStrategicViewPawn::SetupPlayerInputComponent(UInputComponent* const PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	ensureAlways(MoveIa);
	ensureAlways(LmbIa);
	ensureAlways(RmbIa);

	UEnhancedInputComponent* const EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	EnhancedInputComponent->BindAction(MoveIa, ETriggerEvent::Triggered, this, &AWpStrategicViewPawn::OnMoveTriggered);
	EnhancedInputComponent->BindAction(LmbIa, ETriggerEvent::Started, this, &FWpOrderSender::OnLMBStarted);
	EnhancedInputComponent->BindAction(LmbIa, ETriggerEvent::Triggered, this, &FWpOrderSender::OnLMBTriggered);
	EnhancedInputComponent->BindAction(LmbIa, ETriggerEvent::Completed, this, &FWpOrderSender::OnLMBCompleted);
	EnhancedInputComponent->BindAction(RmbIa, ETriggerEvent::Started, this, &FWpOrderSender::OnRMBStarted);
	EnhancedInputComponent->BindAction(RmbIa, ETriggerEvent::Triggered, this, &FWpOrderSender::OnRMBTriggered);
	EnhancedInputComponent->BindAction(RmbIa, ETriggerEvent::Completed, this, &FWpOrderSender::OnRMBCompleted);
}

void AWpStrategicViewPawn::PossessedBy(AController* const NewController)
{
	Super::PossessedBy(NewController);

	APlayerController* const NewPlayerController = Cast<APlayerController>(NewController);

	Initialize(NewPlayerController);

	if (const ULocalPlayer* const LocalPlayer = NewPlayerController->GetLocalPlayer())
	{
		UEnhancedInputLocalPlayerSubsystem* const EnhancedInputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
		ensureAlways(StrategicViewImc);
		EnhancedInputSystem->AddMappingContext(StrategicViewImc, 0);

		FInputModeGameAndUI InputMode = {};
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
		InputMode.SetHideCursorDuringCapture(false);
		NewPlayerController->SetInputMode(InputMode);
		NewPlayerController->SetShowMouseCursor(true);
	}
}

void AWpStrategicViewPawn::UnPossessed()
{
	const APlayerController* const OldPlayerController = GetController<APlayerController>();
	if (const ULocalPlayer* const LocalPlayer = OldPlayerController->GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* const EnhancedInputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			ensureAlways(StrategicViewImc);
			EnhancedInputSystem->RemoveMappingContext(StrategicViewImc);
		}
	}

	Super::UnPossessed();
}

void AWpStrategicViewPawn::OnMoveTriggered(const FInputActionValue& ActionValue)
{
	AddMovementInput(GetActorRightVector(), ActionValue[0]);
	AddMovementInput(GetActorForwardVector(), ActionValue[1]);
}
