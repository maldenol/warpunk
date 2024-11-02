#include "WpBuilding.h"

#include "Warpunk/Game/Data/WpEntityConfig.h"
#include "Warpunk/Game/Data/WpPooledEntityData.h"
#include "Warpunk/Game/EntityComponent/WpEntityComponent.h"

#include "Components/BoxComponent.h"

const FName AWpBuilding::BoxComponentName = TEXT("BoxCollision");
const FName AWpBuilding::MeshComponentName = TEXT("SkeletalMesh");
const FName AWpBuilding::EntityComponentName = TEXT("EntityComponent");

const FName AWpBuilding::BoxComponentCollisionProfileName = TEXT("WpEntitySimple");
const FName AWpBuilding::MeshComponentCollisionProfileName = TEXT("WpEntityComplex");

AWpBuilding::AWpBuilding()
{
	PrimaryActorTick.bCanEverTick = false;

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

	EntityComponent = CreateDefaultSubobject<UWpEntityComponent>(EntityComponentName);
}

UWpEntityComponent* AWpBuilding::GetEntityComponent()
{
	return EntityComponent;
}

const UWpEntityComponent* AWpBuilding::GetEntityComponent() const
{
	return EntityComponent;
}

void AWpBuilding::OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const void* const CustomData)
{
	if (bIsAcquired)
	{
		const FWpPooledEntityData* const PooledEntityData = static_cast<const FWpPooledEntityData*>(CustomData);
		const FWpEntityConfig& EntityConfig = *PooledEntityData->EntityConfig;

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
	}

	SetActorEnableCollision(bIsAcquired);
	SetActorHiddenInGame(!bIsAcquired);
}

TArray<IWpPooledObjectInterface*> AWpBuilding::GetInnerPooledObjects()
{
	return
	{
		Cast<IWpPooledObjectInterface>(EntityComponent),
	};
}
