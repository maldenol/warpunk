#include "WpLocalEntity.h"

#include "Warpunk/Game/Belligerent/WpBelligerentComponent.h"
#include "Warpunk/Game/Intel/WpIntelComponent.h"
#include "Warpunk/Game/Player/WpGamePlayerState.h"
#include "Warpunk/Game/Pool/WpActorPool.h"

#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"

const float AWpLocalEntity::RememberIntelReleaseDelay = 0.5f;

AWpLocalEntity::AWpLocalEntity()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.SetTickFunctionEnable(false);

	bReplicates = false;

	AutoPossessAI = EAutoPossessAI::Disabled;

	CapsuleComponent->SetNotifyRigidBodyCollision(false);
	CapsuleComponent->SetGenerateOverlapEvents(false);

	BoxComponent->SetNotifyRigidBodyCollision(false);
	BoxComponent->SetGenerateOverlapEvents(false);

	StaticMeshComponent->SetCollisionProfileName(TEXT("NoCollision"));
	StaticMeshComponent->SetCanEverAffectNavigation(false);

	SkeletalMeshComponent->SetCollisionProfileName(TEXT("NoCollision"));
	SkeletalMeshComponent->SetCanEverAffectNavigation(false);
}

void AWpLocalEntity::BeginPlay()
{
	Super::BeginPlay();

	CapsuleComponent->OnComponentBeginOverlap.AddDynamic(this, &AWpLocalEntity::OnBeginOverlap);
	BoxComponent->OnComponentBeginOverlap.AddDynamic(this, &AWpLocalEntity::OnBeginOverlap);
}

void AWpLocalEntity::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TimeUntilReleased -= DeltaSeconds;
	if (TimeUntilReleased < 0.0f)
	{
		TimeUntilReleased = 0.0f;
		SetActorTickEnabled(false);

		AWpActorPool::Release<AWpLocalEntity>(this);
	}
}

void AWpLocalEntity::OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
{
	Super::OnPooledObjectAcquiredOrReleased(bIsAcquired, PooledObjectData);

	if (bIsAcquired)
	{
		Super::OnPooledObjectAcquiredOrReleased(false, PooledObjectData);

		SetActorHiddenInGame(false);
		SetPooledObjectVisibility(true);
	}
}

TArray<IWpPooledObjectInterface*> AWpLocalEntity::GetInnerPooledObjects()
{
	return {};
}

void AWpLocalEntity::RememberIntel()
{
	GetCollisionComponent()->SetGenerateOverlapEvents(true);
	SetActorEnableCollision(true);

	const FVector SmallDelta = FVector::UpVector / 1000.0f;
	SetActorLocation(GetActorLocation() + SmallDelta, false);
	SetActorLocation(GetActorLocation() - SmallDelta, false);
}

void AWpLocalEntity::OnBeginOverlap(UPrimitiveComponent* const OverlappedComponent, AActor* const OtherActor, UPrimitiveComponent* const OtherComp, const int32 OtherBodyIndex, const bool bFromSweep, const FHitResult& SweepResult)
{
	if ((OtherActor == GetOwner()) || !OtherActor->GetIsReplicated())
	{
		return;
	}

	if (!OtherComp->IsA<UWpIntelComponent>())
	{
		return;
	}

	if (AWpEntity* const OtherEntity = Cast<AWpEntity>(OtherActor))
	{
		if (const AWpGamePlayerState* const LocalPlayerState = GetWorld()->GetFirstPlayerController()->GetPlayerState<AWpGamePlayerState>())
		{
			if (OtherEntity->GetBelligerentComponent()->BelongsToSameTeam(LocalPlayerState->GetTeamId()))
			{
				GetCollisionComponent()->SetGenerateOverlapEvents(false);
				SetActorEnableCollision(false);

				TimeUntilReleased = RememberIntelReleaseDelay;
				SetActorTickEnabled(true);
			}
		}
	}
}
