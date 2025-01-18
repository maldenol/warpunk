#include "WpProjectile.h"

#include "Warpunk/Game/Entity/WpEntityInterface.h"
#include "Warpunk/Game/Player/WpGamePlayerState.h"
#include "Warpunk/Game/Pool/WpActorPool.h"
#include "Warpunk/Game/Pool/WpPooledActorVisibilityManager.h"
#include "Warpunk/Game/Weapon/WpPooledProjectileData.h"
#include "Warpunk/Game/Weapon/WpProjectileWeapon.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"

const FName AWpProjectile::SphereComponentName = TEXT("SphereCollision");
const FName AWpProjectile::MeshComponentName = TEXT("StaticMesh");
const FName AWpProjectile::MovementComponentName = TEXT("ProjectileMovement");

const FName AWpProjectile::SphereComponentCollisionProfileName = TEXT("WpProjectile");
const FName AWpProjectile::MeshComponentCollisionProfileName = TEXT("NoCollision");

AWpProjectile::AWpProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	Super::SetReplicateMovement(true);
	bAlwaysRelevant = true;
	SetNetDormancy(DORM_DormantPartial);

	SphereComponent = CreateDefaultSubobject<USphereComponent>(SphereComponentName);
	SphereComponent->SetCollisionProfileName(SphereComponentCollisionProfileName);
	SphereComponent->SetNotifyRigidBodyCollision(true);
	SphereComponent->SetGenerateOverlapEvents(true);
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

	if (HasAuthority())
	{
		SphereComponent->OnComponentHit.AddDynamic(this, &AWpProjectile::OnComponentHit);
	}
}

void AWpProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWpProjectile, SphereComponentRadius);
	DOREPLIFETIME(AWpProjectile, MeshComponentStaticMesh);
	DOREPLIFETIME(AWpProjectile, MeshComponentScale);
}

bool AWpProjectile::GetNetDormancy(const FVector& ViewPos, const FVector& ViewDir, AActor* const Viewer, AActor* const ViewTarget, UActorChannel* const InChannel, const float Time, const bool bLowBandwidth)
{
	const APlayerController* const ViewerPlayerController = Cast<APlayerController>(Viewer);
	const AWpGamePlayerState* const ViewerPlayerState = ViewerPlayerController->GetPlayerState<AWpGamePlayerState>();
	const uint32 ViewerTeamId = ViewerPlayerState->GetTeamId();
	return !AWpPooledActorVisibilityManager::IsVisibleForTeam(ViewerTeamId, this);
}

void AWpProjectile::OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
{
	ensureAlways(HasAuthority());

	if (bIsAcquired)
	{
		const FWpPooledProjectileData& PooledProjectileData = static_cast<const FWpPooledProjectileData&>(PooledObjectData);

		ensureAlways(PooledProjectileData.Radius >= 1.0f);
		ensureAlways(PooledProjectileData.Weapon);
		ensureAlways(PooledProjectileData.Mesh);
		ensureAlways(!PooledProjectileData.InitialVelocity.IsNearlyZero());

		const float Radius = PooledProjectileData.Radius;

		Weapon = PooledProjectileData.Weapon;
		SphereComponent->SetSphereRadius(Radius);
		MeshComponent->SetStaticMesh(PooledProjectileData.Mesh);
		MovementComponent->Velocity = PooledProjectileData.InitialVelocity;

		const FVector MeshExtent = MeshComponent->GetStaticMesh()->GetBounds().BoxExtent;
		const float ScaleX = Radius / MeshExtent.X;
		const float ScaleY = Radius / MeshExtent.Y;
		const float ScaleZ = Radius / MeshExtent.Z;
		MeshComponent->SetRelativeScale3D(FVector{ScaleX, ScaleY, ScaleZ});

		SphereComponentRadius = SphereComponent->GetUnscaledSphereRadius();
		MeshComponentStaticMesh = MeshComponent->GetStaticMesh();
		MeshComponentScale = MeshComponent->GetRelativeScale3D();
	}

	SetActorEnableCollision(bIsAcquired);
	SetActorHiddenInGame(!bIsAcquired);

	MovementComponent->SetActive(bIsAcquired);
	MovementComponent->SetComponentTickEnabled(bIsAcquired);
}

void AWpProjectile::OnPooledActorVisibilityChanged(const bool bIsVisible)
{
	const bool bIsHidden = !bIsVisible;

	if (MeshComponent->bHiddenInGame != bIsHidden)
	{
		MeshComponent->SetHiddenInGame(bIsHidden);
	}
}

void AWpProjectile::OnComponentHit(UPrimitiveComponent* const HitComponent, AActor* const OtherActor, UPrimitiveComponent* const OtherComp, const FVector NormalImpulse, const FHitResult& Hit)
{
	ensureAlways(HasAuthority());

	if (IWpEntityInterface* const Entity = Cast<IWpEntityInterface>(OtherActor))
	{
		Weapon->OnProjectileHit(Entity);
	}

	AWpActorPool::Release<AWpProjectile>(this);
}

void AWpProjectile::OnRep_SphereComponentRadius()
{
	SphereComponent->SetSphereRadius(SphereComponentRadius);
}

void AWpProjectile::OnRep_MeshComponentStaticMesh()
{
	MeshComponent->SetStaticMesh(MeshComponentStaticMesh);
}

void AWpProjectile::OnRep_MeshComponentScale()
{
	MeshComponent->SetRelativeScale3D(MeshComponentScale);
}
