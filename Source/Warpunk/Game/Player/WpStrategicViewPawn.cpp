#include "WpStrategicViewPawn.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/FloatingPawnMovement.h"

static const FName MovementComponentName = TEXT("FloatingPawnMovement");

AWpStrategicViewPawn::AWpStrategicViewPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(MovementComponentName);
}

void AWpStrategicViewPawn::BeginPlay()
{
	Super::BeginPlay();

	Initialize(GetController<APlayerController>());
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

	ensureAlways(StrategicViewImc);

	APlayerController* const NewPlayerController = Cast<APlayerController>(NewController);
	const ULocalPlayer* const LocalPlayer = NewPlayerController->GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* const EnhancedInputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	EnhancedInputSystem->AddMappingContext(StrategicViewImc, 0);

	NewPlayerController->SetShowMouseCursor(true);
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	InputMode.SetHideCursorDuringCapture(false);
	NewPlayerController->SetInputMode(InputMode);
}

void AWpStrategicViewPawn::UnPossessed()
{
	ensureAlways(StrategicViewImc);

	const APlayerController* const OldPlayerController = GetController<APlayerController>();
	const ULocalPlayer* const LocalPlayer = OldPlayerController->GetLocalPlayer();
	UEnhancedInputLocalPlayerSubsystem* const EnhancedInputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	EnhancedInputSystem->RemoveMappingContext(StrategicViewImc);

	Super::UnPossessed();
}

uint32 AWpStrategicViewPawn::GetBelligerentID() const
{
	return BelligerentID;
}

void AWpStrategicViewPawn::OnMoveTriggered(const FInputActionValue& ActionValue)
{
	AddMovementInput(GetActorRightVector(), ActionValue[0]);
	AddMovementInput(GetActorForwardVector(), ActionValue[1]);
}
