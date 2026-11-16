#include "WpCrowdFollowingComponent.h"

#include "Warpunk/Game/Entity/WpEntity.h"
#include "Warpunk/Game/Movement/WpEntityMovementComponent.h"

UWpCrowdFollowingComponent::UWpCrowdFollowingComponent()
{
	bAutoActivate = true;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	SetIsReplicatedByDefault(false);
}

void UWpCrowdFollowingComponent::Reset()
{
	Super::Reset();

	bUseBlockDetection = false;

	bCanCheckMovingTooFar = false;
	bCheckMovementAngle = false;
}

void UWpCrowdFollowingComponent::Cleanup()
{
	if (GetOwner() && !GetOwner()->IsActorBeingDestroyed())
	{
		return;
	}

	Super::Cleanup();
}

void UWpCrowdFollowingComponent::GetCrowdAgentCollisions(float& CylinderRadius, float& CylinderHalfHeight) const
{
	CylinderRadius = 0.0;
	CylinderHalfHeight = 0.0;

	if (!NavMovementInterface.IsValid())
	{
		return;
	}

	const UWpEntityMovementComponent* const EntityMovementComponent = Cast<UWpEntityMovementComponent>(NavMovementInterface.GetObject());
	if (const AWpEntity* const Entity = EntityMovementComponent->GetOwner<AWpEntity>())
	{
		const FVector Extent = Entity->GetCollisionComponent()->GetCollisionShape().GetExtent();
		CylinderRadius = FMath::Max(Extent.X, Extent.Y);
		CylinderHalfHeight = Extent.Z;
	}
}

void UWpCrowdFollowingComponent::ApplyCrowdAgentVelocity(const FVector& NewVelocity, const FVector& DestPathCorner, const bool bTraversingLink, const bool bIsNearEndOfPath)
{
	if (!NavMovementInterface.IsValid())
	{
		return;
	}

	const UWpEntityMovementComponent* const EntityMovementComponent = Cast<UWpEntityMovementComponent>(NavMovementInterface.GetObject());
	if (IsCrowdSimulationEnabled() && (GetStatus() == EPathFollowingStatus::Moving) && EntityMovementComponent)
	{
		UpdateCachedDirections(NewVelocity, DestPathCorner, bTraversingLink);

		const FVector CurrentLocation = EntityMovementComponent->GetActorFeetLocation();
		const FVector EndLocation = Path->GetEndLocation();
		const float DistanceSqrd = FVector::DistSquared(CurrentLocation, EndLocation);
		const float BrakingDistanceSqrd = FMath::Square(EntityMovementComponent->GetMinBrakingDistance());
		const bool bShouldDecelerate = DistanceSqrd < BrakingDistanceSqrd;
		const FVector MoveInput = NewVelocity * (bShouldDecelerate ? 0.0 : 1.0);

		NavMovementInterface->RequestPathMove(MoveInput);
	}
}
