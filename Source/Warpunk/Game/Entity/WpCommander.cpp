#include "WpCommander.h"

#include "Warpunk/Game/Navigation/WpUnitAIController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"

AWpCommander::AWpCommander()
{
}

void AWpCommander::PossessedBy(AController* const NewController)
{
	Super::PossessedBy(NewController);

	if (!AIController)
	{
		AIController = GetController<AWpUnitAIController>();
		ensureAlways(AIController);
	}

	const bool bPossessingPlayer = Cast<APlayerController>(NewController) != nullptr;
	if (bPossessingPlayer)
	{
		UCharacterMovementComponent* const MovementComp = Cast<UCharacterMovementComponent>(GetMovementComponent());
		MovementComp->bOrientRotationToMovement = false;
		bUseControllerRotationYaw = true;

		ClientPossessed();
	}
}

void AWpCommander::UnPossessed()
{
	const bool bUnPossessingPlayer = GetController<APlayerController>() != nullptr;
	if (bUnPossessingPlayer)
	{
		UCharacterMovementComponent* const MovementComponent = Cast<UCharacterMovementComponent>(GetMovementComponent());
		MovementComponent->bOrientRotationToMovement = true;
		bUseControllerRotationYaw = false;

		ClientUnPossessed();
	}

	Super::UnPossessed();

	if (bUnPossessingPlayer && AIController)
	{
		AIController->Possess(this);
	}
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
	EnhancedInputComponent->BindAction(FireIa, ETriggerEvent::Triggered, this, &AWpCommander::ServerOnFireTriggered);
}

void AWpCommander::ClientPossessed_Implementation()
{
	const ULocalPlayer* const LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (UEnhancedInputLocalPlayerSubsystem* const EnhancedInputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		ensureAlways(TacticalViewImc);
		EnhancedInputSystem->AddMappingContext(TacticalViewImc, 0);
	}

	APlayerController* const LocalPlayerController = LocalPlayer->GetPlayerController(GetWorld());
	LocalPlayerController->SetInputMode(FInputModeGameOnly{});
	LocalPlayerController->SetShowMouseCursor(false);

	UCharacterMovementComponent* const MovementComp = Cast<UCharacterMovementComponent>(GetMovementComponent());
	MovementComp->bOrientRotationToMovement = false;
	bUseControllerRotationYaw = true;
}

void AWpCommander::ClientUnPossessed_Implementation()
{
	const ULocalPlayer* const LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (UEnhancedInputLocalPlayerSubsystem* const EnhancedInputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		ensureAlways(TacticalViewImc);
		EnhancedInputSystem->RemoveMappingContext(TacticalViewImc);
	}

	UCharacterMovementComponent* const MovementComponent = Cast<UCharacterMovementComponent>(GetMovementComponent());
	MovementComponent->bOrientRotationToMovement = true;
	bUseControllerRotationYaw = false;
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

void AWpCommander::ServerOnFireTriggered_Implementation(const FInputActionValue& ActionValue)
{
}
