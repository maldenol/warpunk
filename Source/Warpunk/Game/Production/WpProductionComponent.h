#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"

#include "Components/ActorComponent.h"

#include "WpProductionComponent.generated.h"

struct FWpOrder;
class IWpEntityInterface;

class AAIController;

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class UWpProductionComponent final : public UActorComponent, public IWpPooledObjectInterface
{
	GENERATED_BODY()

	enum class EWpProductionState : uint8
	{
		Idle,
		InitiatingProduction,
		ProducingBuilding,
		ProducingUnit,
		Repairing,
		Reclaiming,
	};

public:
	DECLARE_EVENT_OneParam(FWpProduction, FRepairFinishedEvent, IWpEntityInterface*);

	DECLARE_EVENT(FWpProduction, FReclaimFinishedEvent);

	UWpProductionComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* const ThisTickFunction) override;

	virtual void OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const FWpPooledObjectData& PooledObjectData) override;

	void StartProduce(const uint32 EntityConfigId, const FVector& Location);
	void StartRepair(IWpEntityInterface* const Entity);
	void StartReclaim(IWpEntityInterface* const Entity);
	void Stop();

	bool CanReach(const FVector& Location) const;
	bool CanReach(const IWpEntityInterface* const Entity) const;
	bool CanRepair(const IWpEntityInterface* const Entity) const;
	bool CanReachAndRepair(const IWpEntityInterface* const Entity) const;

	void ReceiveProxyOrder(const FWpOrder& Order, const bool bOverride);

	float GetRange() const;
	void SetShouldSupport(const bool bInShouldSupport);
	bool ShouldSupport() const;
	IWpEntityInterface* GetCurrentTargetEntity() const;
	void SetPriorityTargetEntity(IWpEntityInterface* const InPriorityTargetEntity);
	void EnableInfiniteProduction(const bool bInInfiniteProductionEnabled);
	bool IsInfiniteProductionEnabled() const;

	FRepairFinishedEvent& OnRepairFinished();
	FReclaimFinishedEvent& OnReclaimFinished();

private:
	void Produce();
	void Repair(const float DeltaTime);
	void Reclaim(const float DeltaTime);

	void UpdateCurrentTargetEntity();

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

	EWpProductionState ProductionState = EWpProductionState::Idle;
	uint32 EntityToProduceConfigId = 0;
	FVector EntityToProduceLocation = FVector::ZeroVector;
	bool bInfiniteProductionEnabled = false;
	TArray<FWpOrder> ProxyOrderQueue = {};
	IWpEntityInterface* CurrentTargetEntity = nullptr;
	IWpEntityInterface* PriorityTargetEntity = nullptr;

	float LastUpdateCurrentTargetEntityTime = 0.0f;
	const float UpdateCurrentTargetEntityCooldown = 0.25f;

	FRepairFinishedEvent RepairFinishedEvent = {};
	FReclaimFinishedEvent ReclaimFinishedEvent = {};
};
