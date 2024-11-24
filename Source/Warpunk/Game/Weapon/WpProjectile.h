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

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData) override;

private:
	UFUNCTION()
	void OnComponentHit(UPrimitiveComponent* const HitComponent, AActor* const OtherActor, UPrimitiveComponent* const OtherComp, const FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnRep_SphereComponentRadius();
	UFUNCTION()
	void OnRep_MeshComponentStaticMesh();
	UFUNCTION()
	void OnRep_MeshComponentScale();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> SphereComponent = nullptr;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> MeshComponent = nullptr;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> MovementComponent = nullptr;

	FWpProjectileWeapon* Weapon = nullptr;

	UPROPERTY(ReplicatedUsing=OnRep_SphereComponentRadius)
	float SphereComponentRadius = 0.0f;
	UPROPERTY(ReplicatedUsing=OnRep_MeshComponentStaticMesh)
	TObjectPtr<UStaticMesh> MeshComponentStaticMesh = nullptr;
	UPROPERTY(ReplicatedUsing = OnRep_MeshComponentScale)
	FVector MeshComponentScale = FVector::OneVector;

	static const FName SphereComponentName;
	static const FName MeshComponentName;
	static const FName MovementComponentName;
	static const FName SphereComponentCollisionProfileName;
	static const FName MeshComponentCollisionProfileName;
};
