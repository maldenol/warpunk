#include "WpUnitMovementComponent.h"

#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Entity/WpPooledEntityData.h"

#include "Net/UnrealNetwork.h"

UWpUnitMovementComponent::UWpUnitMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.SetTickFunctionEnable(false);

	SetIsReplicatedByDefault(true);

	NetworkNoSmoothUpdateDistance = 100.0f;

	FNavAgentProperties& NavAgentProperties = GetNavAgentPropertiesRef();
	NavAgentProperties.bCanCrouch = false;
	NavAgentProperties.bCanFly = false;
	NavAgentProperties.bCanJump = false;
	NavAgentProperties.bCanSwim = false;
	NavAgentProperties.bCanWalk = true;
}

void UWpUnitMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UWpUnitMovementComponent, Speed);
}

void UWpUnitMovementComponent::OnRep_IsActive()
{
	// Override to not enable tick
}

void UWpUnitMovementComponent::OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
{
	ensureAlways(GetOwner()->HasAuthority());

	if (bIsAcquired)
	{
		const FWpPooledEntityData& PooledEntityData = static_cast<const FWpPooledEntityData&>(PooledObjectData);
		const FWpUnitMovementConfig& UnitMovementConfig = PooledEntityData.EntityConfig->UnitMovementConfig;

		ensureAlways(UnitMovementConfig.Speed >= 1.0f);

		SetSpeed(UnitMovementConfig.Speed);

		SetIsFirstPersonControlled(false);
	}

	SetActive(bIsAcquired);
	SetComponentTickEnabled(bIsAcquired);
}

void UWpUnitMovementComponent::OnPooledObjectVisibilityChanged(const bool bIsVisible)
{
	if (!GetOwner()->HasAuthority())
	{
		SetComponentTickEnabled(bIsVisible);
	}
}

void UWpUnitMovementComponent::SetIsFirstPersonControlled(const bool bIsFirstPersonControlled)
{
	bOrientRotationToMovement = !bIsFirstPersonControlled;

	APawn* const OwnerPawn = GetOwner<APawn>();

	OwnerPawn->bUseControllerRotationYaw = bIsFirstPersonControlled;

	if (bIsFirstPersonControlled)
	{
		if (AController* const OwnerController = (OwnerPawn->GetLocalRole() == ROLE_SimulatedProxy) ? GetWorld()->GetFirstPlayerController() : OwnerPawn->GetController())
		{
			OwnerController->SetControlRotation(OwnerPawn->GetActorRotation());
		}
	}
}

void UWpUnitMovementComponent::SetSpeed(const float InSpeed)
{
	ensureAlways(GetOwner()->HasAuthority());

	Speed = InSpeed;
	OnRep_Speed();
}

void UWpUnitMovementComponent::OnRep_Speed()
{
	MaxWalkSpeed = Speed;
}
