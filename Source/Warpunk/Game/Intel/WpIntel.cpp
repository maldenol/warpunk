#include "WpIntel.h"

#include "Warpunk/Game/Data/WpEntityConfig.h"
#include "Warpunk/Game/Data/WpPooledEntityData.h"
#include "Warpunk/Game/EntityComponent/WpEntityComponent.h"
#include "Warpunk/Game/Intel/WpFogOfWarManager.h"

#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

const FName FWpIntel::IntelVolumeName = TEXT("IntelVolume");

const FName FWpIntel::IntelVolumeCollisionProfileName = TEXT("WpIntel");

void FWpIntel::OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const void* const CustomData)
{
	if (bIsAcquired)
	{
		const FWpPooledEntityData* const PooledEntityData = static_cast<const FWpPooledEntityData*>(CustomData);
		const FWpIntelConfig* const IntelConfig = PooledEntityData->EntityConfig->IntelConfig.GetPtrOrNull();

		if (IntelConfig)
		{
			const float Range = IntelConfig->Range;

			static constexpr float HalfSizeZ = 2500.0f;
			const FVector MeshExtent = IntelVolume->GetStaticMesh()->GetBounds().BoxExtent;
			const float ScaleX = Range / MeshExtent.X;
			const float ScaleY = Range / MeshExtent.Y;
			const float ScaleZ = HalfSizeZ / MeshExtent.Z;

			IntelVolume->SetRelativeLocation(FVector{0.0f, 0.0f, -HalfSizeZ});
			IntelVolume->SetRelativeScale3D(FVector{ScaleX, ScaleY, ScaleZ});
		}

		bIsAcquired = IntelConfig != nullptr;
	}

	IntelVolume->SetGenerateOverlapEvents(bIsAcquired);
	IntelVolume->SetHiddenInGame(!bIsAcquired);
}

void FWpIntel::Initialize(UWpEntityComponent* const InOwnerEntityComponent)
{
	UWpEntityComponent* const OwnerEntityComponent = InOwnerEntityComponent;

	IntelVolume = NewObject<UStaticMeshComponent>(OwnerEntityComponent->GetOwner(), UStaticMeshComponent::StaticClass(), IntelVolumeName);
	IntelVolume->RegisterComponent();
	IntelVolume->AttachToComponent(OwnerEntityComponent->GetOwner()->GetRootComponent(), FAttachmentTransformRules{EAttachmentRule::KeepRelative, false});

	const AWpFogOfWarManager* const FogOfWarManager = Cast<AWpFogOfWarManager>(UGameplayStatics::GetActorOfClass(OwnerEntityComponent->GetWorld(), AWpFogOfWarManager::StaticClass()));
	UStaticMesh* const IntelMesh = FogOfWarManager->GetIntelMesh();
	UMaterialInterface* const IntelMaterial = FogOfWarManager->GetIntelMaterial();

	ensureAlways(IntelMesh);
	ensureAlways(IntelMaterial);

	IntelVolume->SetStaticMesh(IntelMesh);
	IntelVolume->SetRelativeLocation(FVector::ZeroVector);
	IntelVolume->SetRelativeScale3D(FVector::ZeroVector);

	IntelVolume->SetCollisionProfileName(IntelVolumeCollisionProfileName);
	IntelVolume->SetNotifyRigidBodyCollision(false);
	IntelVolume->OnComponentBeginOverlap.AddDynamic(OwnerEntityComponent, &UWpEntityComponent::OnIntelVolumeBeginOverlap);
	IntelVolume->OnComponentEndOverlap.AddDynamic(OwnerEntityComponent, &UWpEntityComponent::OnIntelVolumeEndOverlap);

	IntelVolume->SetMaterial(0, IntelMaterial);
	IntelVolume->SetRenderCustomDepth(true);
	IntelVolume->SetCustomDepthStencilValue(1);
	IntelVolume->SetVisibleInSceneCaptureOnly(true);
}

void FWpIntel::AddIntel(UWpEntityComponent* const EntityComponent)
{
	Intel.Add(EntityComponent);
}

void FWpIntel::RemoveIntel(UWpEntityComponent* const EntityComponent)
{
	Intel.RemoveSingle(EntityComponent);
}

const TArray<UWpEntityComponent*>& FWpIntel::GetIntel() const
{
	const bool bIsActive = !IntelVolume->bHiddenInGame;
	if (!bIsActive)
	{
		const bool bNoIntel = Intel.IsEmpty();
		ensureAlways(bNoIntel);
	}

	return Intel;
}
