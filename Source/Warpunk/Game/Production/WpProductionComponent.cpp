#include "WpProductionComponent.h"

#include "Warpunk/Game/Belligerent/WpBelligerentComponent.h"
#include "Warpunk/Game/Endurance/WpEnduranceComponent.h"
#include "Warpunk/Game/Entity/WpBuilding.h"
#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Entity/WpEntityInterface.h"
#include "Warpunk/Game/Entity/WpPooledEntityData.h"
#include "Warpunk/Game/Entity/WpUnit.h"
#include "Warpunk/Game/Intel/WpIntelComponent.h"
#include "Warpunk/Game/Order/WpOrderReceiverComponent.h"
#include "Warpunk/Game/Pool/WpActorPool.h"
#include "Warpunk/Global/GameInstance/WpGameInstance.h"

#include "AIController.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"

UWpProductionComponent::UWpProductionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.SetTickFunctionEnable(false);

	SetIsReplicatedByDefault(false);
}

void UWpProductionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner()->HasAuthority())
	{
		if (const APawn* const Pawn = GetOwner<APawn>())
		{
			AIController = Pawn->GetController<AAIController>();
		}

		UWpOrderReceiverComponent* const OrderReceiverComponent = Cast<IWpEntityInterface>(GetOwner())->GetOrderReceiverComponent();
		OrderReceiverComponent->OnOrderReceived().AddUObject(this, &UWpProductionComponent::OnOrderReceived);
	}
}

void UWpProductionComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* const ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

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
				UpdateCurrentTargetEntity();
			}
			break;
		default:
			break;
		}
	}
}

void UWpProductionComponent::OnRep_IsActive()
{
	// Override to not enable tick
}

void UWpProductionComponent::OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
{
	ensureAlways(GetOwner()->HasAuthority());

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
		}

		bIsAcquired = (ProductionConfig != nullptr);
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
	SetComponentTickEnabled(bIsAcquired);
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

void UWpProductionComponent::StartRepair(IWpEntityInterface* const Entity)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	Stop();

	ProductionState = EWpProductionState::Repairing;
	SetCurrentTargetEntity(Entity);
}

void UWpProductionComponent::StartReclaim(IWpEntityInterface* const Entity)
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

bool UWpProductionComponent::CanReach(const IWpEntityInterface* const Entity) const
{
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

IWpEntityInterface* UWpProductionComponent::GetCurrentTargetEntity() const
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	return CurrentTargetEntity;
}

void UWpProductionComponent::SetPriorityTargetEntity(IWpEntityInterface* const InPriorityTargetEntity)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	const UWpBelligerentComponent* const BelligerentComponent = Cast<IWpEntityInterface>(GetOwner())->GetBelligerentComponent();
	const bool bPriorityTargetBelongsToSameTeam = BelligerentComponent->BelongsToSameTeam(InPriorityTargetEntity->GetBelligerentComponent());
	if (!bPriorityTargetBelongsToSameTeam)
	{
		return;
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

	const UDataTable* const EntityConfigs = GetWorld()->GetGameInstance<UWpGameInstance>()->EntityConfigs;
	const FName RowName = FName{FString::FromInt(EntityToProduceConfigId)};
	const FWpEntityConfig& EntityConfig = *EntityConfigs->FindRow<FWpEntityConfig>(RowName, nullptr);

	EWpProductionState NewProductionState;
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
	const TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes =
	{
		UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel1),
		UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel5),
	};
	UKismetSystemLibrary::BoxOverlapActors(GetWorld(), SpawnLocation, ActorBounds.BoxExtent, ObjectTypes, AActor::StaticClass(), {}, Actors);
	UKismetSystemLibrary::DrawDebugBox(GetWorld(), SpawnLocation, ActorBounds.BoxExtent, FColor::Red);
	const bool bHasSpaceToProduce = Actors.IsEmpty();

	IWpEntityInterface* Entity;

	if (bHasSpaceToProduce)
	{
		const TSubclassOf<AActor>& ActorClass = (NewProductionState == EWpProductionState::ProducingBuilding) ? AWpBuilding::StaticClass() : AWpUnit::StaticClass();

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

void UWpProductionComponent::SetCurrentTargetEntity(IWpEntityInterface* const NewTargetEntity)
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

	const UWpIntelComponent* const IntelComponent = GetOwner<IWpEntityInterface>()->GetIntelComponent();
	const TSet<IWpEntityInterface*>& VisibleEntities = IntelComponent->GetVisibleEntities();

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

	for (IWpEntityInterface* const CandidateEntity : VisibleEntities)
	{
		const bool bCanReachAndRepairCandidate = CanReachAndRepair(CandidateEntity);
		if (!bCanReachAndRepairCandidate)
		{
			continue;
		}

		const bool bCandidateBelongsToSameTeam = GetOwner<IWpEntityInterface>()->GetBelligerentComponent()->BelongsToSameTeam(CandidateEntity->GetBelligerentComponent());
		if (!bCandidateBelongsToSameTeam)
		{
			continue;
		}

		StartRepair(CandidateEntity);
		break;
	}
}

void UWpProductionComponent::OnCurrentTargetEntityStateChanged(IWpEntityInterface* const Entity, const EWpEntityState EntityState)
{
	if (EntityState == EWpEntityState::Wreck)
	{
		IWpEntityInterface* const LastTargetEntity = CurrentTargetEntity;

		Stop();

		RepairFinishedEvent.Broadcast(LastTargetEntity);
	}
}

void UWpProductionComponent::OnOrderReceived(const FWpOrder& InOrder)
{
	if (InOrder.Type == EWpOrderType::Stop)
	{
		IWpEntityInterface* const LastTargetEntity = CurrentTargetEntity;

		Stop();

		RepairFinishedEvent.Broadcast(LastTargetEntity);

		PriorityTargetEntity = nullptr;
	}
}
