#include "WpProductionComponent.h"

#include "Warpunk/Game/Belligerent/WpBelligerentComponent.h"
#include "Warpunk/Game/Endurance/WpEnduranceComponent.h"
#include "Warpunk/Game/Entity/WpEntity.h"
#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Entity/WpPooledEntityData.h"
#include "Warpunk/Game/Intel/WpIntelComponent.h"
#include "Warpunk/Game/Order/WpOrderReceiverComponent.h"
#include "Warpunk/Game/Pool/WpActorPool.h"
#include "Warpunk/Global/GameInstance/WpGameInstance.h"

#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "Net/UnrealNetwork.h"

UWpProductionComponent::UWpProductionComponent()
{
	bAutoActivate = false;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetIsReplicatedByDefault(false);
}

void UWpProductionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner()->HasAuthority())
	{
		AIController = GetOwner<AWpEntity>()->GetController<AAIController>();

		UWpOrderReceiverComponent* const OrderReceiverComponent = GetOwner<AWpEntity>()->GetOrderReceiverComponent();
		OrderReceiverComponent->OnOrderReceived().AddUObject(this, &UWpProductionComponent::OnOrderReceived);
	}
}

void UWpProductionComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* const ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	if (GetOwner<AWpEntity>()->GetEnduranceComponent()->GetEntityState() != EWpEntityState::Operational)
	{
		return;
	}

	if (!AIController)
	{
		switch (ProductionState)
		{
		case EWpProductionState::InitiatingProduction:
			Produce();
			break;
		case EWpProductionState::ProducingBuilding:
		case EWpProductionState::ProducingUnit:
		case EWpProductionState::Repairing:
			Repair(DeltaTime);
			break;
		case EWpProductionState::Reclaiming:
			Reclaim(DeltaTime);
			break;
		default:
			break;
		}
	}
	else if (AIController->GetMoveStatus() == EPathFollowingStatus::Type::Idle)
	{
		switch (ProductionState)
		{
		case EWpProductionState::InitiatingProduction:
			Produce();
			break;
		case EWpProductionState::ProducingBuilding:
		case EWpProductionState::ProducingUnit:
		case EWpProductionState::Repairing:
			Repair(DeltaTime);
			break;
		case EWpProductionState::Reclaiming:
			Reclaim(DeltaTime);
			break;
		case EWpProductionState::Idle:
			if (bShouldSupport)
			{
				UpdateCurrentTargetEntity();
			}
			break;
		default:
			break;
		}
	}
}

void UWpProductionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DISABLE_REPLICATED_PRIVATE_PROPERTY(UActorComponent, bIsActive);
}

void UWpProductionComponent::OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
{
	if (!bIsAcquired)
	{
		ensureAlways(GetOwner()->HasAuthority());
	}

	if (bIsAcquired)
	{
		const FWpPooledEntityData& PooledEntityData = static_cast<const FWpPooledEntityData&>(PooledObjectData);
		const FWpProductionConfig* const ProductionConfig = PooledEntityData.EntityConfig->ProductionConfig.GetPtrOrNull();

		if (ProductionConfig)
		{
			Range = ProductionConfig->Range;
			MatterConsumption = ProductionConfig->MatterConsumption;
			EnergyPerMatterRatio = ProductionConfig->EnergyPerMatterRatio;
			bCanProduce = ProductionConfig->bCanProduce;
			bShouldSupport = ProductionConfig->bIsSupport;

			if (GetOwner()->HasAuthority())
			{
				ProductionState = EWpProductionState::Idle;
				EntityToProduceConfigId = 0;
				EntityToProduceLocation = FVector::ZeroVector;
				bInfiniteProductionEnabled = false;
			}
		}

		bIsAcquired = ProductionConfig != nullptr;
	}

	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	if (!bIsAcquired)
	{
		ProxyOrderQueue.Empty(ProxyOrderQueue.Num());

		if (IsActive())
		{
			Stop();
		}
	}

	SetActive(bIsAcquired);
}

void UWpProductionComponent::StartProduce(const uint32 EntityConfigId, const FVector& Location)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	Stop();

	ProductionState = EWpProductionState::InitiatingProduction;
	EntityToProduceConfigId = EntityConfigId;
	EntityToProduceLocation = Location;
}

void UWpProductionComponent::StartRepair(AWpEntity* const Entity)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	Stop();

	ProductionState = EWpProductionState::Repairing;
	SetCurrentTargetEntity(Entity);
}

void UWpProductionComponent::StartReclaim(AWpEntity* const Entity)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	Stop();

	ProductionState = EWpProductionState::Reclaiming;
	SetCurrentTargetEntity(Entity);
}

void UWpProductionComponent::Stop()
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	ProductionState = EWpProductionState::Idle;
	if (CurrentTargetEntity == PriorityTargetEntity)
	{
		PriorityTargetEntity = nullptr;
	}
	SetCurrentTargetEntity(nullptr);
}

bool UWpProductionComponent::CanReach(const FVector& Location) const
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	const FVector OwnerLocation = GetOwner()->GetActorLocation();
	const float Distance2 = FVector::DistSquared2D(OwnerLocation, Location);
	return Distance2 <= FMath::Square(Range);
}

bool UWpProductionComponent::CanReach(const AWpEntity* const Entity) const
{
	if (!Entity)
	{
		return false;
	}

	const FVector Location = Entity->GetActorLocation();
	return CanReach(Location);
}

bool UWpProductionComponent::CanRepair(const AWpEntity* const Entity) const
{
	ensureAlways(IsActive());

	return Entity->GetEnduranceComponent()->CanBeRepaired(bCanProduce);
}

bool UWpProductionComponent::CanReachAndRepair(const AWpEntity* const Entity) const
{
	return CanReach(Entity) && CanRepair(Entity);
}

void UWpProductionComponent::ReceiveProxyOrder(const FWpOrder& Order, const bool bOverride)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	if (bOverride)
	{
		ProxyOrderQueue.Empty(ProxyOrderQueue.Num());
	}

	ProxyOrderQueue.Add(Order);
}

float UWpProductionComponent::GetRange() const
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	return Range;
}

void UWpProductionComponent::SetShouldSupport(const bool bInShouldSupport)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	bShouldSupport = bInShouldSupport;
}

bool UWpProductionComponent::ShouldSupport() const
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	return bShouldSupport;
}

AWpEntity* UWpProductionComponent::GetCurrentTargetEntity() const
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	return CurrentTargetEntity;
}

void UWpProductionComponent::SetPriorityTargetEntity(AWpEntity* const InPriorityTargetEntity)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	if (InPriorityTargetEntity)
	{
		const UWpBelligerentComponent* const BelligerentComponent = GetOwner<AWpEntity>()->GetBelligerentComponent();
		const bool bPriorityTargetBelongsToSameTeam = BelligerentComponent->BelongsToSameTeam(InPriorityTargetEntity->GetBelligerentComponent());
		if (!bPriorityTargetBelongsToSameTeam)
		{
			return;
		}
	}

	PriorityTargetEntity = InPriorityTargetEntity;

	if (!bShouldSupport)
	{
		StartRepair(PriorityTargetEntity);
	}
}

void UWpProductionComponent::EnableInfiniteProduction(const bool bInInfiniteProductionEnabled)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	bInfiniteProductionEnabled = bInInfiniteProductionEnabled;
}

bool UWpProductionComponent::IsInfiniteProductionEnabled() const
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	return bInfiniteProductionEnabled;
}

UWpProductionComponent::FRepairFinishedEvent& UWpProductionComponent::OnRepairFinished()
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	return RepairFinishedEvent;
}

UWpProductionComponent::FReclaimFinishedEvent& UWpProductionComponent::OnReclaimFinished()
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	return ReclaimFinishedEvent;
}

void UWpProductionComponent::Produce()
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	const FWpEntityConfig EntityConfig = GetOwner()->GetGameInstance<UWpGameInstance>()->GetEntityConfig(EntityToProduceConfigId);

	if (!ensureAlways(EntityConfig.Type != EWpEntityType::Undefined))
	{
		return;
	}
	const bool bIsBuilding = EntityConfig.Type == EWpEntityType::Building;

	const FBoxSphereBounds EntityMeshBounds = EntityConfig.MeshConfig.Mesh->GetBounds();
	const FVector EntityMeshExtent = EntityMeshBounds.BoxExtent * EntityConfig.MeshConfig.Transform.GetScale3D();
	const FVector SpawnLocation = EntityToProduceLocation + EntityMeshExtent * 0.5f * FVector::UpVector + 1.0f * FVector::UpVector;

	TArray<AActor*> Actors;
	const TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes =
	{
		UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel1),
		UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel5),
	};
	UKismetSystemLibrary::BoxOverlapActors(GetWorld(), SpawnLocation, EntityMeshExtent, ObjectTypes, AActor::StaticClass(), {}, Actors);
	UKismetSystemLibrary::DrawDebugBox(GetWorld(), SpawnLocation, EntityMeshExtent, FColor::Red);
	const bool bHasSpaceToProduce = Actors.IsEmpty();

	AWpEntity* Entity;

	if (bHasSpaceToProduce)
	{
		const UWpBelligerentComponent* const BelligerentComponent = GetOwner<AWpEntity>()->GetBelligerentComponent();

		FWpPooledEntityData PooledEntityData;
		PooledEntityData.EntityConfig = &EntityConfig;
		PooledEntityData.EntityConfigId = EntityToProduceConfigId;
		PooledEntityData.BelligerentId = BelligerentComponent->GetBelligerentId();
		PooledEntityData.TeamId = BelligerentComponent->GetTeamId();

		Entity = AWpActorPool::Acquire<AWpEntity>(SpawnLocation, FRotator::ZeroRotator, PooledEntityData);
		if (!Entity)
		{
			return;
		}
	}
	else if (bIsBuilding)
	{
		FHitResult Hit;
		const FVector SmallDelta = FVector::UpVector;
		const FVector TraceStart = SpawnLocation - SmallDelta;
		const FVector TraceEnd = SpawnLocation + SmallDelta;
		GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Camera);
		Entity = Cast<AWpEntity>(Hit.GetActor());
		if (!Entity)
		{
			return;
		}

		const bool bHasSameEntityConfigId = Entity->GetEntityConfigId() == EntityToProduceConfigId;
		const bool bBelongsToSameBelligerent = GetOwner<AWpEntity>()->GetBelligerentComponent()->BelongsToSameBelligerent(Entity->GetBelligerentComponent());
		if (!bHasSameEntityConfigId || !bBelongsToSameBelligerent)
		{
			return;
		}
	}
	else
	{
		return;
	}

	ProductionState = bIsBuilding ? EWpProductionState::ProducingBuilding : EWpProductionState::ProducingUnit;
	SetCurrentTargetEntity(Entity);
}

void UWpProductionComponent::Repair(const float DeltaTime)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	if (!CurrentTargetEntity)
	{
		Stop();
		return;
	}

	if (!CanRepair(CurrentTargetEntity))
	{
		if (ProductionState == EWpProductionState::ProducingUnit)
		{
			for (const FWpOrder& UnitOrder : ProxyOrderQueue)
			{
				CurrentTargetEntity->GetOrderReceiverComponent()->ReceiveOrder(UnitOrder, false);
			}
		}

		AWpEntity* const LastTargetEntity = CurrentTargetEntity;

		Stop();

		RepairFinishedEvent.Broadcast(LastTargetEntity);

		return;
	}

	if (AIController && AIController->GetMoveStatus() != EPathFollowingStatus::Type::Idle)
	{
		return;
	}

	if (CanReach(CurrentTargetEntity))
	{
		CurrentTargetEntity->GetEnduranceComponent()->ReceiveRepair(DeltaTime * MatterConsumption);
	}
}

void UWpProductionComponent::Reclaim(const float DeltaTime)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	if (!CurrentTargetEntity)
	{
		Stop();
		return;
	}

	if (!CanRepair(CurrentTargetEntity))
	{
		Stop();

		ReclaimFinishedEvent.Broadcast();

		return;
	}

	CurrentTargetEntity->GetEnduranceComponent()->ReceiveReclaim(DeltaTime * MatterConsumption);
}

void UWpProductionComponent::SetCurrentTargetEntity(AWpEntity* const NewTargetEntity)
{
	if (NewTargetEntity)
	{
		ensureAlways(IsActive());
	}
	ensureAlways(GetOwner()->HasAuthority());

	if (CurrentTargetEntity)
	{
		UWpEnduranceComponent* const CurrentTargetEnduranceComponent = CurrentTargetEntity->GetEnduranceComponent();
		CurrentTargetEnduranceComponent->OnEntityStateChanged().RemoveAll(this);
	}

	if (NewTargetEntity)
	{
		UWpEnduranceComponent* const NewTargetEnduranceComponent = NewTargetEntity->GetEnduranceComponent();
		NewTargetEnduranceComponent->OnEntityStateChanged().AddUObject(this, &UWpProductionComponent::OnCurrentTargetEntityStateChanged);
	}

	CurrentTargetEntity = NewTargetEntity;
}

void UWpProductionComponent::UpdateCurrentTargetEntity()
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime >= LastUpdateCurrentTargetEntityTime + UpdateCurrentTargetEntityCooldown)
	{
		LastUpdateCurrentTargetEntityTime = CurrentTime;
	}
	else
	{
		return;
	}

	if (CurrentTargetEntity)
	{
		const bool bCanReachAndRepairCurrentTarget = CanReachAndRepair(CurrentTargetEntity);
		if (!bCanReachAndRepairCurrentTarget)
		{
			SetCurrentTargetEntity(nullptr);
		}
	}

	const bool bCurrentTargetIsPriorityTarget = CurrentTargetEntity && (CurrentTargetEntity == PriorityTargetEntity);
	if (bCurrentTargetIsPriorityTarget)
	{
		return;
	}

	const UWpIntelComponent* const IntelComponent = GetOwner<AWpEntity>()->GetIntelComponent();
	const TSet<AWpEntity*>& VisibleEntities = IntelComponent->GetVisibleEntities();

	if (PriorityTargetEntity && VisibleEntities.Contains(PriorityTargetEntity))
	{
		const bool bCanReachAndRepairPriorityTarget = CanReachAndRepair(PriorityTargetEntity);
		if (bCanReachAndRepairPriorityTarget)
		{
			StartRepair(PriorityTargetEntity);
			return;
		}
	}

	if (CurrentTargetEntity)
	{
		return;
	}

	for (AWpEntity* const CandidateEntity : VisibleEntities)
	{
		const bool bCanReachAndRepairCandidate = CanReachAndRepair(CandidateEntity);
		if (!bCanReachAndRepairCandidate)
		{
			continue;
		}

		const bool bCandidateBelongsToSameTeam = GetOwner<AWpEntity>()->GetBelligerentComponent()->BelongsToSameTeam(CandidateEntity->GetBelligerentComponent());
		if (!bCandidateBelongsToSameTeam)
		{
			continue;
		}

		StartRepair(CandidateEntity);
		break;
	}
}

void UWpProductionComponent::OnCurrentTargetEntityStateChanged(AWpEntity* const Entity, const EWpEntityState EntityState)
{
	if (EntityState == EWpEntityState::Wreck)
	{
		AWpEntity* const LastTargetEntity = CurrentTargetEntity;

		Stop();

		RepairFinishedEvent.Broadcast(LastTargetEntity);
	}
}

void UWpProductionComponent::OnOrderReceived(const FWpOrder& InOrder)
{
	if (!IsActive())
	{
		return;
	}

	if (InOrder.Type == EWpOrderType::Stop)
	{
		AWpEntity* const LastTargetEntity = CurrentTargetEntity;

		Stop();

		RepairFinishedEvent.Broadcast(LastTargetEntity);

		PriorityTargetEntity = nullptr;
	}
}
