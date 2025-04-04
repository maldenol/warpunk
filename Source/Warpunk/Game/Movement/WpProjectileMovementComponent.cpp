#include "WpProjectileMovementComponent.h"

#include "Warpunk/Game/Entity/WpPooledEntityData.h"
#include "Warpunk/Game/Weapon/WpPooledProjectileData.h"

#include "GameFramework/GameStateBase.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Net/UnrealNetwork.h"

UWpProjectileMovementComponent::UWpProjectileMovementComponent()
{
	bAutoActivate = false;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetIsReplicatedByDefault(true);

	bAutoUpdateTickRegistration = false;

	bSimulationEnabled = true;
	bInterpMovement = false;
	bInterpRotation = false;
	bRotationFollowsVelocity = true;
	bRotationRemainsVertical = false;
}

void UWpProjectileMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DISABLE_REPLICATED_PRIVATE_PROPERTY(UActorComponent, bIsActive);

	FDoRepLifetimeParams DoRepLifetimeParams;
	DoRepLifetimeParams.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(UWpProjectileMovementComponent, PredictionData, DoRepLifetimeParams);
}

void UWpProjectileMovementComponent::StopSimulating(const FHitResult& HitResult)
{
	// Override to not clear UpdatedComponent
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

	UpdatePredictionData();
}

void UWpProjectileMovementComponent::UpdatePredictionData()
{
	ensureAlways(GetOwner()->HasAuthority());

	PredictionData.bIsAcquired = IsActive();
	if (PredictionData.bIsAcquired)
	{
		PredictionData.ServerTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
		PredictionData.UpdatedComponentLocation = UpdatedComponent->GetComponentLocation();
		PredictionData.UpdatedComponentRotation = UpdatedComponent->GetComponentRotation();
		PredictionData.Velocity = Velocity;
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(UWpProjectileMovementComponent, PredictionData, this);
}

void UWpProjectileMovementComponent::OnRep_PredictionData()
{
	ensureAlways(!GetOwner()->HasAuthority());

	SetActive(PredictionData.bIsAcquired);

	if (!PredictionData.bIsAcquired)
	{
		return;
	}

	UpdatedComponent->SetWorldLocationAndRotation(PredictionData.UpdatedComponentLocation, PredictionData.UpdatedComponentRotation, false);

	Velocity = PredictionData.Velocity;

	if (const AGameStateBase* const GameState = GetWorld()->GetGameState())
	{
		const float DeltaTime = GameState->GetServerWorldTimeSeconds() - PredictionData.ServerTime;
		if (DeltaTime > MIN_TICK_TIME)
		{
			TickComponent(DeltaTime, LEVELTICK_All, &PrimaryComponentTick);
		}
	}
}
