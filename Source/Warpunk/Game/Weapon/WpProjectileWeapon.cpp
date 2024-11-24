#include "WpProjectileWeapon.h"

#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Entity/WpEntityInterface.h"
#include "Warpunk/Game/Pool/WpActorPool.h"
#include "Warpunk/Game/Weapon/WpPooledProjectileData.h"
#include "Warpunk/Game/Weapon/WpPooledWeaponData.h"
#include "Warpunk/Game/Weapon/WpProjectile.h"

#include "Kismet/GameplayStatics.h"

void FWpProjectileWeapon::OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
{
	FWpWeapon::OnPooledObjectAcquiredOrReleased(bIsAcquired, PooledObjectData);

	if (bIsAcquired)
	{
		const FWpPooledWeaponData& PooledWeaponData = static_cast<const FWpPooledWeaponData&>(PooledObjectData);
		const FWpWeaponConfig& WeaponConfig = *PooledWeaponData.WeaponConfig;

		ensureAlways(WeaponConfig.ProjectileRadius >= 1.0f);
		ensureAlways(WeaponConfig.ProjectileMesh);

		ProjectileRadius = WeaponConfig.ProjectileRadius;
		ProjectileMesh = WeaponConfig.ProjectileMesh;
		ProjectileSpeed = FMath::Sqrt(GetRange() * -Cast<AActor>(GetOwnerEntity())->GetWorld()->GetGravityZ());
	}
}

void FWpProjectileWeapon::OnProjectileHit(IWpEntityInterface* const Entity)
{
	UGameplayStatics::ApplyPointDamage(Cast<AActor>(Entity), GetDamage(), FVector::ZeroVector, {}, nullptr, nullptr, nullptr);
}

void FWpProjectileWeapon::Fire(const FVector& TargetLocation)
{
	FWpWeapon::Fire(TargetLocation);

	const AActor* const OwnerActor = Cast<AActor>(GetOwnerEntity());
	const FVector OwnerActorLocation = OwnerActor->GetActorLocation();
	const FVector Location = OwnerActorLocation + OwnerActor->GetActorRotation().RotateVector(GetFireLocation());
	const float HorizontalDistance = FVector::Dist2D(OwnerActorLocation, TargetLocation);
	const float FireAngle = FMath::FastAsin(HorizontalDistance / GetRange()) / 2.0f;
	FVector ForwardVector = TargetLocation - OwnerActorLocation;
	ForwardVector.Normalize();
	const FVector PerpendicularVector = ForwardVector ^ FVector::UpVector;
	const FVector FireVector = ForwardVector.RotateAngleAxis(FMath::RadiansToDegrees(FireAngle), PerpendicularVector);
	const FVector InitialVelocity = FireVector * ProjectileSpeed;

	FWpPooledProjectileData PooledProjectileData;
	PooledProjectileData.Weapon = this;
	PooledProjectileData.Radius = ProjectileRadius;
	PooledProjectileData.Mesh = ProjectileMesh;
	PooledProjectileData.InitialVelocity = InitialVelocity;

	AWpActorPool::Acquire<AWpProjectile>(Location, FRotator::ZeroRotator, PooledProjectileData);
}
