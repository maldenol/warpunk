#include "WpProductionComponent.h"

#include "Warpunk/Game/Belligerent/WpBelligerentComponent.h"
#include "Warpunk/Game/Endurance/WpEnduranceComponent.h"
#include "Warpunk/Game/Entity/WpBuilding.h"
#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Entity/WpEntityInterface.h"
#include "Warpunk/Game/Entity/WpPooledEntityData.h"
#include "Warpunk/Game/Entity/WpUnit.h"
#include "Warpunk/Game/GameMode/WpGameGameState.h"
#include "Warpunk/Game/Intel/WpIntelComponent.h"
#include "Warpunk/Game/Order/WpOrderReceiverComponent.h"
#include "Warpunk/Game/Pool/WpActorPool.h"

#include "AIController.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"

UWpProductionComponent::UWpProductionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicatedByDefault(false);
}

void UWpProductionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (const APawn* const Pawn = GetOwner<APawn>())
	{
		AIController = Pawn->GetController<AAIController>();
	}
}

void UWpProductionComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* const ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetOwner<IWpEntityInterface>()->GetEnduranceComponent()->GetEntityState() != EWpEntityState::Operational)
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
				FindNewTarget();
			}
			break;
		default:
			break;
		}
	}
}

void UWpProductionComponent::OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
{
	if (bIsAcquired)
	{
		const FWpPooledEntityData& PooledEntityData = static_cast<const FWpPooledEntityData&>(PooledObjectData);
		const FWpProductionConfig* const ProductionConfig = PooledEntityData.EntityConfig->ProductionConfig.GetPtrOrNull();

		if (ProductionConfig)
		{
			ensureAlways(ProductionConfig->Range >= 1.0f);
			ensureAlways(ProductionConfig->MatterConsumption >= 1.0f);
			ensureAlways(ProductionConfig->EnergyPerMatterRatio >= 0.001f);

			Range = ProductionConfig->Range;
			MatterConsumption = ProductionConfig->MatterConsumption;
			EnergyPerMatterRatio = ProductionConfig->EnergyPerMatterRatio;
			bCanProduce = ProductionConfig->bCanProduce;
			bShouldSupport = ProductionConfig->bIsSupport;

			ProductionState = EWpProductionState::Idle;
			EntityToProduceConfigId = 0;
			EntityToProduceLocation = FVector::ZeroVector;
			bInfiniteProductionEnabled = false;
			CurrentTargetEntity = nullptr;
			PriorityTargetEntity = nullptr;
		}

		bIsAcquired = ProductionConfig != nullptr;
	}

	if (!bIsAcquired)
	{
		ProxyOrderQueue.Empty();

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

	Stop();

	ProductionState = EWpProductionState::InitiatingProduction;
	EntityToProduceConfigId = EntityConfigId;
	EntityToProduceLocation = Location;
}

void UWpProductionComponent::StartRepair(IWpEntityInterface* const Entity)
{
	ensureAlways(IsActive());

	Stop();

	ProductionState = EWpProductionState::Repairing;
	CurrentTargetEntity = Entity;
}

void UWpProductionComponent::StartReclaim(IWpEntityInterface* const Entity)
{
	ensureAlways(IsActive());

	Stop();

	ProductionState = EWpProductionState::Reclaiming;
	CurrentTargetEntity = Entity;
}

void UWpProductionComponent::Stop()
{
	ensureAlways(IsActive());

	ProductionState = EWpProductionState::Idle;
	if (CurrentTargetEntity == PriorityTargetEntity)
	{
		PriorityTargetEntity = nullptr;
	}
	CurrentTargetEntity = nullptr;
}

bool UWpProductionComponent::CanReach(const FVector& Location) const
{
	ensureAlways(IsActive());

	const FVector OwnerLocation = GetOwner()->GetActorLocation();
	const float Distance2 = FVector::DistSquared2D(OwnerLocation, Location);

	return Distance2 <= Range * Range;
}

bool UWpProductionComponent::CanReach(const IWpEntityInterface* const Entity) const
{
	ensureAlways(IsActive());

	if (!Entity)
	{
		return false;
	}

	const FVector Location = Cast<AActor>(Entity)->GetActorLocation();
	return CanReach(Location);
}

bool UWpProductionComponent::CanRepair(const IWpEntityInterface* const Entity) const
{
	ensureAlways(IsActive());

	return Entity->GetEnduranceComponent()->CanBeRepaired(bCanProduce);
}

bool UWpProductionComponent::CanReachAndRepair(const IWpEntityInterface* const Entity) const
{
	ensureAlways(IsActive());

	return CanReach(Entity) && CanRepair(Entity);
}

void UWpProductionComponent::ReceiveProxyOrder(const FWpOrder& Order, const bool bOverride)
{
	ensureAlways(IsActive());

	if (bOverride)
	{
		ProxyOrderQueue.Empty();
	}

	ProxyOrderQueue.Add(Order);
}

float UWpProductionComponent::GetRange() const
{
	ensureAlways(IsActive());

	return Range;
}

void UWpProductionComponent::SetShouldSupport(const bool bInShouldSupport)
{
	ensureAlways(IsActive());

	bShouldSupport = bInShouldSupport;
}

bool UWpProductionComponent::ShouldSupport() const
{
	ensureAlways(IsActive());

	return bShouldSupport;
}

IWpEntityInterface* UWpProductionComponent::GetCurrentTargetEntity() const
{
	ensureAlways(IsActive());

	return CurrentTargetEntity;
}

void UWpProductionComponent::SetPriorityTargetEntity(IWpEntityInterface* const InPriorityTargetEntity)
{
	ensureAlways(IsActive());

	PriorityTargetEntity = InPriorityTargetEntity;

	if (!bShouldSupport)
	{
		StartRepair(PriorityTargetEntity);
	}
}

void UWpProductionComponent::EnableInfiniteProduction(const bool bInInfiniteProductionEnabled)
{
	ensureAlways(IsActive());

	bInfiniteProductionEnabled = bInInfiniteProductionEnabled;
}

bool UWpProductionComponent::IsInfiniteProductionEnabled() const
{
	ensureAlways(IsActive());

	return bInfiniteProductionEnabled;
}

UWpProductionComponent::FRepairFinishedEvent& UWpProductionComponent::OnRepairFinished()
{
	return RepairFinishedEvent;
}

UWpProductionComponent::FReclaimFinishedEvent& UWpProductionComponent::OnReclaimFinished()
{
	return ReclaimFinishedEvent;
}

void UWpProductionComponent::Produce()
{
	ensureAlways(IsActive());

	const UDataTable* const EntityConfigs = GetWorld()->GetGameState<AWpGameGameState>()->GetEntityConfigs();
	const FName RowName = FName{FString::FromInt(EntityToProduceConfigId)};
	const FWpEntityConfig& EntityConfig = *EntityConfigs->FindRow<FWpEntityConfig>(RowName, nullptr);

	EWpProductionState NewProductionState = EWpProductionState::Idle;
	if (EntityConfig.EntityClass->GetDefaultObject()->IsA<AWpBuilding>())
	{
		NewProductionState = EWpProductionState::ProducingBuilding;
	}
	else
	{
		NewProductionState = EWpProductionState::ProducingUnit;
	}

	const USceneComponent* BoundsComponent;
	if (NewProductionState == EWpProductionState::ProducingBuilding)
	{
		BoundsComponent = AWpBuilding::StaticClass()->GetDefaultObject<AActor>()->GetComponentByClass<UBoxComponent>();
	}
	else
	{
		BoundsComponent = AWpUnit::StaticClass()->GetDefaultObject<AActor>()->GetComponentByClass<UCapsuleComponent>();
	}
	const FBoxSphereBounds ActorBounds = BoundsComponent->GetLocalBounds();
	const FVector SpawnLocation = EntityToProduceLocation + ActorBounds.BoxExtent * 0.5f * FVector::UpVector;

	TArray<AActor*> Actors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel1));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel5));
	UKismetSystemLibrary::BoxOverlapActors(GetWorld(), SpawnLocation, ActorBounds.BoxExtent, ObjectTypes, AActor::StaticClass(), {}, Actors);
	UKismetSystemLibrary::DrawDebugBox(GetWorld(), SpawnLocation, ActorBounds.BoxExtent, FColor::Red);
	const bool bHasSpaceToProduce = Actors.IsEmpty();

	IWpEntityInterface* Entity = nullptr;

	if (bHasSpaceToProduce)
	{
		const TSubclassOf<AActor> ActorClass = (NewProductionState == EWpProductionState::ProducingBuilding) ? AWpBuilding::StaticClass() : AWpUnit::StaticClass();

		const UWpBelligerentComponent* const BelligerentComponent = GetOwner<IWpEntityInterface>()->GetBelligerentComponent();

		FWpPooledEntityData PooledEntityData;
		PooledEntityData.EntityConfig = &EntityConfig;
		PooledEntityData.EntityConfigId = EntityToProduceConfigId;
		PooledEntityData.BelligerentId = BelligerentComponent->GetBelligerentId();
		PooledEntityData.TeamId = BelligerentComponent->GetTeamId();

		AActor* const Actor = AWpActorPool::Acquire(ActorClass, SpawnLocation, FRotator::ZeroRotator, PooledEntityData);

		Entity = Cast<IWpEntityInterface>(Actor);
	}
	else
	{
		if (NewProductionState != EWpProductionState::ProducingBuilding)
		{
			return;
		}

		FHitResult Hit;
		const FVector SmallDelta = FVector::UpVector;
		const FVector TraceStart = SpawnLocation - SmallDelta;
		const FVector TraceEnd = SpawnLocation + SmallDelta;
		GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Camera);
		Entity = Cast<IWpEntityInterface>(Hit.GetActor());
		if (!Entity)
		{
			return;
		}

		const bool bHasSameEntityConfigId = Entity->GetConfigId() == EntityToProduceConfigId;
		const bool bBelongsToSameBelligerent = GetOwner<IWpEntityInterface>()->GetBelligerentComponent()->BelongsToSameBelligerent(Entity->GetBelligerentComponent());
		if (!bHasSameEntityConfigId || !bBelongsToSameBelligerent)
		{
			return;
		}
	}

	ProductionState = NewProductionState;
	CurrentTargetEntity = Entity;

	FWpOrder RepairOrder;
	RepairOrder.Type = EWpOrderType::Repair;
	RepairOrder.TargetEntityActor = Cast<AActor>(CurrentTargetEntity);
	GetOwner<IWpEntityInterface>()->GetOrderReceiverComponent()->NotifyOrderReceived(RepairOrder);
}

void UWpProductionComponent::Repair(const float DeltaTime)
{
	ensureAlways(IsActive());

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

		IWpEntityInterface* const LastTargetEntity = CurrentTargetEntity;

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

void UWpProductionComponent::FindNewTarget()
{
	ensureAlways(IsActive());

	const UWpIntelComponent* const IntelComponent = GetOwner<IWpEntityInterface>()->GetIntelComponent();
	for (IWpEntityInterface* const CandidateEntity : IntelComponent->GetIntel())
	{
		const bool bCanReachAndRepairCandidate = CanReachAndRepair(CandidateEntity);
		const bool bCandidateIsCurrentTarget = CurrentTargetEntity == CandidateEntity;
		if (!bCanReachAndRepairCandidate || bCandidateIsCurrentTarget)
		{
			continue;
		}

		const bool bCandidateIsPriorityTarget = CandidateEntity == PriorityTargetEntity;
		const bool bCanReachAndRepairCurrentTarget = CanReachAndRepair(CurrentTargetEntity);
		const bool bCandidateBelongsToSameTeam = GetOwner<IWpEntityInterface>()->GetBelligerentComponent()->BelongsToSameTeam(CandidateEntity->GetBelligerentComponent());
		if (bCandidateIsPriorityTarget || (!bCanReachAndRepairCurrentTarget && bCandidateBelongsToSameTeam))
		{
			StartRepair(CandidateEntity);
			break;
		}
	}
}
