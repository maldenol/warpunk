#include "WpEntity.h"

#include "Warpunk/Game/Belligerent/WpBelligerentComponent.h"
#include "Warpunk/Game/Endurance/WpEnduranceComponent.h"
#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Entity/WpLocalEntity.h"
#include "Warpunk/Game/Entity/WpPooledEntityData.h"
#include "Warpunk/Game/Intel/WpIntelComponent.h"
#include "Warpunk/Game/Movement/WpEntityMovementComponent.h"
#include "Warpunk/Game/Navigation/WpEntityAIController.h"
#include "Warpunk/Game/Order/WpOrderReceiverComponent.h"
#include "Warpunk/Game/Player/WpGamePlayerState.h"
#include "Warpunk/Game/Pool/WpActorPool.h"
#include "Warpunk/Game/Pool/WpPooledActorVisibilityManager.h"
#include "Warpunk/Game/Production/WpProductionComponent.h"
#include "Warpunk/Game/Weapon/WpWeaponComponent.h"
#include "Warpunk/Global/GameInstance/WpGameInstance.h"
#include "Warpunk/Global/LoadingScreen/WpLoadingScreen.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "NavAreas/NavArea_Obstacle.h"
#include "NavModifierComponent.h"
#include "Net/UnrealNetwork.h"

const FName AWpEntity::RootComponentName = TEXT("RootComponent");
const FName AWpEntity::CapsuleComponentName = TEXT("CapsuleComponent");
const FName AWpEntity::BoxComponentName = TEXT("BoxComponent");
const FName AWpEntity::MeshBaseComponentName = TEXT("MeshBaseComponent");
const FName AWpEntity::StaticMeshComponentName = TEXT("StaticMeshComponent");
const FName AWpEntity::SkeletalMeshComponentName = TEXT("SkeletalMeshComponent");
const FName AWpEntity::NavModifierComponentName = TEXT("NavModifierComponent");
const FName AWpEntity::MovementComponentName = TEXT("MovementComponent");
const FName AWpEntity::BelligerentComponentName = TEXT("BelligerentComponent");
const FName AWpEntity::EnduranceComponentName = TEXT("EnduranceComponent");
const FName AWpEntity::IntelComponentName = TEXT("IntelComponent");
const FName AWpEntity::OrderReceiverComponentName = TEXT("OrderReceiverComponent");
const FName AWpEntity::ProductionComponentName = TEXT("ProductionComponent");
const FName AWpEntity::WeaponComponentName = TEXT("WeaponComponent");

const FName AWpEntity::CollisionComponentCollisionProfileName = TEXT("WpEntitySimple");
const FName AWpEntity::MeshComponentCollisionProfileName = TEXT("WpEntityComplex");

AWpEntity::AWpEntity()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	Super::SetReplicateMovement(false);
	bAlwaysRelevant = true;
	SetNetDormancy(DORM_DormantPartial);
	NetUpdateFrequency = 60.0f;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AWpEntityAIController::StaticClass();

	RootComponent = CreateDefaultSubobject<USceneComponent>(RootComponentName);
	RootComponent->SetIsReplicated(false);
	RootComponent->SetCanEverAffectNavigation(false);

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(CapsuleComponentName);
	CapsuleComponent->SetIsReplicated(false);
	CapsuleComponent->SetCollisionProfileName(CollisionComponentCollisionProfileName);
	CapsuleComponent->SetNotifyRigidBodyCollision(true);
	CapsuleComponent->SetGenerateOverlapEvents(true);
	CapsuleComponent->SetCanEverAffectNavigation(false);
	CapsuleComponent->AttachToComponent(RootComponent, FAttachmentTransformRules{EAttachmentRule::KeepRelative, false});

	BoxComponent = CreateDefaultSubobject<UBoxComponent>(BoxComponentName);
	BoxComponent->SetIsReplicated(false);
	BoxComponent->SetCollisionProfileName(CollisionComponentCollisionProfileName);
	BoxComponent->SetNotifyRigidBodyCollision(true);
	BoxComponent->SetGenerateOverlapEvents(true);
	BoxComponent->SetCanEverAffectNavigation(false);
	BoxComponent->AttachToComponent(RootComponent, FAttachmentTransformRules{EAttachmentRule::KeepRelative, false});

	MeshBaseComponent = CreateDefaultSubobject<USceneComponent>(MeshBaseComponentName);
	MeshBaseComponent->SetIsReplicated(false);
	MeshBaseComponent->SetCanEverAffectNavigation(false);
	MeshBaseComponent->AttachToComponent(RootComponent, FAttachmentTransformRules{EAttachmentRule::KeepRelative, false});

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(StaticMeshComponentName);
	StaticMeshComponent->SetIsReplicated(false);
	StaticMeshComponent->SetCollisionProfileName(MeshComponentCollisionProfileName);
	StaticMeshComponent->BodyInstance.SetInstanceNotifyRBCollision(true);
	StaticMeshComponent->SetGenerateOverlapEvents(false);
	StaticMeshComponent->SetCanEverAffectNavigation(true);
	StaticMeshComponent->AttachToComponent(MeshBaseComponent, FAttachmentTransformRules{EAttachmentRule::KeepRelative, false});

	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(SkeletalMeshComponentName);
	SkeletalMeshComponent->SetIsReplicated(false);
	SkeletalMeshComponent->SetCollisionProfileName(MeshComponentCollisionProfileName);
	SkeletalMeshComponent->BodyInstance.SetInstanceNotifyRBCollision(true);
	SkeletalMeshComponent->SetGenerateOverlapEvents(false);
	SkeletalMeshComponent->SetCanEverAffectNavigation(true);
	SkeletalMeshComponent->AttachToComponent(MeshBaseComponent, FAttachmentTransformRules{EAttachmentRule::KeepRelative, false});

	NavModifierComponent = CreateDefaultSubobject<UNavModifierComponent>(NavModifierComponentName);
	NavModifierComponent->SetIsReplicated(false);
	NavModifierComponent->SetAreaClass(UNavArea_Obstacle::StaticClass());
	NavModifierComponent->SetNavigationRelevancy(false);

	MovementComponent = CreateDefaultSubobject<UWpEntityMovementComponent>(MovementComponentName);
	MovementComponent->SetUpdatedComponent(RootComponent);
	MovementComponent->SetInterpolatedComponent(MeshBaseComponent);

	BelligerentComponent = CreateDefaultSubobject<UWpBelligerentComponent>(BelligerentComponentName);

	EnduranceComponent = CreateDefaultSubobject<UWpEnduranceComponent>(EnduranceComponentName);

	IntelComponent = CreateDefaultSubobject<UWpIntelComponent>(IntelComponentName);
	IntelComponent->SetCanEverAffectNavigation(false);
	IntelComponent->AttachToComponent(RootComponent, FAttachmentTransformRules{EAttachmentRule::KeepRelative, false});

	OrderReceiverComponent = CreateDefaultSubobject<UWpOrderReceiverComponent>(OrderReceiverComponentName);

	ProductionComponent = CreateDefaultSubobject<UWpProductionComponent>(ProductionComponentName);

	WeaponComponent = CreateDefaultSubobject<UWpWeaponComponent>(WeaponComponentName);
	WeaponComponent->SetCanEverAffectNavigation(false);
	WeaponComponent->AttachToComponent(RootComponent, FAttachmentTransformRules{EAttachmentRule::KeepRelative, false});

	SetPooledObjectVisibility(false);
}

void AWpEntity::BeginPlay()
{
	Super::BeginPlay();

	UWpLoadingScreen* const LoadingScreen = GetGameInstance<UWpGameInstance>()->GetLoadingScreen();
	if (!LoadingScreen->IsLoaded())
	{
		LoadingScreen->NotifyObjectLoaded();
	}
}

void AWpEntity::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWpEntity, EntityConfigId);
}

bool AWpEntity::GetNetDormancy(const FVector& ViewPos, const FVector& ViewDir, AActor* const Viewer, AActor* const ViewTarget, UActorChannel* const InChannel, const float Time, const bool bLowBandwidth)
{
	const APlayerController* const ViewerPlayerController = Cast<APlayerController>(Viewer);
	const AWpGamePlayerState* const ViewerPlayerState = ViewerPlayerController->GetPlayerState<AWpGamePlayerState>();
	const uint32 ViewerTeamId = ViewerPlayerState->GetTeamId();
	return !AWpPooledActorVisibilityManager::IsVisibleForTeam(ViewerTeamId, this);
}

void AWpEntity::PostNetReceiveLocationAndRotation()
{
	ensureAlways(EntityType != EWpEntityType::Unit);

	const FRepMovement& LocalRepMovement = GetReplicatedMovement();
	const FVector NewLocation = FRepMovement::RebaseOntoLocalOrigin(LocalRepMovement.Location, this);
	const FRotator& NewRotation = LocalRepMovement.Rotation;

	SetActorLocationAndRotation(NewLocation, NewRotation, false);
}

void AWpEntity::OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
{
	if (!bIsAcquired)
	{
		ensureAlways(HasAuthority());
	}

	if (bIsAcquired)
	{
		const FWpPooledEntityData& PooledEntityData = static_cast<const FWpPooledEntityData&>(PooledObjectData);

		ensureAlways(PooledEntityData.EntityConfigId > 0);

		EntityConfigId = PooledEntityData.EntityConfigId;

		const FWpEntityConfig& EntityConfig = *PooledEntityData.EntityConfig;

		EntityType = EntityConfig.Type;

		SetReplicateMovement(EntityType == EWpEntityType::Building);

		bUseCapsuleCollision = EntityConfig.MeshConfig.bUseCapsuleCollision;
		GetCollisionComponent()->SetRelativeRotation(EntityConfig.MeshConfig.RelativeTransform.Rotator(), false);

		CapsuleComponent->SetActive(bUseCapsuleCollision);
		CapsuleComponent->SetCollisionProfileName(bUseCapsuleCollision ? CollisionComponentCollisionProfileName : TEXT("NoCollision"));
		BoxComponent->SetActive(!bUseCapsuleCollision);
		BoxComponent->SetCollisionProfileName(!bUseCapsuleCollision ? CollisionComponentCollisionProfileName : TEXT("NoCollision"));

		if (bUseCapsuleCollision)
		{
			CapsuleComponent->SetCapsuleSize(
				EntityConfig.MeshConfig.CollisionShape.GetCapsuleRadius(),
				EntityConfig.MeshConfig.CollisionShape.GetCapsuleHalfHeight(),
				false);
		}
		else
		{
			BoxComponent->SetBoxExtent(
				EntityConfig.MeshConfig.CollisionShape.GetBox(),
				false);
		}

		bUseSkeletalMesh = EntityConfig.MeshConfig.bUseSkeletalMesh;
		MeshBaseComponent->SetRelativeTransform(EntityConfig.MeshConfig.RelativeTransform, false);

		StaticMeshComponent->SetActive(!bUseSkeletalMesh);
		StaticMeshComponent->SetCollisionProfileName(!bUseSkeletalMesh ? MeshComponentCollisionProfileName : TEXT("NoCollision"));
		StaticMeshComponent->SetCanEverAffectNavigation(!bUseSkeletalMesh);
		SkeletalMeshComponent->SetActive(bUseSkeletalMesh);
		SkeletalMeshComponent->SetCollisionProfileName(bUseSkeletalMesh ? MeshComponentCollisionProfileName : TEXT("NoCollision"));
		SkeletalMeshComponent->SetCanEverAffectNavigation(bUseSkeletalMesh);

		if (bUseSkeletalMesh)
		{
			SkeletalMeshComponent->SetSkeletalMeshAsset(EntityConfig.MeshConfig.SkeletalMesh);
			SkeletalMeshComponent->SetAnimInstanceClass(EntityConfig.MeshConfig.AnimInstanceClass);
		}
		else
		{
			StaticMeshComponent->SetStaticMesh(EntityConfig.MeshConfig.StaticMesh);
		}
		GetMeshComponent()->SetMaterial(0, EntityConfig.MeshConfig.Material);

		MovementComponent->SetCollisionComponent(GetCollisionComponent());
		MovementComponent->CacheInitialInterpolatedComponentOffset();
	}

	if (!HasAuthority())
	{
		return;
	}

	NavModifierComponent->SetNavigationRelevancy(bIsAcquired && (EntityType == EWpEntityType::Building));

	SetActorEnableCollision(bIsAcquired);
	SetActorHiddenInGame(!bIsAcquired);
}

void AWpEntity::OnPooledObjectVisibilityChanged(const bool bIsVisible)
{
	if (bIsVisible)
	{
		UMeshComponent* const MeshComponent = GetMeshComponent();
		MeshComponent->SetHiddenInGame(false);
		MeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
	}
	else
	{
		StaticMeshComponent->SetHiddenInGame(true);
		StaticMeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
		SkeletalMeshComponent->SetHiddenInGame(true);
		SkeletalMeshComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	}

	if (!bIsVisible && (EntityType == EWpEntityType::Building) && GetIsReplicated())
	{
		const FWpEntityConfig EntityConfig = GetGameInstance<UWpGameInstance>()->GetEntityConfig(EntityConfigId);

		FWpPooledEntityData PooledEntityData;
		PooledEntityData.EntityConfig = &EntityConfig;
		PooledEntityData.EntityConfigId = EntityConfigId;
		PooledEntityData.BelligerentId = GetBelligerentComponent()->GetBelligerentId();
		PooledEntityData.TeamId = GetBelligerentComponent()->GetTeamId();

		AWpLocalEntity* const LocalBuilding = AWpActorPool::Acquire<AWpLocalEntity>(GetWorld(), GetActorLocation(), GetActorRotation(), PooledEntityData);
		LocalBuilding->RememberIntel();
	}
}

TArray<IWpPooledObjectInterface*> AWpEntity::GetInnerPooledObjects()
{
	return
	{
		Cast<IWpPooledObjectInterface>(BelligerentComponent),
		Cast<IWpPooledObjectInterface>(EnduranceComponent),
		Cast<IWpPooledObjectInterface>(IntelComponent),
		Cast<IWpPooledObjectInterface>(MovementComponent),
		Cast<IWpPooledObjectInterface>(OrderReceiverComponent),
		Cast<IWpPooledObjectInterface>(ProductionComponent),
		Cast<IWpPooledObjectInterface>(WeaponComponent),
	};
}

UPrimitiveComponent* AWpEntity::GetCollisionComponent()
{
	return bUseCapsuleCollision ? Cast<UPrimitiveComponent>(CapsuleComponent) : Cast<UPrimitiveComponent>(BoxComponent);
}

const UPrimitiveComponent* AWpEntity::GetCollisionComponent() const
{
	return bUseCapsuleCollision ? Cast<UPrimitiveComponent>(CapsuleComponent) : Cast<UPrimitiveComponent>(BoxComponent);
}

USceneComponent* AWpEntity::GetMeshBaseComponent()
{
	return MeshBaseComponent;
}

const USceneComponent* AWpEntity::GetMeshBaseComponent() const
{
	return MeshBaseComponent;
}

UMeshComponent* AWpEntity::GetMeshComponent()
{
	return bUseSkeletalMesh ? Cast<UMeshComponent>(SkeletalMeshComponent) : Cast<UMeshComponent>(StaticMeshComponent);
}

const UMeshComponent* AWpEntity::GetMeshComponent() const
{
	return bUseSkeletalMesh ? Cast<UMeshComponent>(SkeletalMeshComponent) : Cast<UMeshComponent>(StaticMeshComponent);
}

UWpEntityMovementComponent* AWpEntity::GetEntityMovementComponent()
{
	return MovementComponent;
}

const UWpEntityMovementComponent* AWpEntity::GetEntityMovementComponent() const
{
	return MovementComponent;
}

UWpBelligerentComponent* AWpEntity::GetBelligerentComponent()
{
	return BelligerentComponent;
}

const UWpBelligerentComponent* AWpEntity::GetBelligerentComponent() const
{
	return BelligerentComponent;
}

UWpEnduranceComponent* AWpEntity::GetEnduranceComponent()
{
	return EnduranceComponent;
}

const UWpEnduranceComponent* AWpEntity::GetEnduranceComponent() const
{
	return EnduranceComponent;
}

UWpIntelComponent* AWpEntity::GetIntelComponent()
{
	return IntelComponent;
}

const UWpIntelComponent* AWpEntity::GetIntelComponent() const
{
	return IntelComponent;
}

UWpOrderReceiverComponent* AWpEntity::GetOrderReceiverComponent()
{
	return OrderReceiverComponent;
}

const UWpOrderReceiverComponent* AWpEntity::GetOrderReceiverComponent() const
{
	return OrderReceiverComponent;
}

UWpProductionComponent* AWpEntity::GetProductionComponent()
{
	return ProductionComponent;
}

const UWpProductionComponent* AWpEntity::GetProductionComponent() const
{
	return ProductionComponent;
}

UWpWeaponComponent* AWpEntity::GetWeaponComponent()
{
	return WeaponComponent;
}

const UWpWeaponComponent* AWpEntity::GetWeaponComponent() const
{
	return WeaponComponent;
}

uint32 AWpEntity::GetEntityConfigId() const
{
	return EntityConfigId;
}

EWpEntityType AWpEntity::GetEntityType() const
{
	return EntityType;
}

void AWpEntity::OnRep_EntityConfigId()
{
	const FWpEntityConfig EntityConfig = GetGameInstance<UWpGameInstance>()->GetEntityConfig(EntityConfigId);

	FWpPooledEntityData PooledEntityData;
	PooledEntityData.EntityConfig = &EntityConfig;
	PooledEntityData.EntityConfigId = EntityConfigId;
	PooledEntityData.BelligerentId = 0;
	PooledEntityData.TeamId = 0;

	AcquirePooledObject(PooledEntityData);
}
