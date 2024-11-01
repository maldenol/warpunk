#pragma once

#include "CoreMinimal.h"

class FWpProjectileWeapon;

struct FWpPooledProjectileData final
{
	FWpProjectileWeapon* Weapon = nullptr;
	float Radius = 0.0f;
	UStaticMesh* Mesh = nullptr;
	FVector InitialVelocity = FVector::ZeroVector;
};
