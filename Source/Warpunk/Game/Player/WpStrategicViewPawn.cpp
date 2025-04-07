#include "WpStrategicViewPawn.h"

#include "Warpunk/Game/Order/WpOrderSenderComponent.h"
#include "Warpunk/Game/Player/WpGamePlayerController.h"
#include "Warpunk/Global/GameInstance/WpGameInstance.h"
#include "Warpunk/Global/LoadingScreen/WpLoadingScreen.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/FloatingPawnMovement.h"

const FName AWpStrategicViewPawn::MovementComponentName = TEXT("FloatingPawnMovement");

AWpStrategicViewPawn::AWpStrategicViewPawn()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	Super::SetReplicateMovement(true);
	NetUpdateFrequency = 60.0f;

	AutoPossessAI = EAutoPossessAI::Disabled;

	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(MovementComponentName);
}

bool AWpStrategicViewPawn::IsNetRelevantFor(const AActor* const RealViewer, const AActor* const ViewTarget, const FVector& SrcLocation) const
{
	return OwningPlayerController && (OwningPlayerController == RealViewer);
}

void AWpStrategicViewPawn::PossessedBy(AController* const NewController)
{
	Super::PossessedBy(NewController);

	if (!OwningPlayerController && NewController->IsA<AWpGamePlayerController>())
	{
		OwningPlayerController = Cast<AActor>(NewController);
	}

	ClientPossessed();
}

void AWpStrategicViewPawn::UnPossessed()
{
	ClientUnPossessed();

	Super::UnPossessed();
}

void AWpStrategicViewPawn::SetupPlayerInputComponent(UInputComponent* const PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	ensureAlways(MoveIa);
	ensureAlways(LmbIa);
	ensureAlways(RmbIa);

	UEnhancedInputComponent* const EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	EnhancedInputComponent->BindAction(MoveIa, ETriggerEvent::Triggered, this, &AWpStrategicViewPawn::OnMoveTriggered);
	EnhancedInputComponent->BindAction(LmbIa, ETriggerEvent::Started, this, &AWpStrategicViewPawn::OnLMBStarted);
	EnhancedInputComponent->BindAction(LmbIa, ETriggerEvent::Triggered, this, &AWpStrategicViewPawn::OnLMBTriggered);
	EnhancedInputComponent->BindAction(LmbIa, ETriggerEvent::Completed, this, &AWpStrategicViewPawn::OnLMBCompleted);
	EnhancedInputComponent->BindAction(RmbIa, ETriggerEvent::Started, this, &AWpStrategicViewPawn::OnRMBStarted);
	EnhancedInputComponent->BindAction(RmbIa, ETriggerEvent::Triggered, this, &AWpStrategicViewPawn::OnRMBTriggered);
	EnhancedInputComponent->BindAction(RmbIa, ETriggerEvent::Completed, this, &AWpStrategicViewPawn::OnRMBCompleted);
}

void AWpStrategicViewPawn::ClientPossessed_Implementation()
{
	const ULocalPlayer* const LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (UEnhancedInputLocalPlayerSubsystem* const EnhancedInputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		ensureAlways(StrategicViewImc);
		EnhancedInputSystem->AddMappingContext(StrategicViewImc, 0);
	}

	APlayerController* const LocalPlayerController = LocalPlayer->GetPlayerController(GetWorld());
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	InputMode.SetHideCursorDuringCapture(false);
	LocalPlayerController->SetInputMode(InputMode);
	LocalPlayerController->SetShowMouseCursor(true);

	if (!bHasNotifiedObjectLoaded)
	{
		GetGameInstance<UWpGameInstance>()->GetLoadingScreen()->NotifyObjectLoaded();
		bHasNotifiedObjectLoaded = true;
	}
}

void AWpStrategicViewPawn::ClientUnPossessed_Implementation()
{
	const ULocalPlayer* const LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (UEnhancedInputLocalPlayerSubsystem* const EnhancedInputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		ensureAlways(StrategicViewImc);
		EnhancedInputSystem->RemoveMappingContext(StrategicViewImc);
	}
}

void AWpStrategicViewPawn::ServerUpdateLocation_Implementation(const FVector& Location)
{
	SetActorLocation(Location, false);
}

void AWpStrategicViewPawn::OnMoveTriggered(const FInputActionValue& ActionValue)
{
	AddMovementInput(GetActorRightVector(), ActionValue[0]);
	AddMovementInput(GetActorForwardVector(), ActionValue[1]);

	ServerUpdateLocation(GetActorLocation());
}

void AWpStrategicViewPawn::OnLMBStarted()
{
	if (AWpGamePlayerController* const PlayerController = GetController<AWpGamePlayerController>())
	{
		PlayerController->GetOrderSenderComponent()->OnLMBStarted();
	}
}

void AWpStrategicViewPawn::OnLMBTriggered()
{
	if (AWpGamePlayerController* const PlayerController = GetController<AWpGamePlayerController>())
	{
		PlayerController->GetOrderSenderComponent()->OnLMBTriggered();
	}
}

void AWpStrategicViewPawn::OnLMBCompleted()
{
	if (AWpGamePlayerController* const PlayerController = GetController<AWpGamePlayerController>())
	{
		PlayerController->GetOrderSenderComponent()->OnLMBCompleted();
	}
}

void AWpStrategicViewPawn::OnRMBStarted()
{
	if (AWpGamePlayerController* const PlayerController = GetController<AWpGamePlayerController>())
	{
		PlayerController->GetOrderSenderComponent()->OnRMBStarted();
	}
}

void AWpStrategicViewPawn::OnRMBTriggered()
{
	if (AWpGamePlayerController* const PlayerController = GetController<AWpGamePlayerController>())
	{
		PlayerController->GetOrderSenderComponent()->OnRMBTriggered();
	}
}

void AWpStrategicViewPawn::OnRMBCompleted()
{
	if (AWpGamePlayerController* const PlayerController = GetController<AWpGamePlayerController>())
	{
		PlayerController->GetOrderSenderComponent()->OnRMBCompleted();
	}
}
