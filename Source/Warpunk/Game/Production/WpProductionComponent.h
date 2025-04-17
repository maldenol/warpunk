#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"

#include "Components/ActorComponent.h"

#include "WpProductionComponent.generated.h"

class AWpEntity;
enum class EWpEntityState : uint8;
struct FWpOrder;

class AAIController;

UENUM(NotBlueprintable, NotBlueprintType)
enum class EWpProductionState : uint8
{
	Idle,
	InitiatingProduction,
	ProducingBuilding,
	ProducingUnit,
	Repairing,
	Reclaiming,
};

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class UWpProductionComponent final : public UActorComponent, public IWpPooledObjectInterface
{
	GENERATED_BODY()

public:
	DECLARE_EVENT_OneParam(FWpProduction, FRepairFinishedEvent, AWpEntity*);

	DECLARE_EVENT(FWpProduction, FReclaimFinishedEvent);

	UWpProductionComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* const ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const FWpPooledObjectData& PooledObjectData) override;

	void StartProduce(const uint32 EntityConfigId, const FVector& Location);
	void StartRepair(AWpEntity* const Entity);
	void StartReclaim(AWpEntity* const Entity);
	void Stop();

	bool CanReach(const FVector& Location) const;
	bool CanReach(const AWpEntity* const Entity) const;
	bool CanRepair(const AWpEntity* const Entity) const;
	bool CanReachAndRepair(const AWpEntity* const Entity) const;

	void ReceiveProxyOrder(const FWpOrder& Order, const bool bOverride);

	float GetRange() const;
	void SetShouldSupport(const bool bInShouldSupport);
	bool ShouldSupport() const;
	AWpEntity* GetCurrentTargetEntity() const;
	void SetPriorityTargetEntity(AWpEntity* const InPriorityTargetEntity);
	void EnableInfiniteProduction(const bool bInInfiniteProductionEnabled);
	bool IsInfiniteProductionEnabled() const;

	FRepairFinishedEvent& OnRepairFinished();
	FReclaimFinishedEvent& OnReclaimFinished();

private:
	void Produce();
	void Repair(const float DeltaTime);
	void Reclaim(const float DeltaTime);

	void SetCurrentTargetEntity(AWpEntity* const NewTargetEntity);

	void UpdateCurrentTargetEntity();

	void OnCurrentTargetEntityStateChanged(AWpEntity* const Entity, const EWpEntityState EntityState);
	void OnOrderReceived(const FWpOrder& InOrder);

	UPROPERTY(VisibleInstanceOnly)
	float MatterConsumption = 0.0f;
	UPROPERTY(VisibleInstanceOnly)
	float EnergyPerMatterRatio = 0.0f;
	UPROPERTY(VisibleInstanceOnly)
	float Range = 0.0f;
	UPROPERTY(VisibleInstanceOnly)
	bool bCanProduce = false;
	UPROPERTY(VisibleInstanceOnly)
	bool bShouldSupport = false;

	UPROPERTY(VisibleInstanceOnly)
	AAIController* AIController = nullptr;

	UPROPERTY(VisibleInstanceOnly)
	EWpProductionState ProductionState = EWpProductionState::Idle;
	UPROPERTY(VisibleInstanceOnly)
	uint32 EntityToProduceConfigId = 0;
	UPROPERTY(VisibleInstanceOnly)
	FVector EntityToProduceLocation = FVector::ZeroVector;
	UPROPERTY(VisibleInstanceOnly)
	bool bInfiniteProductionEnabled = false;
	UPROPERTY(VisibleInstanceOnly)
	TArray<FWpOrder> ProxyOrderQueue = {};
	UPROPERTY(VisibleInstanceOnly)
	AWpEntity* CurrentTargetEntity = nullptr;
	UPROPERTY(VisibleInstanceOnly)
	AWpEntity* PriorityTargetEntity = nullptr;

	float LastUpdateCurrentTargetEntityTime = 0.0f;

	FRepairFinishedEvent RepairFinishedEvent = {};
	FReclaimFinishedEvent ReclaimFinishedEvent = {};

public:
	static const float ProductionLocationDistance;

	static const float UpdateCurrentTargetEntityCooldown;
};
