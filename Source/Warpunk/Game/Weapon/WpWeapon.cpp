#include "WpWeapon.h"

#include "Warpunk/Game/Belligerent/WpBelligerentComponent.h"
#include "Warpunk/Game/Endurance/WpEnduranceComponent.h"
#include "Warpunk/Game/Entity/WpEntity.h"
#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Pool/WpPooledActorVisibilityManager.h"
#include "Warpunk/Game/Weapon/WpPooledWeaponData.h"
#include "Warpunk/Game/Weapon/WpWeaponComponent.h"

const float FWpWeapon::UpdateCurrentTargetEntityCooldown = 0.25f;

void FWpWeapon::OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
{
	ensureAlways(OwnerEntity->HasAuthority());

	if (bIsAcquired)
	{
		const FWpPooledWeaponData& PooledWeaponData = static_cast<const FWpPooledWeaponData&>(PooledObjectData);
		const FWpWeaponConfig& WeaponConfig = *PooledWeaponData.WeaponConfig;

		Range = WeaponConfig.Range;
		MinRange = WeaponConfig.MinRange;
		Damage = WeaponConfig.Damage;
		Cooldown = WeaponConfig.Cooldown;
		FireLocation = WeaponConfig.FireLocation;
	}
	else
	{
		SetCurrentTargetEntity(nullptr);
		PriorityTargetEntity = nullptr;
	}
}

void FWpWeapon::Initialize(AWpEntity* const InOwnerEntity)
{
	OwnerEntity = InOwnerEntity;

	ensureAlways(OwnerEntity->HasAuthority());
}

void FWpWeapon::Tick(const float DeltaTime)
{
	ensureAlways(OwnerEntity->GetWeaponComponent()->IsActive());
	ensureAlways(OwnerEntity->HasAuthority());

	UpdateCurrentTargetEntity();

	if (CurrentTargetEntity)
	{
		MaybeFire(CurrentTargetEntity);
	}
}

bool FWpWeapon::MaybeFire(const AWpEntity* const Entity)
{
	const FVector Location = Entity->GetActorLocation();
	return MaybeFire(Location);
}

bool FWpWeapon::MaybeFire(const FVector& Location)
{
	ensureAlways(OwnerEntity->GetWeaponComponent()->IsActive());
	ensureAlways(OwnerEntity->HasAuthority());

	const float CurrentTime = OwnerEntity->GetWorld()->GetTimeSeconds();
	if ((CurrentTime >= LastFireTime + Cooldown) && CanReach(Location))
	{
		LastFireTime = CurrentTime;

		Fire(Location);

		return true;
	}

	return false;
}

bool FWpWeapon::CanReach(const AWpEntity* const Entity) const
{
	if (!Entity)
	{
		return false;
	}

	const FVector Location = Entity->GetActorLocation();
	return CanReach(Location);
}

bool FWpWeapon::CanReach(const FVector& Location) const
{
	ensureAlways(OwnerEntity->GetWeaponComponent()->IsActive());
	ensureAlways(OwnerEntity->HasAuthority());

	const FVector OwnerLocation = OwnerEntity->GetActorLocation();
	const float Distance2 = FVector::DistSquared2D(OwnerLocation, Location);
	return (Distance2 <= FMath::Square(Range)) && (Distance2 >= FMath::Square(MinRange));
}

float FWpWeapon::GetRange() const
{
	ensureAlways(OwnerEntity->HasAuthority());

	return Range;
}

float FWpWeapon::GetMinRange() const
{
	ensureAlways(OwnerEntity->HasAuthority());

	return MinRange;
}

void FWpWeapon::ClearCurrentTargetEntity()
{
	ensureAlways(OwnerEntity->HasAuthority());

	SetCurrentTargetEntity(nullptr);
}

AWpEntity* FWpWeapon::GetCurrentTargetEntity() const
{
	ensureAlways(OwnerEntity->GetWeaponComponent()->IsActive());
	ensureAlways(OwnerEntity->HasAuthority());

	return CurrentTargetEntity;
}

void FWpWeapon::SetPriorityTargetEntity(AWpEntity* const InPriorityTargetEntity)
{
	ensureAlways(OwnerEntity->GetWeaponComponent()->IsActive());
	ensureAlways(OwnerEntity->HasAuthority());

	if (InPriorityTargetEntity)
	{
		const UWpBelligerentComponent* const BelligerentComponent = OwnerEntity->GetBelligerentComponent();
		const bool bPriorityTargetBelongsToSameTeam = BelligerentComponent->BelongsToSameTeam(InPriorityTargetEntity->GetBelligerentComponent());
		if (bPriorityTargetBelongsToSameTeam)
		{
			return;
		}
	}

	PriorityTargetEntity = InPriorityTargetEntity;
}

void FWpWeapon::Fire(const FVector& Location)
{
	ensureAlways(OwnerEntity->GetWeaponComponent()->IsActive());
	ensureAlways(OwnerEntity->HasAuthority());
}

FRotator FWpWeapon::GetDirection(const FVector& Location) const
{
	const FVector OwnerLocation = OwnerEntity->GetActorLocation();
	return FRotator::MakeFromEuler(Location - OwnerLocation);
}

float FWpWeapon::GetDamage() const
{
	ensureAlways(OwnerEntity->GetWeaponComponent()->IsActive());
	ensureAlways(OwnerEntity->HasAuthority());

	return Damage;
}

const FVector& FWpWeapon::GetFireLocation() const
{
	ensureAlways(OwnerEntity->GetWeaponComponent()->IsActive());
	ensureAlways(OwnerEntity->HasAuthority());

	return FireLocation;
}

AWpEntity* FWpWeapon::GetOwnerEntity()
{
	ensureAlways(OwnerEntity->HasAuthority());

	return OwnerEntity;
}

const AWpEntity* FWpWeapon::GetOwnerEntity() const
{
	ensureAlways(OwnerEntity->HasAuthority());

	return OwnerEntity;
}

void FWpWeapon::SetCurrentTargetEntity(AWpEntity* const NewTargetEntity)
{
	if (NewTargetEntity)
	{
		ensureAlways(OwnerEntity->GetWeaponComponent()->IsActive());
	}
	ensureAlways(OwnerEntity->HasAuthority());

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
	ensureAlways(OwnerEntity->HasAuthority());

	const float CurrentTime = OwnerEntity->GetWorld()->GetTimeSeconds();
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
	const TSet<AWpEntity*>& EntitiesInReach = WeaponComponent->GetEntitiesInReach();

	if (CurrentTargetEntity)
	{
		const bool bCurrentTargetIsVisible = AWpPooledActorVisibilityManager::IsVisibleForTeam(EntityTeamId, CurrentTargetEntity);
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
		const bool bPriorityTargetIsVisible = AWpPooledActorVisibilityManager::IsVisibleForTeam(EntityTeamId, PriorityTargetEntity);
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

	for (AWpEntity* const CandidateEntity : EntitiesInReach)
	{
		const bool bCandidateIsCurrentTarget = CurrentTargetEntity == CandidateEntity;
		if (bCandidateIsCurrentTarget)
		{
			continue;
		}

		const bool bCandidateIsVisible = AWpPooledActorVisibilityManager::IsVisibleForTeam(EntityTeamId, CandidateEntity);
		if (!bCandidateIsVisible)
		{
			continue;
		}

		const bool bCandidateBelongsToSameTeam = OwnerEntity->GetBelligerentComponent()->BelongsToSameTeam(CandidateEntity->GetBelligerentComponent());
		if (bCandidateBelongsToSameTeam)
		{
			continue;
		}

		const bool bCanReachCandidate = CanReach(CandidateEntity->GetActorLocation());
		if (!bCanReachCandidate)
		{
			continue;
		}

		SetCurrentTargetEntity(CandidateEntity);
		break;
	}
}

void FWpWeapon::OnCurrentTargetEntityStateChanged(AWpEntity* const Entity, const EWpEntityState EntityState)
{
	if (EntityState == EWpEntityState::Wreck)
	{
		SetCurrentTargetEntity(nullptr);
	}
}
