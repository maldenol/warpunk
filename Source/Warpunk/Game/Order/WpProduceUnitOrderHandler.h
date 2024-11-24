#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Order/WpOrderHandler.h"

class UWpProductionComponent;

class FWpProduceUnitOrderHandler final : public FWpOrderHandler
{
public:
	virtual void Initialize(IWpEntityInterface* const InEntity) override;

	virtual void Reactivate() override;
	virtual void Deactivate() override;

	virtual void SetOrder(const FWpOrder& InOrder) override;

private:
	void BeginProduceUnit();

	void OnRepairFinished(IWpEntityInterface* const InEntity);

	UWpProductionComponent* ProductionComponent = nullptr;

	TArray<uint32> UnitQueue = {};
	int32 UnitQueueIndex = 0;
	TMap<uint32, uint32> ProducedUnits = {};
	bool bWasInfiniteProductionEnabled = false;

	FDelegateHandle OnRepairFinishedDelegateHandle = {};
};
