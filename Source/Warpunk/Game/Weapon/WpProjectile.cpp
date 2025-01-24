#include "WpProjectile.h"

#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Entity/WpEntityInterface.h"
#include "Warpunk/Game/GameMode/WpGameGameState.h"
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
	NetUpdateFrequency = 60.0f;

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
	MovementComponent->SetInterpolatedComponent(MeshComponent);

	OnPooledActorVisibilityChanged(false);
}

void AWpProjectile::BeginPlay()
{
	Super::BeginPlay();

	const bool bHasAuthority = HasAuthority();

	if (bHasAuthority)
	{
		SphereComponent->OnComponentHit.AddDynamic(this, &AWpProjectile::OnComponentHit);
	}

	MovementComponent->bSimulationEnabled = bHasAuthority;
	MovementComponent->bInterpMovement = !bHasAuthority;
	MovementComponent->bInterpRotation = !bHasAuthority;
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

void AWpProjectile::PostNetReceiveLocationAndRotation()
{
	const FRepMovement& LocalRepMovement = GetReplicatedMovement();
	const FVector NewLocation = FRepMovement::RebaseOntoLocalOrigin(LocalRepMovement.Location, this);
	const FRotator& NewRotation = LocalRepMovement.Rotation;

	MovementComponent->MoveInterpolationTarget(NewLocation, NewRotation);
}

void AWpProjectile::OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
{
	ensureAlways(HasAuthority());

	if (bIsAcquired)
	{
		const FWpPooledProjectileData& PooledProjectileData = static_cast<const FWpPooledProjectileData&>(PooledObjectData);

		ensureAlways(PooledProjectileData.ProjectileConfig.EntityConfigId > 0);
		ensureAlways(!PooledProjectileData.InitialVelocity.IsNearlyZero());
		ensureAlways(PooledProjectileData.Weapon);

		ProjectileConfig = PooledProjectileData.ProjectileConfig;
		OnRep_ProjectileConfig();

		MovementComponent->Velocity = PooledProjectileData.InitialVelocity;
		Weapon = PooledProjectileData.Weapon;
	}

	SetActorEnableCollision(bIsAcquired);
	SetActorHiddenInGame(!bIsAcquired);

	MovementComponent->SetActive(bIsAcquired);
	MovementComponent->SetComponentTickEnabled(bIsAcquired);
}

void AWpProjectile::OnPooledActorVisibilityChanged(const bool bIsVisible)
{
	MeshComponent->SetHiddenInGame(!bIsVisible);

	if (!HasAuthority())
	{
		MovementComponent->SetComponentTickEnabled(bIsVisible);
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

void AWpProjectile::OnRep_ProjectileConfig()
{
	const UDataTable* const EntityConfigs = GetWorld()->GetGameState<AWpGameGameState>()->GetEntityConfigs();
	const FName RowName = FName{FString::FromInt(ProjectileConfig.EntityConfigId)};
	const FWpEntityConfig& EntityConfig = *EntityConfigs->FindRow<FWpEntityConfig>(RowName, nullptr);
	const FWpWeaponConfig& WeaponConfig = EntityConfig.WeaponConfigs[ProjectileConfig.WeaponConfigIndex];

	ensureAlways(WeaponConfig.ProjectileRadius >= 1.0f);
	ensureAlways(WeaponConfig.ProjectileMesh);

	const float Radius = WeaponConfig.ProjectileRadius;

	SphereComponent->SetSphereRadius(Radius);
	MeshComponent->SetStaticMesh(WeaponConfig.ProjectileMesh);

	const FVector MeshExtent = MeshComponent->GetStaticMesh()->GetBounds().BoxExtent;
	const float ScaleX = Radius / MeshExtent.X;
	const float ScaleY = Radius / MeshExtent.Y;
	const float ScaleZ = Radius / MeshExtent.Z;
	MeshComponent->SetRelativeScale3D(FVector{ScaleX, ScaleY, ScaleZ});
}
