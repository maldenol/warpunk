#include "WpProjectile.h"

#include "Warpunk/Game/Entity/WpEntity.h"
#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Movement/WpProjectileMovementComponent.h"
#include "Warpunk/Game/Player/WpGamePlayerState.h"
#include "Warpunk/Game/Pool/WpActorPool.h"
#include "Warpunk/Game/Pool/WpPooledActorVisibilityManager.h"
#include "Warpunk/Game/Weapon/WpPooledProjectileData.h"
#include "Warpunk/Game/Weapon/WpProjectileWeapon.h"
#include "Warpunk/Global/GameInstance/WpGameInstance.h"
#include "Warpunk/Global/LoadingScreen/WpLoadingScreen.h"

#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

const FName AWpProjectile::CollisionComponentName = TEXT("CollisionComponent");
const FName AWpProjectile::MeshComponentName = TEXT("MeshComponent");
const FName AWpProjectile::MovementComponentName = TEXT("MovementComponent");

const FName AWpProjectile::CollisionComponentCollisionProfileName = TEXT("WpProjectile");
const FName AWpProjectile::MeshComponentCollisionProfileName = TEXT("NoCollision");

AWpProjectile::AWpProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	Super::SetReplicateMovement(false);
	bAlwaysRelevant = true;
	SetNetDormancy(DORM_DormantPartial);
	NetUpdateFrequency = 60.0f;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(CollisionComponentName);
	CollisionComponent->SetIsReplicated(false);
	CollisionComponent->SetCollisionProfileName(CollisionComponentCollisionProfileName);
	CollisionComponent->SetNotifyRigidBodyCollision(true);
	CollisionComponent->SetGenerateOverlapEvents(true);
	RootComponent = CollisionComponent;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(MeshComponentName);
	MeshComponent->SetIsReplicated(false);
	MeshComponent->SetCollisionProfileName(MeshComponentCollisionProfileName);
	MeshComponent->SetNotifyRigidBodyCollision(false);
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules{EAttachmentRule::KeepRelative, false});

	MovementComponent = CreateDefaultSubobject<UWpProjectileMovementComponent>(MovementComponentName);
	MovementComponent->SetUpdatedComponent(CollisionComponent);

	SetPooledObjectVisibility(false);
}

void AWpProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		CollisionComponent->OnComponentHit.AddDynamic(this, &AWpProjectile::OnComponentHit);
	}

	UWpLoadingScreen* const LoadingScreen = GetGameInstance<UWpGameInstance>()->GetLoadingScreen();
	if (!LoadingScreen->IsLoaded())
	{
		LoadingScreen->NotifyObjectLoaded();
	}
}

void AWpProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWpProjectile, ProjectileConfig);
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
	if (!bIsAcquired)
	{
		ensureAlways(HasAuthority());
	}

	if (bIsAcquired)
	{
		const FWpPooledProjectileData& PooledProjectileData = static_cast<const FWpPooledProjectileData&>(PooledObjectData);

		ensureAlways(PooledProjectileData.ProjectileConfig.EntityConfigId > 0);

		ProjectileConfig = PooledProjectileData.ProjectileConfig;

		const FWpEntityConfig EntityConfig = GetGameInstance<UWpGameInstance>()->GetEntityConfig(ProjectileConfig.EntityConfigId);
		const FWpWeaponConfig& WeaponConfig = EntityConfig.WeaponConfigs[ProjectileConfig.WeaponConfigIndex];

		CollisionComponent->SetSphereRadius(WeaponConfig.ProjectileRadius);

		MeshComponent->SetRelativeTransform(WeaponConfig.ProjectileMeshRelativeTransform, false);
		MeshComponent->SetStaticMesh(WeaponConfig.ProjectileMesh);

		if (HasAuthority())
		{
			ensureAlways(!FMath::IsNearlyZero(PooledProjectileData.Damage));
			ensureAlways(PooledProjectileData.Weapon);

			Damage = PooledProjectileData.Damage;
			Weapon = PooledProjectileData.Weapon;
		}
	}

	if (!HasAuthority())
	{
		return;
	}

	SetActorEnableCollision(bIsAcquired);
	SetActorHiddenInGame(!bIsAcquired);
}

void AWpProjectile::OnPooledObjectVisibilityChanged(const bool bIsVisible)
{
	MeshComponent->SetHiddenInGame(!bIsVisible);
}

TArray<IWpPooledObjectInterface*> AWpProjectile::GetInnerPooledObjects()
{
	return
	{
		Cast<IWpPooledObjectInterface>(MovementComponent),
	};
}

void AWpProjectile::OnComponentHit(UPrimitiveComponent* const HitComponent, AActor* const OtherActor, UPrimitiveComponent* const OtherComp, const FVector NormalImpulse, const FHitResult& Hit)
{
	ensureAlways(HasAuthority());

	if (AWpEntity* const Entity = Cast<AWpEntity>(OtherActor))
	{
		if (Entity->GetIsReplicated())
		{
			UGameplayStatics::ApplyPointDamage(Entity, Damage, FVector::ZeroVector, {}, nullptr, nullptr, nullptr);

			Weapon->OnProjectileHit(Entity);
		}
	}

	AWpActorPool::Release<AWpProjectile>(this);
}

void AWpProjectile::OnRep_ProjectileConfig()
{
	FWpPooledProjectileData PooledProjectileData;
	PooledProjectileData.ProjectileConfig = ProjectileConfig;
	PooledProjectileData.InitialVelocity = FVector::ZeroVector;
	PooledProjectileData.Damage = 0.0f;
	PooledProjectileData.Weapon = nullptr;

	AcquirePooledObject(PooledProjectileData);
}
