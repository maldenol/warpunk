#include "WpUnit.h"

#include "Warpunk/Game/Data/WpEntityConfig.h"
#include "Warpunk/Game/Data/WpPooledEntityData.h"
#include "Warpunk/Game/EntityComponent/WpEntityComponent.h"
#include "Warpunk/Game/Navigation/WpUnitAIController.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

static const FName EntityComponentName = TEXT("EntityComponent");

static const FName CapsuleComponentCollisionProfileName = TEXT("WpEntitySimple");
static const FName MeshComponentCollisionProfileName = TEXT("WpEntityComplex");

AWpUnit::AWpUnit()
{
	PrimaryActorTick.bCanEverTick = false;

	AIControllerClass = AWpUnitAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	UCapsuleComponent* const CapsuleComp = GetCapsuleComponent();
	CapsuleComp->SetCollisionProfileName(CapsuleComponentCollisionProfileName);
	CapsuleComp->SetNotifyRigidBodyCollision(true);
	CapsuleComp->SetGenerateOverlapEvents(true);

	UMeshComponent* const MeshComponent = GetMesh();
	MeshComponent->SetCollisionProfileName(MeshComponentCollisionProfileName);
	MeshComponent->BodyInstance.SetInstanceNotifyRBCollision(true);
	MeshComponent->SetGenerateOverlapEvents(false);

	UCharacterMovementComponent* const MovementComp = Cast<UCharacterMovementComponent>(ACharacter::GetMovementComponent());
	MovementComp->bOrientRotationToMovement = true;
	bUseControllerRotationYaw = false;

	EntityComponent = CreateDefaultSubobject<UWpEntityComponent>(EntityComponentName);
}

UWpEntityComponent* AWpUnit::GetEntityComponent()
{
	return EntityComponent;
}

const UWpEntityComponent* AWpUnit::GetEntityComponent() const
{
	return EntityComponent;
}

void AWpUnit::OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const void* const CustomData)
{
	if (bIsAcquired)
	{
		const FWpPooledEntityData* const PooledEntityData = static_cast<const FWpPooledEntityData*>(CustomData);
		const FWpEntityConfig& EntityConfig = *PooledEntityData->EntityConfig;

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

		UCharacterMovementComponent* const MovementComponent = Cast<UCharacterMovementComponent>(GetMovementComponent());
		MovementComponent->MaxWalkSpeed = EntityConfig.UnitMovementConfig.Speed;
	}

	SetActorEnableCollision(bIsAcquired);
	SetActorHiddenInGame(!bIsAcquired);

	UCharacterMovementComponent* const MovementComp = Cast<UCharacterMovementComponent>(GetMovementComponent());
	MovementComp->SetActive(bIsAcquired);

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
		Cast<IWpPooledObjectInterface>(EntityComponent),
	};
}
