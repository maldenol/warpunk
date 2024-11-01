#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Order/WpOrderHandler.h"

class FWpProduction;

class FWpProduceUnitOrderHandler final : public FWpOrderHandler
{
public:
	virtual void SetOwnerEntityComponent(UWpEntityComponent* const InOwnerEntityComponent) override;

	virtual void Reactivate() override;
	virtual void Deactivate() override;

private:
	void BeginProduceUnit();

	void OnRepairFinished(UWpEntityComponent* const EntityComponent);

	FWpProduction* Production = nullptr;

	TArray<uint32> UnitQueue = {};
	int32 UnitQueueIndex = 0;
	TMap<uint32, uint32> ProducedUnits = {};

	FDelegateHandle OnRepairFinishedDelegateHandle = {};
};
