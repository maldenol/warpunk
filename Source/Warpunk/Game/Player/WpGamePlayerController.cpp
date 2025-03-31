#include "WpGamePlayerController.h"

#include "Warpunk/Game/Entity/WpCommander.h"
#include "Warpunk/Game/Movement/WpEntityMovementComponent.h"
#include "Warpunk/Game/Order/WpOrderSenderComponent.h"
#include "Warpunk/Game/Player/WpGamePlayerState.h"
#include "Warpunk/Game/Player/WpStrategicViewPawn.h"
#include "Warpunk/Global/GameInstance/WpGameInstance.h"
#include "Warpunk/Global/LoadingScreen/WpLoadingScreen.h"

#include "EnhancedInputComponent.h"

const FName AWpGamePlayerController::OrderSenderComponentName = TEXT("OrderSenderComponent");

AWpGamePlayerController::AWpGamePlayerController()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;

	bAttachToPawn = false;

	OrderSenderComponent = CreateDefaultSubobject<UWpOrderSenderComponent>(OrderSenderComponentName);
}

void AWpGamePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && IsLocalController())
	{
		GetGameInstance<UWpGameInstance>()->GetLoadingScreen()->NotifyObjectLoaded();
		bHasNotifiedObjectLoaded = true;
	}
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

void AWpGamePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	ensureAlways(ToggleViewIa);

	UEnhancedInputComponent* const EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	EnhancedInputComponent->BindAction(ToggleViewIa, ETriggerEvent::Triggered, this, &AWpGamePlayerController::ServerToggleView);
}

void AWpGamePlayerController::SetControlRotation(const FRotator& NewRotation)
{
	Super::SetControlRotation(NewRotation);

	if (!HasAuthority())
	{
		ServerSetControlRotation(NewRotation);
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

UWpOrderSenderComponent* AWpGamePlayerController::GetOrderSenderComponent()
{
	return OrderSenderComponent;
}

const UWpOrderSenderComponent* AWpGamePlayerController::GetOrderSenderComponent() const
{
	return OrderSenderComponent;
}

void AWpGamePlayerController::ClientSetExpectedLoadedObjectNum_Implementation(const uint32 ExpectedLoadedObjectNum)
{
	GetGameInstance<UWpGameInstance>()->GetLoadingScreen()->SetExpectedLoadedObjectNum(ExpectedLoadedObjectNum);
}

void AWpGamePlayerController::ServerSetCommanderMovementInput_Implementation(const float Forward, const float Right)
{
	if (Commander && (GetPawn<AWpCommander>() == Commander))
	{
		Commander->GetEntityMovementComponent()->ConsumeInputVector();
		Commander->GetEntityMovementComponent()->AddMovementInput(Forward, Right);
	}
}

void AWpGamePlayerController::NotifyCommanderDestroyed()
{
	const bool bFirstPersonView = GetPawn<AWpCommander>() == Commander;
	if (bFirstPersonView)
	{
		ServerToggleView();
	}
}

void AWpGamePlayerController::ServerSetControlRotation_Implementation(const FRotator& NewRotation)
{
	SetControlRotation(NewRotation);
}

void AWpGamePlayerController::ServerToggleView_Implementation()
{
	if (!StrategicViewPawn || !Commander)
	{
		return;
	}

	const bool bStrategicView = GetPawn<AWpStrategicViewPawn>() == StrategicViewPawn;

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
