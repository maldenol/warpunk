#include "WpProjectile.h"

#include "Warpunk/Game/Data/WpPooledProjectileData.h"
#include "Warpunk/Game/EntityComponent/WpEntityInterface.h"
#include "Warpunk/Game/Pool/WpActorPool.h"
#include "Warpunk/Game/Weapon/WpProjectileWeapon.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

static const FName SphereComponentName = TEXT("SphereCollision");
static const FName MeshComponentName = TEXT("StaticMesh");
static const FName MovementComponentName = TEXT("ProjectileMovement");

static const FName SphereComponentCollisionProfileName = TEXT("WpProjectile");
static const FName MeshComponentCollisionProfileName = TEXT("NoCollision");

AWpProjectile::AWpProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	SphereComponent = CreateDefaultSubobject<USphereComponent>(SphereComponentName);
	SphereComponent->SetCollisionProfileName(SphereComponentCollisionProfileName);
	SphereComponent->SetNotifyRigidBodyCollision(true);
	SphereComponent->SetGenerateOverlapEvents(false);
	RootComponent = SphereComponent;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(MeshComponentName);
	MeshComponent->SetCollisionProfileName(MeshComponentCollisionProfileName);
	MeshComponent->SetNotifyRigidBodyCollision(false);
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules{EAttachmentRule::KeepRelative, false});

	MovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(MovementComponentName);
}

void AWpProjectile::BeginPlay()
{
	Super::BeginPlay();

	SphereComponent->OnComponentHit.AddDynamic(this, &AWpProjectile::OnComponentHit);
}

void AWpProjectile::OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const void* const CustomData)
{
	if (bIsAcquired)
	{
		const FWpPooledProjectileData* const PooledProjectileData = static_cast<const FWpPooledProjectileData*>(CustomData);

		ensureAlways(PooledProjectileData->Radius >= 1.0f);
		ensureAlways(PooledProjectileData->Weapon);
		ensureAlways(PooledProjectileData->Mesh);
		ensureAlways(!PooledProjectileData->InitialVelocity.IsNearlyZero());

		const float Radius = PooledProjectileData->Radius;

		Weapon = PooledProjectileData->Weapon;
		SphereComponent->SetSphereRadius(Radius);
		MeshComponent->SetStaticMesh(PooledProjectileData->Mesh);
		MovementComponent->Velocity = PooledProjectileData->InitialVelocity;

		const FVector MeshExtent = MeshComponent->GetStaticMesh()->GetBounds().BoxExtent;
		const float ScaleX = Radius / MeshExtent.X;
		const float ScaleY = Radius / MeshExtent.Y;
		const float ScaleZ = Radius / MeshExtent.Z;
		MeshComponent->SetRelativeScale3D(FVector{ScaleX, ScaleY, ScaleZ});
	}

	SetActorEnableCollision(bIsAcquired);
	SetActorHiddenInGame(!bIsAcquired);

	MovementComponent->SetActive(bIsAcquired);
}

void AWpProjectile::OnComponentHit(UPrimitiveComponent* const HitComponent, AActor* const OtherActor, UPrimitiveComponent* const OtherComp, const FVector NormalImpulse, const FHitResult& Hit)
{
	if (IWpEntityInterface* const Entity = Cast<IWpEntityInterface>(OtherActor))
	{
		Weapon->OnProjectileHit(Entity->GetEntityComponent());
	}

	AWpActorPool::Release<AWpProjectile>(this);
}
