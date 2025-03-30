#include "WpProjectileMovementComponent.h"

#include "Warpunk/Game/Entity/WpPooledEntityData.h"
#include "Warpunk/Game/Weapon/WpPooledProjectileData.h"

UWpProjectileMovementComponent::UWpProjectileMovementComponent()
{
	bAutoActivate = false;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetIsReplicatedByDefault(false);

	bAutoUpdateTickRegistration = false;

	bRotationFollowsVelocity = true;
	bRotationRemainsVertical = false;

	InterpLocationSnapToTargetDistance = 100.0f;
}

void UWpProjectileMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	const bool bHasAuthority = GetOwner()->HasAuthority();
	bSimulationEnabled = bHasAuthority;
	bInterpMovement = !bHasAuthority;
	bInterpRotation = !bHasAuthority;
}

void UWpProjectileMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	// Do not call base function so bIsActive is not replicated
}

void UWpProjectileMovementComponent::OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
{
	if (!bIsAcquired)
	{
		ensureAlways(GetOwner()->HasAuthority());
	}

	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	if (bIsAcquired)
	{
		const FWpPooledProjectileData& PooledProjectileData = static_cast<const FWpPooledProjectileData&>(PooledObjectData);

		ensureAlways(!PooledProjectileData.InitialVelocity.IsNearlyZero());

		Velocity = PooledProjectileData.InitialVelocity;
	}

	SetActive(bIsAcquired);
}

void UWpProjectileMovementComponent::OnPooledObjectVisibilityChanged(const bool bIsVisible)
{
	if (!GetOwner()->HasAuthority())
	{
		SetActive(bIsVisible);
	}
}
