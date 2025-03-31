#include "WpEntity.h"

#include "Warpunk/Game/Belligerent/WpBelligerentComponent.h"
#include "Warpunk/Game/Endurance/WpEnduranceComponent.h"
#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Entity/WpPooledEntityData.h"
#include "Warpunk/Game/Intel/WpIntelComponent.h"
#include "Warpunk/Game/Movement/WpEntityMovementComponent.h"
#include "Warpunk/Game/Navigation/WpEntityAIController.h"
#include "Warpunk/Game/Order/WpOrderReceiverComponent.h"
#include "Warpunk/Game/Player/WpGamePlayerState.h"
#include "Warpunk/Game/Pool/WpPooledActorVisibilityManager.h"
#include "Warpunk/Game/Production/WpProductionComponent.h"
#include "Warpunk/Game/Weapon/WpWeaponComponent.h"
#include "Warpunk/Global/GameInstance/WpGameInstance.h"
#include "Warpunk/Global/LoadingScreen/WpLoadingScreen.h"

#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "NavAreas/NavArea_Obstacle.h"
#include "NavModifierComponent.h"
#include "Net/UnrealNetwork.h"

const FName AWpEntity::CollisionComponentName = TEXT("CollisionComponent");
const FName AWpEntity::MeshComponentName = TEXT("MeshComponent");
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
	Super::SetReplicateMovement(true);
	bAlwaysRelevant = true;
	SetNetDormancy(DORM_DormantPartial);
	NetUpdateFrequency = 60.0f;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AWpEntityAIController::StaticClass();

	CollisionComponent = CreateDefaultSubobject<UCapsuleComponent>(CollisionComponentName);
	CollisionComponent->SetCollisionProfileName(CollisionComponentCollisionProfileName);
	CollisionComponent->SetNotifyRigidBodyCollision(true);
	CollisionComponent->SetGenerateOverlapEvents(true);
	CollisionComponent->SetCanEverAffectNavigation(false);
	RootComponent = CollisionComponent;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(MeshComponentName);
	MeshComponent->SetCollisionProfileName(MeshComponentCollisionProfileName);
	MeshComponent->BodyInstance.SetInstanceNotifyRBCollision(true);
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->SetCanEverAffectNavigation(true);
	MeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules{EAttachmentRule::KeepRelative, false});

	NavModifierComponent = CreateDefaultSubobject<UNavModifierComponent>(NavModifierComponentName);
	NavModifierComponent->SetAreaClass(UNavArea_Obstacle::StaticClass());
	NavModifierComponent->SetNavigationRelevancy(false);

	MovementComponent = CreateDefaultSubobject<UWpEntityMovementComponent>(MovementComponentName);
	MovementComponent->SetUpdatedComponent(CollisionComponent);
	MovementComponent->SetInterpolatedComponent(MeshComponent);

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
	if (EntityType == EWpEntityType::Unit)
	{
		return;
	}

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

		MeshComponent->SetRelativeTransform(EntityConfig.MeshConfig.Transform);
		MeshComponent->SetStaticMesh(EntityConfig.MeshConfig.Mesh);
		MeshComponent->SetMaterial(0, EntityConfig.MeshConfig.Material);

		const FBoxSphereBounds MeshBounds = MeshComponent->GetStaticMesh()->GetBounds();

		const FVector MeshOrigin = MeshBounds.Origin * EntityConfig.MeshConfig.Transform.GetScale3D();
		MeshComponent->SetRelativeLocation(-MeshOrigin);

		const FVector MeshExtent = MeshBounds.BoxExtent * EntityConfig.MeshConfig.Transform.GetScale3D();
		const float Radius = FMath::Max(MeshExtent.X, MeshExtent.Y);
		const float HalfHeight = MeshExtent.Z;
		CollisionComponent->SetCapsuleSize(Radius, HalfHeight);

		MovementComponent->SetCollisionSize(Radius, HalfHeight * 2.0f);
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
	if (!HasAuthority() && !bIsVisible)
	{
		SetActorLocation(FVector::ZeroVector);
	}

	MeshComponent->SetHiddenInGame(!bIsVisible);
	MeshComponent->SetCollisionResponseToChannel(ECC_Camera, bIsVisible ? ECR_Block : ECR_Ignore);

	if (UWidgetComponent* const WidgetComponent = EnduranceComponent->GetEnduranceWidgetComponent())
	{
		WidgetComponent->SetHiddenInGame(!bIsVisible);
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

UCapsuleComponent* AWpEntity::GetCollisionComponent()
{
	return CollisionComponent;
}

const UCapsuleComponent* AWpEntity::GetCollisionComponent() const
{
	return CollisionComponent;
}

UStaticMeshComponent* AWpEntity::GetMeshComponent()
{
	return MeshComponent;
}

const UStaticMeshComponent* AWpEntity::GetMeshComponent() const
{
	return MeshComponent;
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
	const UDataTable* const EntityConfigs = GetGameInstance<UWpGameInstance>()->EntityConfigs;
	const FName RowName = FName{FString::FromInt(EntityConfigId)};
	const FWpEntityConfig& EntityConfig = *EntityConfigs->FindRow<FWpEntityConfig>(RowName, nullptr);

	FWpPooledEntityData PooledEntityData;
	PooledEntityData.EntityConfig = &EntityConfig;
	PooledEntityData.EntityConfigId = EntityConfigId;
	PooledEntityData.BelligerentId = 0;
	PooledEntityData.TeamId = 0;

	AcquirePooledObject(PooledEntityData);
}
