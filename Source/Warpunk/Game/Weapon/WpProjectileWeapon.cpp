#include "WpProjectileWeapon.h"

#include "Warpunk/Game/Entity/WpEntity.h"
#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Pool/WpActorPool.h"
#include "Warpunk/Game/Weapon/WpPooledProjectileData.h"
#include "Warpunk/Game/Weapon/WpPooledWeaponData.h"
#include "Warpunk/Game/Weapon/WpProjectile.h"
#include "Warpunk/Game/Weapon/WpWeaponComponent.h"

void FWpProjectileWeapon::OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
{
	FWpWeapon::OnPooledObjectAcquiredOrReleased(bIsAcquired, PooledObjectData);

	if (bIsAcquired)
	{
		const FWpPooledWeaponData& PooledWeaponData = static_cast<const FWpPooledWeaponData&>(PooledObjectData);
		const FWpWeaponConfig& WeaponConfig = *PooledWeaponData.WeaponConfig;

		ensureAlways(PooledWeaponData.ProjectileConfig.EntityConfigId > 0);

		ProjectileConfig = PooledWeaponData.ProjectileConfig;
		bHighAngle = WeaponConfig.bHighAngle;

		const float GravityZ = GetGravityZ();

		ProjectileSpeed2 = GetRange() * GravityZ;
		ProjectileSpeed4 = FMath::Square(ProjectileSpeed2);
		ProjectileSpeed = FMath::Sqrt(ProjectileSpeed2);

		ProjectileSpeedMinRange2 = GetMinRange() * GravityZ;
		ProjectileSpeedMinRange4 = FMath::Square(ProjectileSpeedMinRange2);
	}
}

bool FWpProjectileWeapon::CanReach(const FVector& Location) const
{
	ensureAlways(GetOwnerEntity()->GetWeaponComponent()->IsActive());
	ensureAlways(GetOwnerEntity()->HasAuthority());

	const FVector& TargetLocation = Location;

	FVector AzimuthVector, ProjectileLocation;
	CalcAzimuthVectorAndProjectileLocation(TargetLocation, AzimuthVector, ProjectileLocation);
	if (AzimuthVector.IsNearlyZero())
	{
		return false;
	}

	float HorizontalDelta, VerticalDelta;
	CalcLocationDelta(ProjectileLocation, TargetLocation, HorizontalDelta, VerticalDelta);
	const float GravityZ = GetGravityZ();

	const float Discriminant = CalcDiscriminant(HorizontalDelta, VerticalDelta, GravityZ);
	if (Discriminant < 0.0f)
	{
		return false;
	}

	const float DiscriminantMinRange = CalcDiscriminantMinRange(HorizontalDelta, VerticalDelta, GravityZ);
	if (DiscriminantMinRange > 0.0f)
	{
		return false;
	}

	return true;
}

void FWpProjectileWeapon::OnProjectileHit(AWpEntity* const Entity)
{
	ensureAlways(GetOwnerEntity()->HasAuthority());

	if (!GetOwnerEntity()->GetWeaponComponent()->IsActive())
	{
		return;
	}
}

void FWpProjectileWeapon::Fire(const FVector& Location)
{
	FWpWeapon::Fire(Location);

	const FVector& TargetLocation = Location;

	FVector AzimuthVector, ProjectileLocation;
	CalcAzimuthVectorAndProjectileLocation(TargetLocation, AzimuthVector, ProjectileLocation);
	if (AzimuthVector.IsNearlyZero())
	{
		return;
	}

	const float ElevationAngle = CalcElevationAngle(ProjectileLocation, TargetLocation);

	const FVector ElevationPerpendicularVector = AzimuthVector ^ FVector::UpVector;
	const FVector FireVector = AzimuthVector.RotateAngleAxis(ElevationAngle, ElevationPerpendicularVector);

	const FVector InitialVelocity = FireVector * ProjectileSpeed;

	FWpPooledProjectileData PooledProjectileData;
	PooledProjectileData.ProjectileConfig = ProjectileConfig;
	PooledProjectileData.InitialVelocity = InitialVelocity;
	PooledProjectileData.Damage = GetDamage();
	PooledProjectileData.Weapon = this;

	AWpActorPool::Acquire<AWpProjectile>(ProjectileLocation, FRotator::ZeroRotator, PooledProjectileData);
}

float FWpProjectileWeapon::CalcElevationAngle(const FVector& ProjectileLocation, const FVector& TargetLocation) const
{
	float HorizontalDelta, VerticalDelta;
	CalcLocationDelta(ProjectileLocation, TargetLocation, HorizontalDelta, VerticalDelta);
	const float GravityZ = GetGravityZ();

	const float Discriminant = CalcDiscriminant(HorizontalDelta, VerticalDelta, GravityZ);

	if (!ensureAlways(Discriminant >= 0.0f))
	{
		return -90.0f;
	}

	const float DiscriminantSqrt = FMath::Sqrt(Discriminant) * (bHighAngle ? 1.0f : -1.0f);

	const float ElevationAngleTan = (ProjectileSpeed2 + DiscriminantSqrt) / (HorizontalDelta * GravityZ);

	const float ElevationAngle = FMath::RadiansToDegrees(FMath::Atan(ElevationAngleTan));

	return ElevationAngle;
}

void FWpProjectileWeapon::CalcAzimuthVectorAndProjectileLocation(const FVector& TargetLocation, FVector& OutAzimuthVector, FVector& OutProjectileLocation) const
{
	const FVector OwnerActorLocation = GetOwnerEntity()->GetActorLocation();

	OutAzimuthVector = FVector{(FVector2D{TargetLocation} - FVector2D{OwnerActorLocation}).GetSafeNormal(), 0.0};

	OutProjectileLocation = OwnerActorLocation + OutAzimuthVector.ToOrientationQuat().RotateVector(GetFireLocation());
}

void FWpProjectileWeapon::CalcLocationDelta(const FVector& ProjectileLocation, const FVector& TargetLocation, float& OutHorizontalDelta, float& OutVerticalDelta)
{
	OutHorizontalDelta = FVector::Dist2D(TargetLocation, ProjectileLocation);
	OutVerticalDelta = TargetLocation.Z - ProjectileLocation.Z;
}

float FWpProjectileWeapon::CalcDiscriminant(const float HorizontalDelta, const float VerticalDelta, const float GravityZ) const
{
	const float Discriminant = ProjectileSpeed4
		- FMath::Square(HorizontalDelta) * FMath::Square(GravityZ)
		- VerticalDelta * ProjectileSpeed2 * GravityZ * 2.0f;

	return Discriminant;
}

float FWpProjectileWeapon::CalcDiscriminantMinRange(const float HorizontalDelta, const float VerticalDelta, const float GravityZ) const
{
	const float DiscriminantMinRange = ProjectileSpeedMinRange4
		- FMath::Square(HorizontalDelta) * FMath::Square(GravityZ)
		- VerticalDelta * ProjectileSpeedMinRange2 * GravityZ * 2.0f;

	return DiscriminantMinRange;
}

float FWpProjectileWeapon::GetGravityZ() const
{
	return -GetOwnerEntity()->GetWorld()->GetGravityZ();
}
