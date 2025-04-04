#include "WpIntelComponent.h"

#include "Warpunk/Game/Belligerent/WpBelligerentComponent.h"
#include "Warpunk/Game/Endurance/WpEnduranceComponent.h"
#include "Warpunk/Game/Entity/WpEntity.h"
#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Entity/WpPooledEntityData.h"
#include "Warpunk/Game/Intel/WpFogOfWarManager.h"
#include "Warpunk/Game/Player/WpGamePlayerController.h"
#include "Warpunk/Game/Player/WpGamePlayerState.h"
#include "Warpunk/Game/Pool/WpPooledActorVisibilityManager.h"
#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"
#include "Warpunk/Global/GameInstance/WpGameInstance.h"
#include "Warpunk/Global/LoadingScreen/WpLoadingScreen.h"

#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

const FName UWpIntelComponent::MeshCollisionProfileName = TEXT("WpIntel");

UWpIntelComponent::UWpIntelComponent()
{
	bAutoActivate = false;
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

	UWpEnduranceComponent* const EnduranceComponent = GetOwner<AWpEntity>()->GetEnduranceComponent();
	EnduranceComponent->OnEntityStateChanged().AddUObject(this, &UWpIntelComponent::OnEntityStateChanged);
	OnEntityStateChanged(GetOwner<AWpEntity>(), EnduranceComponent->GetEntityState());

	UWpLoadingScreen* const LoadingScreen = GetWorld()->GetGameInstance<UWpGameInstance>()->GetLoadingScreen();
	LoadingScreen->OnLoaded().AddUObject(this, &UWpIntelComponent::OnLoaded);
	if (LoadingScreen->IsLoaded())
	{
		OnLoaded();
	}
}

void UWpIntelComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UWpLoadingScreen* const LoadingScreen = GetWorld()->GetGameInstance<UWpGameInstance>()->GetLoadingScreen();
	LoadingScreen->OnLoaded().RemoveAll(this);

	Super::EndPlay(EndPlayReason);
}

void UWpIntelComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DISABLE_REPLICATED_PRIVATE_PROPERTY(UActorComponent, bIsActive);
	DISABLE_REPLICATED_PRIVATE_PROPERTY(UStaticMeshComponent, StaticMesh);

	DOREPLIFETIME(UWpIntelComponent, bMeshIsVisible);
}

void UWpIntelComponent::OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
{
	if (!bIsAcquired)
	{
		ensureAlways(GetOwner()->HasAuthority());
	}

	if (bIsAcquired)
	{
		const FWpPooledEntityData& PooledEntityData = static_cast<const FWpPooledEntityData&>(PooledObjectData);
		const FWpIntelConfig* const IntelConfig = PooledEntityData.EntityConfig->IntelConfig.GetPtrOrNull();

		if (IntelConfig)
		{
			if (!GetStaticMesh())
			{
				const AWpFogOfWarManager* const FogOfWarManager = AWpFogOfWarManager::Instance(this);
				SetStaticMesh(FogOfWarManager->GetIntelMesh());
				SetMaterial(0, FogOfWarManager->GetIntelMaterial());
			}

			const float Range = IntelConfig->Range;

			static constexpr float HalfSizeZ = 2500.0f;
			const FVector MeshExtent = GetStaticMesh()->GetBounds().BoxExtent;
			const float ScaleX = Range / MeshExtent.X;
			const float ScaleY = Range / MeshExtent.Y;
			const float ScaleZ = HalfSizeZ / MeshExtent.Z;

			SetRelativeLocation(FVector{0.0f, 0.0f, -HalfSizeZ});
			SetRelativeScale3D(FVector{ScaleX, ScaleY, ScaleZ});
		}

		bIsAcquired = IntelConfig != nullptr;
	}

	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	SetMeshIsVisibile(bIsAcquired);

	SetActive(bIsAcquired);
}

void UWpIntelComponent::OnPooledObjectVisibilityChanged(const bool bIsVisible)
{
	if (!GetOwner()->HasAuthority())
	{
		SetActive(bIsVisible);
	}
}

const TSet<AWpEntity*>& UWpIntelComponent::GetVisibleEntities() const
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	return VisibleEntities;
}

void UWpIntelComponent::OnBeginOverlap(UPrimitiveComponent* const OverlappedComponent, AActor* const OtherActor, UPrimitiveComponent* const OtherComp, const int32 OtherBodyIndex, const bool bFromSweep, const FHitResult& SweepResult)
{
	ensureAlways(GetOwner()->HasAuthority());

	if ((OtherActor == GetOwner()) || !OtherActor->GetIsReplicated())
	{
		return;
	}

	if (OtherActor->Implements<UWpPooledObjectInterface>())
	{
		const uint32 EntityTeamId = GetOwner<AWpEntity>()->GetBelligerentComponent()->GetTeamId();
		AWpPooledActorVisibilityManager::UpdateActorVisibility(EntityTeamId, OtherActor, true);
	}

	if (AWpEntity* const OtherEntity = Cast<AWpEntity>(OtherActor))
	{
		ensureAlways(!VisibleEntities.Contains(OtherEntity));

		VisibleEntities.Add(OtherEntity);
	}
}

void UWpIntelComponent::OnEndOverlap(UPrimitiveComponent* const OverlappedComponent, AActor* const OtherActor, UPrimitiveComponent* const OtherComp, const int32 OtherBodyIndex)
{
	ensureAlways(GetOwner()->HasAuthority());

	if ((OtherActor == GetOwner()) || !OtherActor->GetIsReplicated())
	{
		return;
	}

	if (OtherActor->Implements<UWpPooledObjectInterface>())
	{
		const uint32 EntityTeamId = GetOwner<AWpEntity>()->GetBelligerentComponent()->GetTeamId();
		AWpPooledActorVisibilityManager::UpdateActorVisibility(EntityTeamId, OtherActor, false);
	}

	if (AWpEntity* const OtherEntity = Cast<AWpEntity>(OtherActor))
	{
		ensureAlways(VisibleEntities.Contains(OtherEntity));

		VisibleEntities.Remove(OtherEntity);
	}
}

void UWpIntelComponent::OnRep_bMeshIsVisible()
{
	UpdateMeshVisibilityBasedOnTeam();
}

void UWpIntelComponent::OnEntityStateChanged(AWpEntity* const Entity, const EWpEntityState EntityState)
{
	const bool bEnableOverlap = EntityState == EWpEntityState::Operational;
	SetGenerateOverlapEvents(bEnableOverlap);
	if (bEnableOverlap)
	{
		const FVector SmallDelta = FVector::UpVector / 1000.0f;
		GetOwner()->SetActorLocation(GetOwner()->GetActorLocation() + SmallDelta, false);
		GetOwner()->SetActorLocation(GetOwner()->GetActorLocation() - SmallDelta, false);
	}

	UpdateMeshVisibilityBasedOnTeam();
}

void UWpIntelComponent::OnLoaded()
{
	bCanCheckTeam = true;

	GetWorld()->GetGameInstance<UWpGameInstance>()->GetLoadingScreen()->OnLoaded().RemoveAll(this);

	UpdateMeshVisibilityBasedOnTeam();
}

void UWpIntelComponent::SetMeshIsVisibile(const bool bInMeshIsVisible)
{
	ensureAlways(GetOwner()->HasAuthority());

	bMeshIsVisible = bInMeshIsVisible;
	OnRep_bMeshIsVisible();
}

void UWpIntelComponent::UpdateMeshVisibilityBasedOnTeam()
{
	bool bIsVisibleIntelVolumeOnClient = bMeshIsVisible;

	if (bIsVisibleIntelVolumeOnClient)
	{
		if (bCanCheckTeam)
		{
			const ULocalPlayer* const LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
			const AWpGamePlayerController* const LocalPlayerController = Cast<AWpGamePlayerController>(LocalPlayer->GetPlayerController(GetWorld()));
			const AWpGamePlayerState* const LocalPlayerState = LocalPlayerController->GetPlayerState<AWpGamePlayerState>();
			const uint32 EntityTeamId = GetOwner<AWpEntity>()->GetBelligerentComponent()->GetTeamId();
			const uint32 LocalPlayerTeamId = LocalPlayerState->GetTeamId();
			const bool bBelongsToLocalPlayerTeam = EntityTeamId == LocalPlayerTeamId;
			bIsVisibleIntelVolumeOnClient &= bBelongsToLocalPlayerTeam;

			const UWpEnduranceComponent* const EnduranceComponent = GetOwner<AWpEntity>()->GetEnduranceComponent();
			const bool bEntityIsOperational = EnduranceComponent->GetEntityState() == EWpEntityState::Operational;
			bIsVisibleIntelVolumeOnClient &= bEntityIsOperational;
		}
		else
		{
			bIsVisibleIntelVolumeOnClient = false;
		}
	}

	SetHiddenInSceneCapture(!bIsVisibleIntelVolumeOnClient);
}
