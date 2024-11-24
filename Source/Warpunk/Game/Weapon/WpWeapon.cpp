#include "WpWeapon.h"

#include "Warpunk/Game/Belligerent/WpBelligerentComponent.h"
#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Entity/WpEntityInterface.h"
#include "Warpunk/Game/Intel/WpIntelComponent.h"
#include "Warpunk/Game/Weapon/WpPooledWeaponData.h"

void FWpWeapon::OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
{
	if (bIsAcquired)
	{
		const FWpPooledWeaponData& PooledWeaponData = static_cast<const FWpPooledWeaponData&>(PooledObjectData);
		const FWpWeaponConfig& WeaponConfig = *PooledWeaponData.WeaponConfig;

		ensureAlways(WeaponConfig.Range >= 1.0f);
		ensureAlways(WeaponConfig.Damage >= 0.001f);
		ensureAlways(!WeaponConfig.FireLocation.IsNearlyZero());

		Range = WeaponConfig.Range;
		Cooldown = 1.0f / WeaponConfig.FireRate;
		Damage = WeaponConfig.Damage;
		FireLocation = WeaponConfig.FireLocation;
	}
}

void FWpWeapon::Initialize(IWpEntityInterface* const InOwnerEntity)
{
	OwnerEntity = InOwnerEntity;
}

void FWpWeapon::Tick(const float DeltaTime)
{
	FindNewTarget();
}

bool FWpWeapon::TryFire(const IWpEntityInterface* const Entity)
{
	const FVector Location = Cast<AActor>(Entity)->GetActorLocation();
	return TryFire(Location);
}

bool FWpWeapon::TryFire(const FVector& Location)
{
	const float CurrentTime = Cast<AActor>(OwnerEntity)->GetWorld()->GetTimeSeconds();
	if (CurrentTime >= LastFireTime + Cooldown && CanReach(Location))
	{
		LastFireTime = CurrentTime;

		Fire(Location);

		return true;
	}

	return false;
}

bool FWpWeapon::CanReach(const IWpEntityInterface* const Entity) const
{
	if (!Entity)
	{
		return false;
	}

	const FVector Location = Cast<AActor>(Entity)->GetActorLocation();
	return CanReach(Location);
}

bool FWpWeapon::CanReach(const FVector& Location) const
{
	const FVector OwnerLocation = Cast<AActor>(OwnerEntity)->GetActorLocation();
	const float Distance2 = FVector::DistSquared2D(OwnerLocation, Location);

	return Distance2 <= Range * Range;
}

float FWpWeapon::GetRange() const
{
	return Range;
}

IWpEntityInterface* FWpWeapon::GetCurrentTargetEntity() const
{
	return CurrentTargetEntity;
}

void FWpWeapon::SetPriorityTargetEntity(IWpEntityInterface* const InPriorityTargetEntity)
{
	PriorityTargetEntity = InPriorityTargetEntity;
}

void FWpWeapon::Fire(const FVector& Location)
{
}

FRotator FWpWeapon::GetDirection(const FVector& Location) const
{
	const FVector OwnerLocation = Cast<AActor>(OwnerEntity)->GetActorLocation();
	return FRotator::MakeFromEuler(Location - OwnerLocation);
}

float FWpWeapon::GetDamage() const
{
	return Damage;
}

const FVector& FWpWeapon::GetFireLocation() const
{
	return FireLocation;
}

IWpEntityInterface* FWpWeapon::GetOwnerEntity()
{
	return OwnerEntity;
}

const IWpEntityInterface* FWpWeapon::GetOwnerEntity() const
{
	return OwnerEntity;
}

void FWpWeapon::FindNewTarget()
{
	const UWpIntelComponent* const IntelComponent = OwnerEntity->GetIntelComponent();
	for (IWpEntityInterface* const CandidateEntity : IntelComponent->GetIntel())
	{
		const FVector CandidateLocation = Cast<AActor>(CandidateEntity)->GetActorLocation();

		const bool bCanReachCandidate = CanReach(CandidateLocation);
		if (!bCanReachCandidate)
		{
			continue;
		}

		const bool bCandidateIsPriorityTarget = CandidateEntity == PriorityTargetEntity;
		const bool bCanReachCurrentTarget = CanReach(CurrentTargetEntity);
		const bool bCandidateBelongsToSameTeam = OwnerEntity->GetBelligerentComponent()->BelongsToSameTeam(CandidateEntity->GetBelligerentComponent());
		if (bCandidateIsPriorityTarget || (!bCanReachCurrentTarget && !bCandidateBelongsToSameTeam))
		{
			CurrentTargetEntity = CandidateEntity;
			TryFire(CurrentTargetEntity);
			return;
		}
	}

	if (CurrentTargetEntity)
	{
		TryFire(CurrentTargetEntity);
	}
}
