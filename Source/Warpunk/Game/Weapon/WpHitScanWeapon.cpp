#include "WpHitScanWeapon.h"

#include "Warpunk/Game/Entity/WpEntity.h"

#include "Kismet/GameplayStatics.h"

bool FWpHitScanWeapon::CanReach(const FVector& Location) const
{
	if (!FWpWeapon::CanReach(Location))
	{
		return false;
	}

	return HitScan(Location).GetActor() == Cast<AActor>(GetCurrentTargetEntity());
}

void FWpHitScanWeapon::Fire(const FVector& Location)
{
	FWpWeapon::Fire(Location);

	const FHitResult Hit = HitScan(Location);
	UGameplayStatics::ApplyPointDamage(Hit.GetActor(), GetDamage(), Hit.ImpactNormal, Hit, nullptr, nullptr, nullptr);
}

FHitResult FWpHitScanWeapon::HitScan(const FVector& Location) const
{
	const FVector TraceStart = GetOwnerEntity()->GetActorLocation() + GetOwnerEntity()->GetActorRotation().RotateVector(GetFireLocation());
	const FVector TraceEnd = TraceStart + GetDirection(Location).Vector() * GetRange();

	FHitResult Hit;
	GetOwnerEntity()->GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Camera);
	return Hit;
}
