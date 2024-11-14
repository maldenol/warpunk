#include "WpCommander.h"

#include "Warpunk/Game/EntityComponent/WpEntityComponent.h"
#include "Warpunk/Game/Navigation/WpUnitAIController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"

AWpCommander::AWpCommander()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AWpCommander::SetupPlayerInputComponent(UInputComponent* const PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	ensureAlways(MoveIa);
	ensureAlways(LookIa);
	ensureAlways(FireIa);

	UEnhancedInputComponent* const EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	EnhancedInputComponent->BindAction(MoveIa, ETriggerEvent::Triggered, this, &AWpCommander::OnMoveTriggered);
	EnhancedInputComponent->BindAction(LookIa, ETriggerEvent::Triggered, this, &AWpCommander::OnLookTriggered);
	EnhancedInputComponent->BindAction(FireIa, ETriggerEvent::Triggered, this, &AWpCommander::OnFireTriggered);
}

void AWpCommander::PossessedBy(AController* const NewController)
{
	Super::PossessedBy(NewController);

	if (!AIController)
	{
		AIController = GetController<AWpUnitAIController>();
		ensureAlways(AIController);
	}

	APlayerController* const NewPlayerController = Cast<APlayerController>(NewController);
	if (!NewPlayerController)
	{
		return;
	}

	ensureAlways(TacticalViewImc);

	const ULocalPlayer* const LocalPlayer = NewPlayerController->GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* const EnhancedInputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	EnhancedInputSystem->AddMappingContext(TacticalViewImc, 0);

	NewPlayerController->SetInputMode(FInputModeGameOnly{});
	NewPlayerController->SetShowMouseCursor(false);

	NewPlayerController->StopMovement();

	UCharacterMovementComponent* const MovementComp = Cast<UCharacterMovementComponent>(GetMovementComponent());
	MovementComp->bOrientRotationToMovement = false;
	bUseControllerRotationYaw = true;
}

void AWpCommander::UnPossessed()
{
	const APlayerController* const OldPlayerController = GetController<APlayerController>();
	if (!OldPlayerController)
	{
		return;
	}

	ensureAlways(TacticalViewImc);

	const ULocalPlayer* const LocalPlayer = OldPlayerController->GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* const EnhancedInputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	EnhancedInputSystem->RemoveMappingContext(TacticalViewImc);

	UCharacterMovementComponent* const MovementComponent = Cast<UCharacterMovementComponent>(GetMovementComponent());
	MovementComponent->bOrientRotationToMovement = true;
	bUseControllerRotationYaw = false;

	Super::UnPossessed();

	AIController->Possess(this);
}

void AWpCommander::OnMoveTriggered(const FInputActionValue& ActionValue)
{
	AddMovementInput(GetActorRightVector(), ActionValue[0]);
	AddMovementInput(GetActorForwardVector(), ActionValue[1]);
}

void AWpCommander::OnLookTriggered(const FInputActionValue& ActionValue)
{
	AddControllerYawInput(ActionValue[0]);
	AddControllerPitchInput(ActionValue[1]);
}

void AWpCommander::OnFireTriggered(const FInputActionValue& ActionValue)
{
}
