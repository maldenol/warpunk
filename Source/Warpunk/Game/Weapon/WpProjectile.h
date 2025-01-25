#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Pool/WpPooledActorInterface.h"
#include "Warpunk/Game/Weapon/WpProjectileConfig.h"

#include "GameFramework/Actor.h"

#include "WpProjectile.generated.h"

class FWpProjectileWeapon;

class UProjectileMovementComponent;
class USphereComponent;

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class AWpProjectile final : public AActor, public IWpPooledActorInterface
{
	GENERATED_BODY()

public:
	AWpProjectile();

	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool GetNetDormancy(const FVector& ViewPos, const FVector& ViewDir, AActor* const Viewer, AActor* const ViewTarget, UActorChannel* const InChannel, const float Time, const bool bLowBandwidth) override;

	virtual void PostNetReceiveLocationAndRotation() override;

	virtual void OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData) override;

	virtual void OnPooledActorVisibilityChanged(const bool bIsVisible) override;

private:
	UFUNCTION()
	void OnComponentHit(UPrimitiveComponent* const HitComponent, AActor* const OtherActor, UPrimitiveComponent* const OtherComp, const FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnRep_ProjectileConfig();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> SphereComponent = nullptr;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> MeshComponent = nullptr;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> MovementComponent = nullptr;

	UPROPERTY(ReplicatedUsing=OnRep_ProjectileConfig)
	FWpProjectileConfig ProjectileConfig = {};
	float Damage = 0.0f;
	FWpProjectileWeapon* Weapon = nullptr;

	static const FName SphereComponentName;
	static const FName MeshComponentName;
	static const FName MovementComponentName;
	static const FName SphereComponentCollisionProfileName;
	static const FName MeshComponentCollisionProfileName;
};
