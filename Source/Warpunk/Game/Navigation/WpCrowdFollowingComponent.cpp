#include "WpCrowdFollowingComponent.h"

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

void UWpCrowdFollowingComponent::ApplyCrowdAgentVelocity(const FVector& NewVelocity, const FVector& DestPathCorner, const bool bTraversingLink, const bool bIsNearEndOfPath)
{
	const UWpEntityMovementComponent* const EntityMovementComponent = Cast<UWpEntityMovementComponent>(MovementComp);
	if (IsCrowdSimulationEnabled() && (Status == EPathFollowingStatus::Moving) && EntityMovementComponent)
	{
		UpdateCachedDirections(NewVelocity, DestPathCorner, bTraversingLink);

		const FVector CurrentLocation = MovementComp->GetActorFeetLocation();
		const FVector EndLocation = Path->GetEndLocation();
		const float DistanceSqrd = FVector::DistSquared(CurrentLocation, EndLocation);
		const float BrakingDistanceSqrd = FMath::Square(EntityMovementComponent->GetBrakingDistance());
		const bool bShouldDecelerate = DistanceSqrd < BrakingDistanceSqrd;
		const FVector MoveInput = NewVelocity * (bShouldDecelerate ? -1.0 : 1.0);

		MovementComp->RequestPathMove(MoveInput);
	}
}
