#include "WpProduction.h"

#include "Warpunk/Game/Data/WpEntityConfig.h"
#include "Warpunk/Game/Data/WpPooledEntityData.h"
#include "Warpunk/Game/Entity/WpBuilding.h"
#include "Warpunk/Game/Entity/WpUnit.h"
#include "Warpunk/Game/EntityComponent/WpEntityComponent.h"
#include "Warpunk/Game/EntityComponent/WpEntityInterface.h"
#include "Warpunk/Game/GameMode/WpGameGameState.h"
#include "Warpunk/Game/Intel/WpIntel.h"
#include "Warpunk/Game/Pool/WpActorPool.h"

#include "AIController.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"

void FWpProduction::OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const void* const CustomData)
{
	if (bIsAcquired)
	{
		const FWpPooledEntityData* const PooledEntityData = static_cast<const FWpPooledEntityData*>(CustomData);
		const FWpProductionConfig* const ProductionConfig = PooledEntityData->EntityConfig->ProductionConfig.GetPtrOrNull();

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
			TargetLocation = FVector::ZeroVector;
			CurrentTargetEntityComponent = nullptr;
			PriorityTargetEntityComponent = nullptr;
			EntityToProduceID = 0;
			bInfiniteProduction = false;
		}

		bIsAcquired = ProductionConfig != nullptr;
	}

	if (!bIsAcquired)
	{
		ProxyOrderQueue.Empty();

		if (bIsActive)
		{
			Stop();
		}
	}

	bIsActive = bIsAcquired;
}

void FWpProduction::Initialize(UWpEntityComponent* const InOwnerEntityComponent)
{
	OwnerEntityComponent = InOwnerEntityComponent;

	if (const APawn* const Pawn = OwnerEntityComponent->GetOwner<APawn>())
	{
		AIController = Pawn->GetController<AAIController>();
	}
}

bool FWpProduction::IsActive() const
{
	return bIsActive;
}

void FWpProduction::Tick(const float DeltaTime)
{
	if (!bIsActive)
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

void FWpProduction::StartProduce(const uint32 InEntityToProduceID, const FVector& InTargetLocation)
{
	ensureAlways(bIsActive);

	Stop();

	ProductionState = EWpProductionState::InitiatingProduction;
	TargetLocation = InTargetLocation;
	EntityToProduceID = InEntityToProduceID;
}

void FWpProduction::StartRepair(UWpEntityComponent* const InTargetEntityComponent)
{
	ensureAlways(bIsActive);

	Stop();

	ProductionState = EWpProductionState::Repairing;
	CurrentTargetEntityComponent = InTargetEntityComponent;
}

void FWpProduction::StartReclaim(UWpEntityComponent* const InTargetEntityComponent)
{
	ensureAlways(bIsActive);

	Stop();

	ProductionState = EWpProductionState::Reclaiming;
	CurrentTargetEntityComponent = InTargetEntityComponent;
}

void FWpProduction::Stop()
{
	ensureAlways(bIsActive);

	ProductionState = EWpProductionState::Idle;
	if (CurrentTargetEntityComponent == PriorityTargetEntityComponent)
	{
		PriorityTargetEntityComponent = nullptr;
	}
	CurrentTargetEntityComponent = nullptr;
}

bool FWpProduction::CanReach(const FVector& InTargetLocation) const
{
	ensureAlways(bIsActive);

	const FVector OwnerLocation = OwnerEntityComponent->GetOwner()->GetActorLocation();
	const float Distance2 = FVector::DistSquared2D(OwnerLocation, InTargetLocation);

	return Distance2 <= Range * Range;
}

bool FWpProduction::CanReach(const UWpEntityComponent* const InTargetEntityComponent) const
{
	ensureAlways(bIsActive);

	if (!IsValid(InTargetEntityComponent))
	{
		return false;
	}

	const FVector InTargetLocation = InTargetEntityComponent->GetOwner()->GetActorLocation();
	return CanReach(InTargetLocation);
}

bool FWpProduction::CanRepair(const UWpEntityComponent* const InTargetEntityComponent) const
{
	ensureAlways(bIsActive);

	return WP_AS_ENDURANCE_CONST(InTargetEntityComponent).CanBeRepaired(bCanProduce);
}

bool FWpProduction::CanReachAndRepair(const UWpEntityComponent* const InTargetEntityComponent) const
{
	ensureAlways(bIsActive);

	return CanReach(InTargetEntityComponent) && CanRepair(InTargetEntityComponent);
}

void FWpProduction::ReceiveProxyOrder(const FWpOrder& Order, const bool bOverride)
{
	ensureAlways(bIsActive);

	if (bOverride)
	{
		ProxyOrderQueue.Empty();
	}

	ProxyOrderQueue.Add(Order);
}

float FWpProduction::GetRange() const
{
	ensureAlways(bIsActive);

	return Range;
}

void FWpProduction::SetShouldSupport(const bool bInShouldSupport)
{
	ensureAlways(bIsActive);

	bShouldSupport = bInShouldSupport;
}

bool FWpProduction::ShouldSupport() const
{
	ensureAlways(bIsActive);

	return bShouldSupport;
}

UWpEntityComponent* FWpProduction::GetCurrentTarget() const
{
	ensureAlways(bIsActive);

	return CurrentTargetEntityComponent;
}

void FWpProduction::SetPriorityTarget(UWpEntityComponent* const InPriorityTargetEntityComponent)
{
	ensureAlways(bIsActive);

	PriorityTargetEntityComponent = InPriorityTargetEntityComponent;

	if (!bShouldSupport)
	{
		StartRepair(PriorityTargetEntityComponent);
	}
}

void FWpProduction::SetInfiniteProduction(const bool bInInfiniteProduction)
{
	ensureAlways(bIsActive);

	bInfiniteProduction = bInInfiniteProduction;
}

bool FWpProduction::GetInfiniteProduction() const
{
	ensureAlways(bIsActive);

	return bInfiniteProduction;
}

void FWpProduction::Produce()
{
	ensureAlways(bIsActive);

	const UDataTable* const EntityConfigs = OwnerEntityComponent->GetWorld()->GetGameState<AWpGameGameState>()->GetEntityConfigs();
	const FName RowName = FName{FString::FromInt(EntityToProduceID)};
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
	const FVector SpawnLocation = TargetLocation + ActorBounds.BoxExtent * 0.5f * FVector::UpVector;

	TArray<AActor*> Actors = {};
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel1));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel5));
	UKismetSystemLibrary::BoxOverlapActors(OwnerEntityComponent->GetWorld(), SpawnLocation, ActorBounds.BoxExtent, ObjectTypes, AActor::StaticClass(), {}, Actors);
	UKismetSystemLibrary::DrawDebugBox(OwnerEntityComponent->GetWorld(), SpawnLocation, ActorBounds.BoxExtent, FColor::Red);
	const bool bHasSpaceToProduce = Actors.IsEmpty();

	IWpEntityInterface* Entity = nullptr;

	if (bHasSpaceToProduce)
	{
		const TSubclassOf<AActor> ActorClass = (NewProductionState == EWpProductionState::ProducingBuilding) ? AWpBuilding::StaticClass() : AWpUnit::StaticClass();

		FWpPooledEntityData PooledEntityData;
		PooledEntityData.EntityConfig = &EntityConfig;
		PooledEntityData.EntityConfigID = EntityToProduceID;
		PooledEntityData.BelligerentID = WP_AS_BELLIGERENT_MEMBER_CONST(OwnerEntityComponent).GetBelligerentID();

		AActor* const Actor = AWpActorPool::Acquire(ActorClass, SpawnLocation, FRotator::ZeroRotator, &PooledEntityData);

		Entity = Cast<IWpEntityInterface>(Actor);
	}
	else
	{
		if (NewProductionState != EWpProductionState::ProducingBuilding)
		{
			return;
		}

		FHitResult Hit;
		const FVector TraceStart = SpawnLocation - FVector::UpVector;
		const FVector TraceEnd = SpawnLocation + FVector::DownVector;
		OwnerEntityComponent->GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Camera);
		Entity = Cast<IWpEntityInterface>(Hit.GetActor());
		if (!Entity)
		{
			return;
		}

		const bool bHasSameEntityConfigID = Entity->GetEntityComponent()->GetEntityConfigID() == EntityToProduceID;
		const bool bBelongsToSameBelligerent = WP_AS_BELLIGERENT_MEMBER_CONST(OwnerEntityComponent).BelongsToSameBelligerent(Entity->GetEntityComponent());
		if (!bHasSameEntityConfigID || !bBelongsToSameBelligerent)
		{
			return;
		}
	}

	ProductionState = NewProductionState;
	CurrentTargetEntityComponent = Entity->GetEntityComponent();

	FWpOrder RepairOrder;
	RepairOrder.Type = EWpOrderType::Repair;
	RepairOrder.TargetEntityComponent = CurrentTargetEntityComponent;
	WP_AS_ORDER_RECEIVER(OwnerEntityComponent).OnOrderReceived.Broadcast(RepairOrder);
}

void FWpProduction::Repair(const float DeltaTime)
{
	ensureAlways(bIsActive);

	if (!CurrentTargetEntityComponent)
	{
		Stop();
		return;
	}

	if (!CanRepair(CurrentTargetEntityComponent))
	{
		if (ProductionState == EWpProductionState::ProducingUnit)
		{
			for (const FWpOrder& UnitOrder : ProxyOrderQueue)
			{
				WP_AS_ORDER_RECEIVER(CurrentTargetEntityComponent).ReceiveOrder(UnitOrder, false);
			}
		}

		UWpEntityComponent* const LastTargetEntityComponent = CurrentTargetEntityComponent;

		Stop();

		OnRepairFinished.Broadcast(LastTargetEntityComponent);

		return;
	}

	if (AIController && AIController->GetMoveStatus() != EPathFollowingStatus::Type::Idle)
	{
		return;
	}

	if (CanReach(CurrentTargetEntityComponent))
	{
		WP_AS_ENDURANCE(CurrentTargetEntityComponent).ReceiveRepair(DeltaTime * MatterConsumption);
	}
}

void FWpProduction::Reclaim(const float DeltaTime)
{
	ensureAlways(bIsActive);

	if (!CurrentTargetEntityComponent)
	{
		Stop();
		return;
	}

	if (!CanRepair(CurrentTargetEntityComponent))
	{
		Stop();

		OnReclaimFinished.Broadcast();

		return;
	}

	WP_AS_ENDURANCE(CurrentTargetEntityComponent).ReceiveReclaim(DeltaTime * MatterConsumption);
}

void FWpProduction::FindNewTarget()
{
	ensureAlways(bIsActive);

	const FWpIntel& Intel = WP_AS_INTEL_CONST(OwnerEntityComponent);
	for (UWpEntityComponent* const CandidateEntityComponent : Intel.GetIntel())
	{
		if (!IsValid(CandidateEntityComponent))
		{
			continue;
		}

		const bool bCanReachAndRepairCandidate = CanReachAndRepair(CandidateEntityComponent);
		const bool bCandidateIsCurrentTarget = CurrentTargetEntityComponent == CandidateEntityComponent;
		if (!bCanReachAndRepairCandidate || bCandidateIsCurrentTarget)
		{
			continue;
		}

		const bool bCandidateIsPriorityTarget = CandidateEntityComponent == PriorityTargetEntityComponent;
		const bool bCanReachAndRepairCurrentTarget = CanReachAndRepair(CurrentTargetEntityComponent);
		const bool bCandidateBelongsToSameTeam = WP_AS_BELLIGERENT_MEMBER_CONST(OwnerEntityComponent).BelongsToSameTeam(CandidateEntityComponent);
		if (bCandidateIsPriorityTarget || (!bCanReachAndRepairCurrentTarget && bCandidateBelongsToSameTeam))
		{
			StartRepair(CandidateEntityComponent);
			break;
		}
	}
}
