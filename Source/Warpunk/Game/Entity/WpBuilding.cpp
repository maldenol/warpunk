#include "WpBuilding.h"

#include "Warpunk/Game/Belligerent/WpBelligerentComponent.h"
#include "Warpunk/Game/Endurance/WpEnduranceComponent.h"
#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Entity/WpPooledEntityData.h"
#include "Warpunk/Game/Intel/WpIntelComponent.h"
#include "Warpunk/Game/Order/WpOrderReceiverComponent.h"
#include "Warpunk/Game/Production/WpProductionComponent.h"
#include "Warpunk/Game/Weapon/WpWeaponComponent.h"

#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"

const FName AWpBuilding::BoxComponentName = TEXT("BoxCollision");
const FName AWpBuilding::MeshComponentName = TEXT("SkeletalMesh");
const FName AWpBuilding::BelligerentComponentName = TEXT("BelligerentComponent");
const FName AWpBuilding::EnduranceComponentName = TEXT("EnduranceComponent");
const FName AWpBuilding::IntelComponentName = TEXT("IntelComponent");
const FName AWpBuilding::OrderReceiverComponentName = TEXT("OrderReceiverComponent");
const FName AWpBuilding::ProductionComponentName = TEXT("ProductionComponent");
const FName AWpBuilding::WeaponComponentName = TEXT("WeaponComponent");

const FName AWpBuilding::BoxComponentCollisionProfileName = TEXT("WpEntitySimple");
const FName AWpBuilding::MeshComponentCollisionProfileName = TEXT("WpEntityComplex");

AWpBuilding::AWpBuilding()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	Super::SetReplicateMovement(true);

	BoxComponent = CreateDefaultSubobject<UBoxComponent>(BoxComponentName);
	BoxComponent->SetCollisionProfileName(BoxComponentCollisionProfileName);
	BoxComponent->SetNotifyRigidBodyCollision(true);
	BoxComponent->SetGenerateOverlapEvents(true);
	RootComponent = BoxComponent;

	MeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(MeshComponentName);
	MeshComponent->SetCollisionProfileName(MeshComponentCollisionProfileName);
	MeshComponent->BodyInstance.SetInstanceNotifyRBCollision(true);
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->AttachToComponent(RootComponent, FAttachmentTransformRules{EAttachmentRule::KeepRelative, false});

	BelligerentComponent = CreateDefaultSubobject<UWpBelligerentComponent>(BelligerentComponentName);
	EnduranceComponent = CreateDefaultSubobject<UWpEnduranceComponent>(EnduranceComponentName);
	IntelComponent = CreateDefaultSubobject<UWpIntelComponent>(IntelComponentName);
	IntelComponent->AttachToComponent(RootComponent, FAttachmentTransformRules{EAttachmentRule::KeepRelative, false});
	OrderReceiverComponent = CreateDefaultSubobject<UWpOrderReceiverComponent>(OrderReceiverComponentName);
	ProductionComponent = CreateDefaultSubobject<UWpProductionComponent>(ProductionComponentName);
	WeaponComponent = CreateDefaultSubobject<UWpWeaponComponent>(WeaponComponentName);
}

void AWpBuilding::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWpBuilding, MeshComponentTransform);
	DOREPLIFETIME(AWpBuilding, MeshComponentSkeletalMesh);
	DOREPLIFETIME(AWpBuilding, MeshComponentMaterial);
	DOREPLIFETIME(AWpBuilding, MeshComponentAnimInstanceClass);
	DOREPLIFETIME(AWpBuilding, BoxComponentExtent);
}

UWpBelligerentComponent* AWpBuilding::GetBelligerentComponent()
{
	return BelligerentComponent;
}

const UWpBelligerentComponent* AWpBuilding::GetBelligerentComponent() const
{
	return BelligerentComponent;
}

UWpEnduranceComponent* AWpBuilding::GetEnduranceComponent()
{
	return EnduranceComponent;
}

const UWpEnduranceComponent* AWpBuilding::GetEnduranceComponent() const
{
	return EnduranceComponent;
}

UWpIntelComponent* AWpBuilding::GetIntelComponent()
{
	return IntelComponent;
}

const UWpIntelComponent* AWpBuilding::GetIntelComponent() const
{
	return IntelComponent;
}

UWpOrderReceiverComponent* AWpBuilding::GetOrderReceiverComponent()
{
	return OrderReceiverComponent;
}

const UWpOrderReceiverComponent* AWpBuilding::GetOrderReceiverComponent() const
{
	return OrderReceiverComponent;
}

UWpProductionComponent* AWpBuilding::GetProductionComponent()
{
	return ProductionComponent;
}

const UWpProductionComponent* AWpBuilding::GetProductionComponent() const
{
	return ProductionComponent;
}

UWpWeaponComponent* AWpBuilding::GetWeaponComponent()
{
	return WeaponComponent;
}

const UWpWeaponComponent* AWpBuilding::GetWeaponComponent() const
{
	return WeaponComponent;
}

uint32 AWpBuilding::GetConfigId() const
{
	return ConfigId;
}

void AWpBuilding::OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
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

		MeshComponent->SetRelativeTransform(EntityConfig.EntityMeshConfig.Transform);
		MeshComponent->SetSkeletalMesh(EntityConfig.EntityMeshConfig.Mesh);
		MeshComponent->SetMaterial(0, EntityConfig.EntityMeshConfig.Material);
		MeshComponent->SetAnimInstanceClass(EntityConfig.EntityMeshConfig.Animation);

		const FBoxSphereBounds MeshBounds = MeshComponent->GetLocalBounds();
		const FVector MeshOrigin = MeshBounds.Origin * EntityConfig.EntityMeshConfig.Transform.GetScale3D();
		MeshComponent->SetRelativeLocation(-MeshOrigin);
		const FVector MeshExtent = MeshBounds.BoxExtent * EntityConfig.EntityMeshConfig.Transform.GetScale3D();
		BoxComponent->SetBoxExtent(MeshExtent);

		MeshComponentTransform = MeshComponent->GetRelativeTransform();
		MeshComponentSkeletalMesh = MeshComponent->GetSkeletalMeshAsset();
		MeshComponentMaterial = MeshComponent->GetMaterial(0);
		MeshComponentAnimInstanceClass = MeshComponent->GetAnimClass();
		BoxComponentExtent = BoxComponent->GetUnscaledBoxExtent();
	}

	SetActorEnableCollision(bIsAcquired);
	SetActorHiddenInGame(!bIsAcquired);
}

TArray<IWpPooledObjectInterface*> AWpBuilding::GetInnerPooledObjects()
{
	return
	{
		Cast<IWpPooledObjectInterface>(BelligerentComponent),
		Cast<IWpPooledObjectInterface>(EnduranceComponent),
		Cast<IWpPooledObjectInterface>(IntelComponent),
		Cast<IWpPooledObjectInterface>(OrderReceiverComponent),
		Cast<IWpPooledObjectInterface>(ProductionComponent),
		Cast<IWpPooledObjectInterface>(WeaponComponent),
	};
}

void AWpBuilding::OnRep_MeshComponentTransform()
{
	MeshComponent->SetRelativeTransform(MeshComponentTransform);
}

void AWpBuilding::OnRep_MeshComponentSkeletalMesh()
{
	MeshComponent->SetSkeletalMesh(MeshComponentSkeletalMesh);
}

void AWpBuilding::OnRep_MeshComponentMaterial()
{
	MeshComponent->SetMaterial(0, MeshComponentMaterial);
}

void AWpBuilding::OnRep_MeshComponentAnimInstanceClass()
{
	MeshComponent->SetAnimInstanceClass(MeshComponentAnimInstanceClass);
}

void AWpBuilding::OnRep_BoxComponentExtent()
{
	BoxComponent->SetBoxExtent(BoxComponentExtent);
}
