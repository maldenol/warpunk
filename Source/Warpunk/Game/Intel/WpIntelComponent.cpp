#include "WpIntelComponent.h"

#include "Warpunk/Game/Belligerent/WpBelligerentComponent.h"
#include "Warpunk/Game/Endurance/WpEnduranceComponent.h"
#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Entity/WpEntityInterface.h"
#include "Warpunk/Game/Entity/WpPooledEntityData.h"
#include "Warpunk/Game/Intel/WpFogOfWarManager.h"
#include "Warpunk/Game/Player/WpGamePlayerController.h"
#include "Warpunk/Game/Player/WpGamePlayerState.h"
#include "Warpunk/Game/Pool/WpPooledActorInterface.h"
#include "Warpunk/Game/Pool/WpPooledActorVisibilityManager.h"

#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

const FName UWpIntelComponent::MeshCollisionProfileName = TEXT("WpIntel");

UWpIntelComponent::UWpIntelComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);

	UWpIntelComponent::SetCollisionProfileName(MeshCollisionProfileName);
	UWpIntelComponent::SetNotifyRigidBodyCollision(false);
	SetGenerateOverlapEvents(false);

	SetRenderCustomDepth(true);
	SetCustomDepthStencilValue(1);
	SetVisibleInSceneCaptureOnly(true);
}

void UWpIntelComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner()->HasAuthority())
	{
		OnComponentBeginOverlap.AddDynamic(this, &UWpIntelComponent::OnBeginOverlap);
		OnComponentEndOverlap.AddDynamic(this, &UWpIntelComponent::OnEndOverlap);
	}

	if (!GetOwner()->HasAuthority())
	{
		InitializeMesh();
	}

	UWpEnduranceComponent* const EnduranceComponent = GetOwner<IWpEntityInterface>()->GetEnduranceComponent();
	EnduranceComponent->OnEntityStateChanged().AddUObject(this, &UWpIntelComponent::OnEntityStateChanged);
	OnEntityStateChanged(EnduranceComponent->GetEntityState());
}

void UWpIntelComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UWpIntelComponent, bIsMeshVisible);
	DOREPLIFETIME(UWpIntelComponent, MeshTransform);
}

void UWpIntelComponent::OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
{
	ensureAlways(GetOwner()->HasAuthority());

	if (bIsAcquired)
	{
		const FWpPooledEntityData& PooledEntityData = static_cast<const FWpPooledEntityData&>(PooledObjectData);
		const FWpIntelConfig* const IntelConfig = PooledEntityData.EntityConfig->IntelConfig.GetPtrOrNull();

		if (IntelConfig)
		{
			InitializeMesh();

			const float Range = IntelConfig->Range;

			static constexpr float HalfSizeZ = 2500.0f;
			const FVector MeshExtent = GetStaticMesh()->GetBounds().BoxExtent;
			const float ScaleX = Range / MeshExtent.X;
			const float ScaleY = Range / MeshExtent.Y;
			const float ScaleZ = HalfSizeZ / MeshExtent.Z;

			SetRelativeLocation(FVector{0.0f, 0.0f, -HalfSizeZ});
			SetRelativeScale3D(FVector{ScaleX, ScaleY, ScaleZ});

			MeshTransform = GetRelativeTransform();
		}

		bIsAcquired = IntelConfig != nullptr;
	}

	SetMeshVisibility(bIsAcquired);

	SetActive(bIsAcquired);
}

const TArray<IWpEntityInterface*>& UWpIntelComponent::GetVisibleEntities() const
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	return VisibleEntities;
}

void UWpIntelComponent::OnBeginOverlap(UPrimitiveComponent* const OverlappedComponent, AActor* const OtherActor, UPrimitiveComponent* const OtherComp, const int32 OtherBodyIndex, const bool bFromSweep, const FHitResult& SweepResult)
{
	ensureAlways(GetOwner()->HasAuthority());

	if (OtherActor == GetOwner())
	{
		return;
	}

	if (OtherActor->GetClass()->ImplementsInterface(UWpPooledActorInterface::StaticClass()))
	{
		const uint32 EntityTeamId = Cast<IWpEntityInterface>(GetOwner())->GetBelligerentComponent()->GetTeamId();
		AWpPooledActorVisibilityManager::UpdateActorVisibility(EntityTeamId, OtherActor, true);
	}

	if (IWpEntityInterface* const OtherEntity = Cast<IWpEntityInterface>(OtherActor))
	{
		ensureAlways(!VisibleEntities.Contains(OtherEntity));

		VisibleEntities.Add(OtherEntity);
	}
}

void UWpIntelComponent::OnEndOverlap(UPrimitiveComponent* const OverlappedComponent, AActor* const OtherActor, UPrimitiveComponent* const OtherComp, const int32 OtherBodyIndex)
{
	ensureAlways(GetOwner()->HasAuthority());

	if (OtherActor == GetOwner())
	{
		return;
	}

	if (OtherActor->GetClass()->ImplementsInterface(UWpPooledActorInterface::StaticClass()))
	{
		const uint32 EntityTeamId = Cast<IWpEntityInterface>(GetOwner())->GetBelligerentComponent()->GetTeamId();
		AWpPooledActorVisibilityManager::UpdateActorVisibility(EntityTeamId, OtherActor, false);
	}

	if (IWpEntityInterface* const OtherEntity = Cast<IWpEntityInterface>(OtherActor))
	{
		ensureAlways(VisibleEntities.Contains(OtherEntity));

		VisibleEntities.RemoveSingle(OtherEntity);
	}
}

void UWpIntelComponent::OnRep_bIsMeshVisible()
{
	UpdateMeshVisibilityBasedOnTeam();
}

void UWpIntelComponent::OnRep_MeshTransform()
{
	SetRelativeTransform(MeshTransform);
}

void UWpIntelComponent::InitializeMesh()
{
	const AWpFogOfWarManager* const FogOfWarManager = Cast<AWpFogOfWarManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AWpFogOfWarManager::StaticClass()));
	UStaticMesh* const IntelMesh = FogOfWarManager->GetIntelMesh();
	UMaterialInterface* const IntelMaterial = FogOfWarManager->GetIntelMaterial();

	ensureAlways(IntelMesh);
	ensureAlways(IntelMaterial);

	SetStaticMesh(IntelMesh);
	SetMaterial(0, IntelMaterial);
}

void UWpIntelComponent::OnEntityStateChanged(const EWpEntityState EntityState)
{
	const bool bHasAuthorityAndIsOperational = GetOwner()->HasAuthority() && (EntityState == EWpEntityState::Operational);
	SetGenerateOverlapEvents(bHasAuthorityAndIsOperational);
	if (bHasAuthorityAndIsOperational)
	{
		const FVector SmallDelta = FVector::UpVector / 1000.0f;
		GetOwner()->SetActorLocation(GetOwner()->GetActorLocation() + SmallDelta);
		GetOwner()->SetActorLocation(GetOwner()->GetActorLocation() - SmallDelta);
	}

	UpdateMeshVisibilityBasedOnTeam();
}

void UWpIntelComponent::SetMeshVisibility(const bool bInIsMeshVisible)
{
	ensureAlways(GetOwner()->HasAuthority());

	bIsMeshVisible = bInIsMeshVisible;
	OnRep_bIsMeshVisible();
}

void UWpIntelComponent::UpdateMeshVisibilityBasedOnTeam()
{
	bool bIsVisibleIntelVolumeOnClient = bIsMeshVisible;

	if (bIsVisibleIntelVolumeOnClient)
	{
		const ULocalPlayer* const LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
		AWpGamePlayerController* const LocalPlayerController = Cast<AWpGamePlayerController>(LocalPlayer->GetPlayerController(GetWorld()));
		if (const AWpGamePlayerState* const LocalPlayerState = LocalPlayerController->GetPlayerState<AWpGamePlayerState>())
		{
			const uint32 EntityTeamId = GetOwner<IWpEntityInterface>()->GetBelligerentComponent()->GetTeamId();
			const uint32 LocalPlayerTeamId = LocalPlayerState->GetTeamId();
			const bool bBelongsToLocalPlayerTeam = (EntityTeamId == LocalPlayerTeamId);
			bIsVisibleIntelVolumeOnClient &= bBelongsToLocalPlayerTeam;

			const UWpEnduranceComponent* const EnduranceComponent = GetOwner<IWpEntityInterface>()->GetEnduranceComponent();
			const bool bEntityIsOperational = (EnduranceComponent->GetEntityState() == EWpEntityState::Operational);
			bIsVisibleIntelVolumeOnClient &= bEntityIsOperational;
		}
		else
		{
			if (!OnPlayerStateReplicatedDelegateHandle.IsValid())
			{
				OnPlayerStateReplicatedDelegateHandle = LocalPlayerController->OnPlayerStateReplicated().AddUObject(this, &UWpIntelComponent::OnPlayerStateReplicated);
			}

			bIsVisibleIntelVolumeOnClient = false;
		}
	}

	SetHiddenInSceneCapture(!bIsVisibleIntelVolumeOnClient);
}

void UWpIntelComponent::OnPlayerStateReplicated()
{
	ensureAlways(IsActive());

	const ULocalPlayer* const LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	AWpGamePlayerController* const LocalPlayerController = Cast<AWpGamePlayerController>(LocalPlayer->GetPlayerController(GetWorld()));
	LocalPlayerController->OnPlayerStateReplicated().Remove(OnPlayerStateReplicatedDelegateHandle);
	OnPlayerStateReplicatedDelegateHandle.Reset();

	if (bIsMeshVisible)
	{
		UpdateMeshVisibilityBasedOnTeam();
	}
}
