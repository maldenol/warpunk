#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Order/WpOrder.h"

class AWpBuilding;
class UWpEntityComponent;

class FWpOrderSender
{
	enum class EWpOrderSenderState : uint8
	{
		DefaultOrder,
		SelectedOrder,
		ProduceBuildingOrder,
		ProduceUnitOrder,
	};

public:
	DECLARE_EVENT_OneParam(FWpOrderSender, FAvailableOrdersChangedEvent, const TSet<EWpOrderType>&);

	void Initialize(APlayerController* InPlayerController);

	void Tick();

	void SelectOrderType(const EWpOrderType OrderType);

	void SelectBuildingToProduce(const uint32 BuildingConfigId);

	void AddUnitToProduce(const uint32 UnitConfigId);
	void RemoveUnitToProduce(const uint32 UnitConfigId);
	TMap<uint32, uint32> GetUnitToProduceConfigIds();

	void OnLMBStarted();
	void OnLMBTriggered();
	void OnLMBCompleted();
	void OnRMBStarted();
	void OnRMBTriggered();
	void OnRMBCompleted();

	FAvailableOrdersChangedEvent& OnAvailableOrdersChanged();

	static const FName OverrideInputKeyName;

private:
	void SetSelectEntitiesStartLocation();
	void SelectEntities();
	TArray<UWpEntityComponent*> GetSelectedEntities(FWpDefaultOrders* const OutDefaultOrders) const;

	void SetOrderStartLocation();
	void SendOrders(const EWpOrderType OrderType);

	void UpdateBuildingPlan();

	void PopulateUnitToProduceClassNumbersPerProducer(const TArray<UWpEntityComponent*>& SelectedEntities);

	FHitResult LineTraceUnderCursor() const;

	APlayerController* PlayerController = nullptr;

	uint32 BelligerentId = 0;

	EWpOrderSenderState OrderSenderState = EWpOrderSenderState::DefaultOrder;

	FVector SelectEntitiesStartLocation = FVector::ZeroVector;
	TMap<FWpDefaultOrders, TArray<UWpEntityComponent*>> SelectedEntitiesPerDefaultOrders = {};

	EWpOrderType SelectedOrderType = EWpOrderType::Undefined;
	FVector OrderStartLocation = FVector::ZeroVector;

	uint32 BuildingToProduceConfigId = 0;
	TObjectPtr<AWpBuilding> BuildingToProduce = nullptr;

	TArray<TMap<uint32, uint32>> UnitToProduceConfigIdNumbersPerProducer = {};

	FAvailableOrdersChangedEvent AvailableOrdersChangedEvent = {};
};
