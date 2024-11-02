#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"

#include "GameFramework/Actor.h"

#include "WpProjectile.generated.h"

class FWpProjectileWeapon;

class UProjectileMovementComponent;
class USphereComponent;

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class AWpProjectile final : public AActor, public IWpPooledObjectInterface
{
	GENERATED_BODY()

public:
	AWpProjectile();

	virtual void BeginPlay() override;

	virtual void OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const void* const CustomData) override;

	static const FName SphereComponentName;
	static const FName MeshComponentName;
	static const FName MovementComponentName;
	static const FName SphereComponentCollisionProfileName;
	static const FName MeshComponentCollisionProfileName;

private:
	UFUNCTION()
	void OnComponentHit(UPrimitiveComponent* const HitComponent, AActor* const OtherActor, UPrimitiveComponent* const OtherComp, const FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> SphereComponent = nullptr;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> MeshComponent = nullptr;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> MovementComponent = nullptr;

	FWpProjectileWeapon* Weapon = nullptr;
};
