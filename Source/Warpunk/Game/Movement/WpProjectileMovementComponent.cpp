#include "WpProjectileMovementComponent.h"

#include "Warpunk/Game/Entity/WpPooledEntityData.h"
#include "Warpunk/Game/Weapon/WpPooledProjectileData.h"

UWpProjectileMovementComponent::UWpProjectileMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.SetTickFunctionEnable(false);

	SetIsReplicatedByDefault(false);

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

void UWpProjectileMovementComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* const ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsActive())
	{
		return; // todo
	}

	ensureAlways(IsActive());
}

void UWpProjectileMovementComponent::OnRep_IsActive()
{
	// Override to not enable tick
}

void UWpProjectileMovementComponent::UpdateTickRegistration()
{
	// Override to not enable tick
}

void UWpProjectileMovementComponent::OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
{
	ensureAlways(GetOwner()->HasAuthority());

	if (bIsAcquired)
	{
		const FWpPooledProjectileData& PooledProjectileData = static_cast<const FWpPooledProjectileData&>(PooledObjectData);

		ensureAlways(!PooledProjectileData.InitialVelocity.IsNearlyZero());

		Velocity = PooledProjectileData.InitialVelocity;
	}

	SetActive(bIsAcquired);
	SetComponentTickEnabled(bIsAcquired);
}

void UWpProjectileMovementComponent::OnPooledObjectVisibilityChanged(const bool bIsVisible)
{
	if (!GetOwner()->HasAuthority())
	{
		SetComponentTickEnabled(bIsVisible);
	}
}
