#include "WpHitScanWeapon.h"

#include "Warpunk/Game/EntityComponent/WpEntityComponent.h"

#include "Kismet/GameplayStatics.h"

bool FWpHitScanWeapon::CanReach(const FVector& TargetLocation) const
{
	if (!FWpWeapon::CanReach(TargetLocation))
	{
		return false;
	}

	return HitScan(TargetLocation).GetActor() == GetCurrentTarget()->GetOwner();
}

void FWpHitScanWeapon::Fire(const FVector& TargetLocation)
{
	FWpWeapon::Fire(TargetLocation);

	const FHitResult Hit = HitScan(TargetLocation);
	UGameplayStatics::ApplyPointDamage(Hit.GetActor(), GetDamage(), Hit.ImpactNormal, Hit, nullptr, nullptr, nullptr);
}

FHitResult FWpHitScanWeapon::HitScan(const FVector& TargetLocation) const
{
	const AActor* const OwnerActor = GetOwnerEntityComponent()->GetOwner();
	const FVector TraceStart = OwnerActor->GetActorLocation() + OwnerActor->GetActorRotation().RotateVector(GetFireLocation());
	const FVector TraceEnd = TraceStart + GetDirection(TargetLocation).Vector() * GetRange();

	FHitResult Hit;
	GetOwnerEntityComponent()->GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Camera);
	return Hit;
}
