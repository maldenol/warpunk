#include "WpWeapon.h"

#include "Warpunk/Game/Belligerent/WpBelligerentComponent.h"
#include "Warpunk/Game/Endurance/WpEnduranceComponent.h"
#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Entity/WpEntityInterface.h"
#include "Warpunk/Game/Pool/WpPooledActorVisibilityManager.h"
#include "Warpunk/Game/Weapon/WpPooledWeaponData.h"
#include "Warpunk/Game/Weapon/WpWeaponComponent.h"

void FWpWeapon::OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
{
	ensureAlways(Cast<AActor>(OwnerEntity)->HasAuthority());

	if (bIsAcquired)
	{
		const FWpPooledWeaponData& PooledWeaponData = static_cast<const FWpPooledWeaponData&>(PooledObjectData);
		const FWpWeaponConfig& WeaponConfig = *PooledWeaponData.WeaponConfig;

		ensureAlways(!FMath::IsNearlyZero(WeaponConfig.Range));
		ensureAlways(!FMath::IsNearlyZero(WeaponConfig.Damage));
		ensureAlways(!WeaponConfig.FireLocation.IsNearlyZero());

		Range = WeaponConfig.Range;
		MinRange = WeaponConfig.MinRange;
		Cooldown = 1.0f / WeaponConfig.FireRate;
		Damage = WeaponConfig.Damage;
		FireLocation = WeaponConfig.FireLocation;
	}
	else
	{
		SetCurrentTargetEntity(nullptr);
		PriorityTargetEntity = nullptr;
	}
}

void FWpWeapon::Initialize(IWpEntityInterface* const InOwnerEntity)
{
	OwnerEntity = InOwnerEntity;

	ensureAlways(Cast<AActor>(OwnerEntity)->HasAuthority());
}

void FWpWeapon::Tick(const float DeltaTime)
{
	ensureAlways(OwnerEntity->GetWeaponComponent()->IsActive());
	ensureAlways(Cast<AActor>(OwnerEntity)->HasAuthority());

	UpdateCurrentTargetEntity();

	if (CurrentTargetEntity)
	{
		TryFire(CurrentTargetEntity);
	}
}

bool FWpWeapon::TryFire(const IWpEntityInterface* const Entity)
{
	const FVector Location = Cast<AActor>(Entity)->GetActorLocation();
	return TryFire(Location);
}

bool FWpWeapon::TryFire(const FVector& Location)
{
	ensureAlways(OwnerEntity->GetWeaponComponent()->IsActive());
	ensureAlways(Cast<AActor>(OwnerEntity)->HasAuthority());

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
	ensureAlways(OwnerEntity->GetWeaponComponent()->IsActive());
	ensureAlways(Cast<AActor>(OwnerEntity)->HasAuthority());

	const FVector OwnerLocation = Cast<AActor>(OwnerEntity)->GetActorLocation();
	const float Distance2 = FVector::DistSquared2D(OwnerLocation, Location);
	return (Distance2 <= FMath::Square(Range)) && (Distance2 >= FMath::Square(MinRange));
}

float FWpWeapon::GetRange() const
{
	ensureAlways(Cast<AActor>(OwnerEntity)->HasAuthority());

	return Range;
}

float FWpWeapon::GetMinRange() const
{
	ensureAlways(Cast<AActor>(OwnerEntity)->HasAuthority());

	return MinRange;
}

void FWpWeapon::ClearCurrentTargetEntity()
{
	ensureAlways(Cast<AActor>(OwnerEntity)->HasAuthority());

	SetCurrentTargetEntity(nullptr);
}

IWpEntityInterface* FWpWeapon::GetCurrentTargetEntity() const
{
	ensureAlways(OwnerEntity->GetWeaponComponent()->IsActive());
	ensureAlways(Cast<AActor>(OwnerEntity)->HasAuthority());

	return CurrentTargetEntity;
}

void FWpWeapon::SetPriorityTargetEntity(IWpEntityInterface* const InPriorityTargetEntity)
{
	ensureAlways(OwnerEntity->GetWeaponComponent()->IsActive());
	ensureAlways(Cast<AActor>(OwnerEntity)->HasAuthority());

	const UWpBelligerentComponent* const BelligerentComponent = OwnerEntity->GetBelligerentComponent();
	const bool bPriorityTargetBelongsToSameTeam = BelligerentComponent->BelongsToSameTeam(InPriorityTargetEntity->GetBelligerentComponent());
	if (bPriorityTargetBelongsToSameTeam)
	{
		return;
	}

	PriorityTargetEntity = InPriorityTargetEntity;
}

void FWpWeapon::Fire(const FVector& Location)
{
	ensureAlways(OwnerEntity->GetWeaponComponent()->IsActive());
	ensureAlways(Cast<AActor>(OwnerEntity)->HasAuthority());
}

FRotator FWpWeapon::GetDirection(const FVector& Location) const
{
	const FVector OwnerLocation = Cast<AActor>(OwnerEntity)->GetActorLocation();
	return FRotator::MakeFromEuler(Location - OwnerLocation);
}

float FWpWeapon::GetDamage() const
{
	ensureAlways(OwnerEntity->GetWeaponComponent()->IsActive());
	ensureAlways(Cast<AActor>(OwnerEntity)->HasAuthority());

	return Damage;
}

const FVector& FWpWeapon::GetFireLocation() const
{
	ensureAlways(OwnerEntity->GetWeaponComponent()->IsActive());
	ensureAlways(Cast<AActor>(OwnerEntity)->HasAuthority());

	return FireLocation;
}

IWpEntityInterface* FWpWeapon::GetOwnerEntity()
{
	ensureAlways(Cast<AActor>(OwnerEntity)->HasAuthority());

	return OwnerEntity;
}

const IWpEntityInterface* FWpWeapon::GetOwnerEntity() const
{
	ensureAlways(Cast<AActor>(OwnerEntity)->HasAuthority());

	return OwnerEntity;
}

void FWpWeapon::SetCurrentTargetEntity(IWpEntityInterface* const NewTargetEntity)
{
	if (NewTargetEntity)
	{
		ensureAlways(OwnerEntity->GetWeaponComponent()->IsActive());
	}
	ensureAlways(Cast<AActor>(OwnerEntity)->HasAuthority());

	if (CurrentTargetEntity)
	{
		UWpEnduranceComponent* const CurrentTargetEnduranceComponent = CurrentTargetEntity->GetEnduranceComponent();
		CurrentTargetEnduranceComponent->OnEntityStateChanged().RemoveAll(this);
	}

	if (NewTargetEntity)
	{
		UWpEnduranceComponent* const NewTargetEnduranceComponent = NewTargetEntity->GetEnduranceComponent();
		NewTargetEnduranceComponent->OnEntityStateChanged().AddRaw(this, &FWpWeapon::OnCurrentTargetEntityStateChanged);
	}

	CurrentTargetEntity = NewTargetEntity;
}

void FWpWeapon::UpdateCurrentTargetEntity()
{
	ensureAlways(OwnerEntity->GetWeaponComponent()->IsActive());
	ensureAlways(Cast<AActor>(OwnerEntity)->HasAuthority());

	const float CurrentTime = Cast<AActor>(OwnerEntity)->GetWorld()->GetTimeSeconds();
	if (CurrentTime >= LastUpdateCurrentTargetEntityTime + UpdateCurrentTargetEntityCooldown)
	{
		LastUpdateCurrentTargetEntityTime = CurrentTime;
	}
	else
	{
		return;
	}

	const uint32 EntityTeamId = OwnerEntity->GetBelligerentComponent()->GetTeamId();

	const UWpWeaponComponent* const WeaponComponent = OwnerEntity->GetWeaponComponent();
	const TSet<IWpEntityInterface*>& EntitiesInReach = WeaponComponent->GetEntitiesInReach();

	if (CurrentTargetEntity)
	{
		const AActor* const CurrentTargetActor = Cast<AActor>(CurrentTargetEntity);

		const bool bCurrentTargetIsVisible = AWpPooledActorVisibilityManager::IsVisibleForTeam(EntityTeamId, CurrentTargetActor);
		if (!bCurrentTargetIsVisible)
		{
			SetCurrentTargetEntity(nullptr);
		}
		else
		{
			const bool bCanReachCurrentTarget = EntitiesInReach.Contains(CurrentTargetEntity);
			if (!bCanReachCurrentTarget)
			{
				SetCurrentTargetEntity(nullptr);
			}
		}
	}

	const bool bCurrentTargetIsPriorityTarget = CurrentTargetEntity && (CurrentTargetEntity == PriorityTargetEntity);
	if (bCurrentTargetIsPriorityTarget)
	{
		return;
	}

	if (PriorityTargetEntity && EntitiesInReach.Contains(PriorityTargetEntity))
	{
		const bool bPriorityTargetIsVisible = AWpPooledActorVisibilityManager::IsVisibleForTeam(EntityTeamId, Cast<AActor>(PriorityTargetEntity));
		if (bPriorityTargetIsVisible)
		{
			SetCurrentTargetEntity(PriorityTargetEntity);
			return;
		}
	}

	if (CurrentTargetEntity)
	{
		return;
	}

	for (IWpEntityInterface* const CandidateEntity : EntitiesInReach)
	{
		const bool bCandidateIsCurrentTarget = (CurrentTargetEntity == CandidateEntity);
		if (bCandidateIsCurrentTarget)
		{
			continue;
		}

		const AActor* const CandidateActor = Cast<AActor>(CandidateEntity);

		const bool bCandidateIsVisible = AWpPooledActorVisibilityManager::IsVisibleForTeam(EntityTeamId, CandidateActor);
		if (!bCandidateIsVisible)
		{
			continue;
		}

		const bool bCandidateBelongsToSameTeam = OwnerEntity->GetBelligerentComponent()->BelongsToSameTeam(CandidateEntity->GetBelligerentComponent());
		if (bCandidateBelongsToSameTeam)
		{
			continue;
		}

		const bool bCanReachCandidate = CanReach(CandidateActor->GetActorLocation());
		if (!bCanReachCandidate)
		{
			continue;
		}

		SetCurrentTargetEntity(CandidateEntity);
		break;
	}
}

void FWpWeapon::OnCurrentTargetEntityStateChanged(IWpEntityInterface* const Entity, const EWpEntityState EntityState)
{
	if (EntityState == EWpEntityState::Wreck)
	{
		SetCurrentTargetEntity(nullptr);
	}
}
