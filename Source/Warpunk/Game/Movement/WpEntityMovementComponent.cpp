#include "WpEntityMovementComponent.h"

#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Entity/WpPooledEntityData.h"

#include "Net/UnrealNetwork.h"

UWpEntityMovementComponent::UWpEntityMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.SetTickFunctionEnable(false);

	SetIsReplicatedByDefault(true);

	// NetworkNoSmoothUpdateDistance = 100.0f; todo

	FNavAgentProperties& NavAgentProperties = GetNavAgentPropertiesRef();
	NavAgentProperties.bCanCrouch = false;
	NavAgentProperties.bCanFly = false;
	NavAgentProperties.bCanJump = false;
	NavAgentProperties.bCanSwim = false;
	NavAgentProperties.bCanWalk = true;
}

void UWpEntityMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void UWpEntityMovementComponent::OnRep_IsActive()
{
	// Override to not enable tick
}

void UWpEntityMovementComponent::OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
{
	ensureAlways(GetOwner()->HasAuthority());

	if (bIsAcquired)
	{
		const FWpPooledEntityData& PooledEntityData = static_cast<const FWpPooledEntityData&>(PooledObjectData);

		if (PooledEntityData.EntityConfig->Type == EWpEntityType::Unit)
		{
			const FWpMovementConfig& MovementConfig = PooledEntityData.EntityConfig->MovementConfig;

			SetIsFirstPersonControlled(false);
		}
		else
		{
			bIsAcquired = false;
		}
	}

	SetActive(bIsAcquired);
	SetComponentTickEnabled(bIsAcquired);
}

void UWpEntityMovementComponent::OnPooledObjectVisibilityChanged(const bool bIsVisible)
{
	if (!GetOwner()->HasAuthority())
	{
		SetComponentTickEnabled(bIsVisible);
	}
}

void UWpEntityMovementComponent::SetIsFirstPersonControlled(const bool bIsFirstPersonControlled)
{
	if (bIsFirstPersonControlled)
	{
		const APawn* const OwnerPawn = GetOwner<APawn>();
		if (AController* const OwnerController = (OwnerPawn->GetLocalRole() == ROLE_SimulatedProxy) ? GetWorld()->GetFirstPlayerController() : OwnerPawn->GetController())
		{
			OwnerController->SetControlRotation(OwnerPawn->GetActorRotation());
		}
	}
}
