#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Pool/WpPooledObjectData.h"

class FWpProjectileWeapon;

struct FWpPooledProjectileData final : FWpPooledObjectData
{
	FWpProjectileWeapon* Weapon = nullptr;
	float Radius = 0.0f;
	UStaticMesh* Mesh = nullptr;
	FVector InitialVelocity = FVector::ZeroVector;
};
