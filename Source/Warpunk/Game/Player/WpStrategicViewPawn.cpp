#include "WpStrategicViewPawn.h"

#include "Warpunk/Game/Order/WpOrderSenderComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/FloatingPawnMovement.h"

const FName AWpStrategicViewPawn::MovementComponentName = TEXT("FloatingPawnMovement");
const FName AWpStrategicViewPawn::OrderSenderComponentName = TEXT("OrderSenderComponent");

AWpStrategicViewPawn::AWpStrategicViewPawn()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	Super::SetReplicateMovement(true);
	bOnlyRelevantToOwner = true;

	AutoPossessAI = EAutoPossessAI::Disabled;

	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(MovementComponentName);

	OrderSenderComponent = CreateDefaultSubobject<UWpOrderSenderComponent>(OrderSenderComponentName);
}

void AWpStrategicViewPawn::PossessedBy(AController* const NewController)
{
	Super::PossessedBy(NewController);

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

UWpOrderSenderComponent* AWpStrategicViewPawn::GetOrderSenderComponent()
{
	return OrderSenderComponent;
}

const UWpOrderSenderComponent* AWpStrategicViewPawn::GetOrderSenderComponent() const
{
	return OrderSenderComponent;
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
	SetActorLocation(Location);
}

void AWpStrategicViewPawn::OnMoveTriggered(const FInputActionValue& ActionValue)
{
	AddMovementInput(GetActorRightVector(), ActionValue[0]);
	AddMovementInput(GetActorForwardVector(), ActionValue[1]);

	ServerUpdateLocation(GetActorLocation());
}

void AWpStrategicViewPawn::OnLMBStarted()
{
	OrderSenderComponent->OnLMBStarted();
}

void AWpStrategicViewPawn::OnLMBTriggered()
{
	OrderSenderComponent->OnLMBTriggered();
}

void AWpStrategicViewPawn::OnLMBCompleted()
{
	OrderSenderComponent->OnLMBCompleted();
}

void AWpStrategicViewPawn::OnRMBStarted()
{
	OrderSenderComponent->OnRMBStarted();
}

void AWpStrategicViewPawn::OnRMBTriggered()
{
	OrderSenderComponent->OnRMBTriggered();
}

void AWpStrategicViewPawn::OnRMBCompleted()
{
	OrderSenderComponent->OnRMBCompleted();
}
