#include "WpHitScanWeapon.h"

#include "Warpunk/Game/Entity/WpEntityInterface.h"

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
	const AActor* const OwnerActor = Cast<AActor>(GetOwnerEntity());
	const FVector TraceStart = OwnerActor->GetActorLocation() + OwnerActor->GetActorRotation().RotateVector(GetFireLocation());
	const FVector TraceEnd = TraceStart + GetDirection(Location).Vector() * GetRange();

	FHitResult Hit;
	OwnerActor->GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Camera);
	return Hit;
}
