#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"

struct FWpOrder;
class UWpEntityComponent;

class AAIController;

class FWpProduction final : public IWpPooledObjectInterface
{
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
	DECLARE_EVENT_OneParam(FWpProduction, FRepairFinishedEvent, UWpEntityComponent*);

	DECLARE_EVENT(FWpProduction, FReclaimFinishedEvent);

	virtual void OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const void* const CustomData) override;

	void Initialize(UWpEntityComponent* const InOwnerEntityComponent);

	bool IsActive() const;

	void Tick(const float DeltaTime);

	void StartProduce(const uint32 InEntityToProduceId, const FVector& InTargetLocation);
	void StartRepair(UWpEntityComponent* const InTargetEntityComponent);
	void StartReclaim(UWpEntityComponent* const InTargetEntityComponent);
	void Stop();

	bool CanReach(const FVector& InTargetLocation) const;
	bool CanReach(const UWpEntityComponent* const InTargetEntityComponent) const;
	bool CanRepair(const UWpEntityComponent* const InTargetEntityComponent) const;
	bool CanReachAndRepair(const UWpEntityComponent* const InTargetEntityComponent) const;

	void ReceiveProxyOrder(const FWpOrder& Order, const bool bOverride);

	float GetRange() const;
	void SetShouldSupport(const bool bInShouldSupport);
	bool ShouldSupport() const;
	UWpEntityComponent* GetCurrentTarget() const;
	void SetPriorityTarget(UWpEntityComponent* const InPriorityTargetEntityComponent);
	void SetInfiniteProduction(const bool bInInfiniteProduction);
	bool GetInfiniteProduction() const;

	FRepairFinishedEvent& OnRepairFinished();
	FReclaimFinishedEvent& OnReclaimFinished();

private:
	void Produce();
	void Repair(const float DeltaTime);
	void Reclaim(const float DeltaTime);

	void FindNewTarget();

	UWpEntityComponent* OwnerEntityComponent = nullptr;
	bool bIsActive = false;

	float MatterConsumption = 0.0f;
	float EnergyPerMatterRatio = 0.0f;
	float Range = 0.0f;
	bool bCanProduce = false;
	bool bShouldSupport = false;

	AAIController* AIController = nullptr;

	EWpProductionState ProductionState = EWpProductionState::Idle;
	FVector TargetLocation = FVector::ZeroVector;
	UWpEntityComponent* CurrentTargetEntityComponent = nullptr;
	UWpEntityComponent* PriorityTargetEntityComponent = nullptr;
	uint32 EntityToProduceId = 0;
	bool bInfiniteProduction = false;
	TArray<FWpOrder> ProxyOrderQueue = {};

	FRepairFinishedEvent RepairFinishedEvent = {};
	FReclaimFinishedEvent ReclaimFinishedEvent = {};
};
