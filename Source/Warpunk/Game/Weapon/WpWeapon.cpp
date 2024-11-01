#include "WpWeapon.h"

#include "Warpunk/Game/Data/WpEntityConfig.h"
#include "Warpunk/Game/EntityComponent/WpEntityComponent.h"
#include "Warpunk/Game/Intel/WpIntel.h"

void FWpWeapon::OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const void* const CustomData)
{
	if (bIsAcquired)
	{
		const FWpWeaponConfig* const WeaponConfig = static_cast<const FWpWeaponConfig*>(CustomData);

		ensureAlways(WeaponConfig->Range >= 1.0f);
		ensureAlways(WeaponConfig->Damage >= 0.001f);
		ensureAlways(!WeaponConfig->FireLocation.IsNearlyZero());

		Range = WeaponConfig->Range;
		Cooldown = 1.0f / WeaponConfig->FireRate;
		Damage = WeaponConfig->Damage;
		FireLocation = WeaponConfig->FireLocation;
	}
}

void FWpWeapon::Initialize(UWpEntityComponent* const InOwnerEntityComponent)
{
	OwnerEntityComponent = InOwnerEntityComponent;
}

void FWpWeapon::Tick(const float DeltaTime)
{
	FindNewTarget();
}

bool FWpWeapon::TryFire(const UWpEntityComponent* const TargetEntityComponent)
{
	const FVector TargetLocation = TargetEntityComponent->GetOwner()->GetActorLocation();
	return TryFire(TargetLocation);
}

bool FWpWeapon::TryFire(const FVector& TargetLocation)
{
	const float CurrentTime = OwnerEntityComponent->GetWorld()->GetTimeSeconds();
	if (CurrentTime >= LastFireTime + Cooldown && CanReach(TargetLocation))
	{
		LastFireTime = CurrentTime;

		Fire(TargetLocation);

		return true;
	}

	return false;
}

bool FWpWeapon::CanReach(const FVector& TargetLocation) const
{
	const FVector OwnerLocation = OwnerEntityComponent->GetOwner()->GetActorLocation();
	const float Distance2 = FVector::DistSquared2D(OwnerLocation, TargetLocation);

	return Distance2 <= Range * Range;
}

float FWpWeapon::GetRange() const
{
	return Range;
}

UWpEntityComponent* FWpWeapon::GetCurrentTarget() const
{
	return CurrentTargetEntityComponent;
}

void FWpWeapon::SetPriorityTarget(UWpEntityComponent* const InPriorityTargetEntityComponent)
{
	PriorityTargetEntityComponent = InPriorityTargetEntityComponent;
}

void FWpWeapon::Fire(const FVector& TargetLocation)
{
}

FRotator FWpWeapon::GetDirection(const FVector& TargetLocation) const
{
	const FVector OwnerLocation = OwnerEntityComponent->GetOwner()->GetActorLocation();
	return FRotator::MakeFromEuler(TargetLocation - OwnerLocation);
}

float FWpWeapon::GetDamage() const
{
	return Damage;
}

const FVector& FWpWeapon::GetFireLocation() const
{
	return FireLocation;
}

UWpEntityComponent* FWpWeapon::GetOwnerEntityComponent()
{
	return OwnerEntityComponent;
}

const UWpEntityComponent* FWpWeapon::GetOwnerEntityComponent() const
{
	return OwnerEntityComponent;
}

void FWpWeapon::FindNewTarget()
{
	const FWpIntel& Intel = WP_AS_INTEL_CONST(OwnerEntityComponent);
	for (UWpEntityComponent* const CandidateEntityComponent : Intel.GetIntel())
	{
		if (!IsValid(CandidateEntityComponent))
		{
			continue;
		}

		const FVector CandidateLocation = CandidateEntityComponent->GetOwner()->GetActorLocation();

		const bool bCanReachCandidate = CanReach(CandidateLocation);
		if (!bCanReachCandidate)
		{
			continue;
		}

		FVector CurrentTargetLocation;
		if (CurrentTargetEntityComponent)
		{
			CurrentTargetLocation = CurrentTargetEntityComponent->GetOwner()->GetActorLocation();
		}

		const bool bCandidateIsPriorityTarget = CandidateEntityComponent == PriorityTargetEntityComponent;
		const bool bCanReachCurrentTarget = CurrentTargetEntityComponent && CanReach(CurrentTargetLocation);
		const bool bCandidateBelongsToSameTeam = WP_AS_BELLIGERENT_MEMBER_CONST(OwnerEntityComponent).BelongsToSameTeam(CandidateEntityComponent);
		if (bCandidateIsPriorityTarget || (!bCanReachCurrentTarget && !bCandidateBelongsToSameTeam))
		{
			CurrentTargetEntityComponent = CandidateEntityComponent;
			TryFire(CurrentTargetEntityComponent);
			break;
		}
	}
}
