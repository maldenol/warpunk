#include "WpCommander.h"

#include "Warpunk/Game/Movement/WpEntityMovementComponent.h"
#include "Warpunk/Game/Navigation/WpEntityAIController.h"
#include "Warpunk/Game/Player/WpGamePlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"

const FName AWpCommander::CameraComponentName = TEXT("CameraComponent");

AWpCommander::AWpCommander()
{
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(CameraComponentName);
	CameraComponent->bUsePawnControlRotation = true;
	CameraComponent->AttachToComponent(MeshBaseComponent, FAttachmentTransformRules{EAttachmentRule::KeepRelative, false});
}

void AWpCommander::PossessedBy(AController* const NewController)
{
	Super::PossessedBy(NewController);

	if (!AIController)
	{
		AIController = GetController<AWpEntityAIController>();
		ensureAlways(AIController);
	}

	SetAutonomousProxy(false);

	const bool bPossessedByPlayer = Cast<APlayerController>(NewController) != nullptr;
	if (bPossessedByPlayer)
	{
		MovementComponent->SetIsFirstPersonControlled(true);

		ClientPossessed();
	}
}

void AWpCommander::UnPossessed()
{
	const bool bUnPossessedByPlayer = GetController<APlayerController>() != nullptr;
	if (bUnPossessedByPlayer)
	{
		MovementComponent->SetIsFirstPersonControlled(false);

		ClientUnPossessed();
	}

	Super::UnPossessed();

	if (bUnPossessedByPlayer && AIController)
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

void AWpCommander::OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
{
	Super::OnPooledObjectAcquiredOrReleased(bIsAcquired, PooledObjectData);

	if (!bIsAcquired)
	{
		if (AWpGamePlayerController* const PlayerController = GetController<AWpGamePlayerController>())
		{
			PlayerController->NotifyCommanderDestroyed();
		}
	}
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

	MovementComponent->SetIsFirstPersonControlled(true);
}

void AWpCommander::ClientUnPossessed_Implementation()
{
	const ULocalPlayer* const LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (UEnhancedInputLocalPlayerSubsystem* const EnhancedInputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		ensureAlways(TacticalViewImc);
		EnhancedInputSystem->RemoveMappingContext(TacticalViewImc);
	}

	MovementComponent->SetIsFirstPersonControlled(false);
}

void AWpCommander::OnMoveTriggered(const FInputActionValue& ActionValue)
{
	MovementComponent->AddMovementInput(ActionValue[1], ActionValue[0]);
}

void AWpCommander::OnLookTriggered(const FInputActionValue& ActionValue)
{
	AddControllerYawInput(ActionValue[0]);
	AddControllerPitchInput(ActionValue[1]);
}

void AWpCommander::ServerOnFireTriggered_Implementation(const FInputActionValue& ActionValue)
{
}
