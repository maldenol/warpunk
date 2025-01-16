#include "WpUnit.h"

#include "Warpunk/Game/Belligerent/WpBelligerentComponent.h"
#include "Warpunk/Game/Endurance/WpEnduranceComponent.h"
#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Entity/WpPooledEntityData.h"
#include "Warpunk/Game/Intel/WpIntelComponent.h"
#include "Warpunk/Game/Movement/WpUnitMovementComponent.h"
#include "Warpunk/Game/Navigation/WpUnitAIController.h"
#include "Warpunk/Game/Order/WpOrderReceiverComponent.h"
#include "Warpunk/Game/Player/WpGamePlayerState.h"
#include "Warpunk/Game/Pool/WpPooledActorVisibilityManager.h"
#include "Warpunk/Game/Production/WpProductionComponent.h"
#include "Warpunk/Game/Weapon/WpWeaponComponent.h"

#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"

const FName AWpUnit::BelligerentComponentName = TEXT("BelligerentComponent");
const FName AWpUnit::EnduranceComponentName = TEXT("EnduranceComponent");
const FName AWpUnit::IntelComponentName = TEXT("IntelComponent");
const FName AWpUnit::OrderReceiverComponentName = TEXT("OrderReceiverComponent");
const FName AWpUnit::ProductionComponentName = TEXT("ProductionComponent");
const FName AWpUnit::WeaponComponentName = TEXT("WeaponComponent");

const FName AWpUnit::CapsuleComponentCollisionProfileName = TEXT("WpEntitySimple");
const FName AWpUnit::MeshComponentCollisionProfileName = TEXT("WpEntityComplex");

AWpUnit::AWpUnit(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UWpUnitMovementComponent>(CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	Super::SetReplicateMovement(true);
	bAlwaysRelevant = true;
	SetNetDormancy(DORM_DormantPartial);

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AWpUnitAIController::StaticClass();

	UCapsuleComponent* const CapsuleComp = GetCapsuleComponent();
	CapsuleComp->SetCollisionProfileName(CapsuleComponentCollisionProfileName);
	CapsuleComp->SetNotifyRigidBodyCollision(true);
	CapsuleComp->SetGenerateOverlapEvents(true);

	UMeshComponent* const MeshComponent = GetMesh();
	MeshComponent->SetCollisionProfileName(MeshComponentCollisionProfileName);
	MeshComponent->BodyInstance.SetInstanceNotifyRBCollision(true);
	MeshComponent->SetGenerateOverlapEvents(false);

	BelligerentComponent = CreateDefaultSubobject<UWpBelligerentComponent>(BelligerentComponentName);
	EnduranceComponent = CreateDefaultSubobject<UWpEnduranceComponent>(EnduranceComponentName);
	IntelComponent = CreateDefaultSubobject<UWpIntelComponent>(IntelComponentName);
	OrderReceiverComponent = CreateDefaultSubobject<UWpOrderReceiverComponent>(OrderReceiverComponentName);
	ProductionComponent = CreateDefaultSubobject<UWpProductionComponent>(ProductionComponentName);
	WeaponComponent = CreateDefaultSubobject<UWpWeaponComponent>(WeaponComponentName);

	IntelComponent->AttachToComponent(RootComponent, FAttachmentTransformRules{EAttachmentRule::KeepRelative, false});
	WeaponComponent->AttachToComponent(RootComponent, FAttachmentTransformRules{EAttachmentRule::KeepRelative, false});
}

void AWpUnit::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWpUnit, MeshComponentTransform);
	DOREPLIFETIME(AWpUnit, MeshComponentSkeletalMesh);
	DOREPLIFETIME(AWpUnit, MeshComponentMaterial);
	DOREPLIFETIME(AWpUnit, MeshComponentAnimInstanceClass);
	DOREPLIFETIME(AWpUnit, CapsuleComponentSize);
}

bool AWpUnit::GetNetDormancy(const FVector& ViewPos, const FVector& ViewDir, AActor* const Viewer, AActor* const ViewTarget, UActorChannel* const InChannel, const float Time, const bool bLowBandwidth)
{
	const APlayerController* const ViewerPlayerController = Cast<APlayerController>(Viewer);
	const AWpGamePlayerState* const ViewerPlayerState = ViewerPlayerController->GetPlayerState<AWpGamePlayerState>();
	const uint32 ViewerTeamId = ViewerPlayerState->GetTeamId();
	return !AWpPooledActorVisibilityManager::IsVisibleForTeam(ViewerTeamId, this);
}

UWpBelligerentComponent* AWpUnit::GetBelligerentComponent()
{
	return BelligerentComponent;
}

const UWpBelligerentComponent* AWpUnit::GetBelligerentComponent() const
{
	return BelligerentComponent;
}

UWpEnduranceComponent* AWpUnit::GetEnduranceComponent()
{
	return EnduranceComponent;
}

const UWpEnduranceComponent* AWpUnit::GetEnduranceComponent() const
{
	return EnduranceComponent;
}

UWpIntelComponent* AWpUnit::GetIntelComponent()
{
	return IntelComponent;
}

const UWpIntelComponent* AWpUnit::GetIntelComponent() const
{
	return IntelComponent;
}

UWpOrderReceiverComponent* AWpUnit::GetOrderReceiverComponent()
{
	return OrderReceiverComponent;
}

const UWpOrderReceiverComponent* AWpUnit::GetOrderReceiverComponent() const
{
	return OrderReceiverComponent;
}

UWpProductionComponent* AWpUnit::GetProductionComponent()
{
	return ProductionComponent;
}

const UWpProductionComponent* AWpUnit::GetProductionComponent() const
{
	return ProductionComponent;
}

UWpWeaponComponent* AWpUnit::GetWeaponComponent()
{
	return WeaponComponent;
}

const UWpWeaponComponent* AWpUnit::GetWeaponComponent() const
{
	return WeaponComponent;
}

uint32 AWpUnit::GetConfigId() const
{
	return ConfigId;
}

void AWpUnit::OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
{
	if (bIsAcquired)
	{
		const FWpPooledEntityData& PooledEntityData = static_cast<const FWpPooledEntityData&>(PooledObjectData);
		const FWpEntityConfig& EntityConfig = *PooledEntityData.EntityConfig;

		ensureAlways(PooledEntityData.EntityConfigId > 0);

		ConfigId = PooledEntityData.EntityConfigId;

		ensureAlways(EntityConfig.EntityMeshConfig.Mesh);
		ensureAlways(EntityConfig.EntityMeshConfig.Material);
		ensureAlways(EntityConfig.EntityMeshConfig.Animation);

		USkeletalMeshComponent* const MeshComponent = GetMesh();
		MeshComponent->SetRelativeTransform(EntityConfig.EntityMeshConfig.Transform);
		MeshComponent->SetSkeletalMesh(EntityConfig.EntityMeshConfig.Mesh);
		MeshComponent->SetMaterial(0, EntityConfig.EntityMeshConfig.Material);
		MeshComponent->SetAnimInstanceClass(EntityConfig.EntityMeshConfig.Animation);

		const FBoxSphereBounds MeshBounds = MeshComponent->GetLocalBounds();
		const FVector MeshOrigin = MeshBounds.Origin * EntityConfig.EntityMeshConfig.Transform.GetScale3D();
		MeshComponent->SetRelativeLocation(-MeshOrigin);
		const FVector MeshExtent = MeshBounds.BoxExtent * EntityConfig.EntityMeshConfig.Transform.GetScale3D();
		const float Radius = FMath::Max(MeshExtent.X, MeshExtent.Y);
		const float HalfHeight = MeshExtent.Z;
		UCapsuleComponent* const CapsuleComp = GetCapsuleComponent();
		CapsuleComp->SetCapsuleSize(Radius, HalfHeight);

		CacheInitialMeshOffset(MeshComponent->GetRelativeLocation(), MeshComponent->GetRelativeRotation());

		MeshComponentTransform = MeshComponent->GetRelativeTransform();
		MeshComponentSkeletalMesh = MeshComponent->GetSkeletalMeshAsset();
		MeshComponentMaterial = MeshComponent->GetMaterial(0);
		MeshComponentAnimInstanceClass = MeshComponent->GetAnimClass();
		CapsuleComponentSize.X = CapsuleComp->GetUnscaledCapsuleRadius();
		CapsuleComponentSize.Y = CapsuleComp->GetUnscaledCapsuleHalfHeight();
	}

	SetActorEnableCollision(bIsAcquired);
	SetActorHiddenInGame(!bIsAcquired);

	if (AAIController* const AIController = GetController<AAIController>())
	{
		AIController->SetActorTickEnabled(bIsAcquired);
		AIController->SetActorHiddenInGame(!bIsAcquired);
	}
}

TArray<IWpPooledObjectInterface*> AWpUnit::GetInnerPooledObjects()
{
	return
	{
		Cast<IWpPooledObjectInterface>(BelligerentComponent),
		Cast<IWpPooledObjectInterface>(EnduranceComponent),
		Cast<IWpPooledObjectInterface>(IntelComponent),
		Cast<IWpPooledObjectInterface>(OrderReceiverComponent),
		Cast<IWpPooledObjectInterface>(ProductionComponent),
		Cast<IWpPooledObjectInterface>(WeaponComponent),
		Cast<IWpPooledObjectInterface>(GetMovementComponent()),
	};
}

void AWpUnit::OnPooledActorVisibilityChanged(const bool bIsVisible)
{
	const bool bIsHidden = !bIsVisible;

	UMeshComponent* const MeshComponent = GetMesh();
	if (MeshComponent->bHiddenInGame != bIsHidden)
	{
		MeshComponent->SetHiddenInGame(bIsHidden);
		MeshComponent->SetCollisionResponseToChannel(ECC_Camera, bIsHidden ? ECR_Ignore : ECR_Block);
	}

	if (UWidgetComponent* const WidgetComponent = EnduranceComponent->GetEnduranceWidgetComponent())
	{
		if (WidgetComponent->bHiddenInGame != bIsHidden)
		{
			WidgetComponent->SetHiddenInGame(bIsHidden);
		}
	}
}

void AWpUnit::OnRep_MeshComponentTransform()
{
	GetMesh()->SetRelativeTransform(MeshComponentTransform);

	CacheInitialMeshOffset(GetMesh()->GetRelativeLocation(), GetMesh()->GetRelativeRotation());
}

void AWpUnit::OnRep_MeshComponentSkeletalMesh()
{
	GetMesh()->SetSkeletalMesh(MeshComponentSkeletalMesh);
}

void AWpUnit::OnRep_MeshComponentMaterial()
{
	GetMesh()->SetMaterial(0, MeshComponentMaterial);
}

void AWpUnit::OnRep_MeshComponentAnimInstanceClass()
{
	GetMesh()->SetAnimInstanceClass(MeshComponentAnimInstanceClass);
}

void AWpUnit::OnRep_CapsuleComponentSize()
{
	GetCapsuleComponent()->SetCapsuleSize(CapsuleComponentSize.X, CapsuleComponentSize.Y);
}
