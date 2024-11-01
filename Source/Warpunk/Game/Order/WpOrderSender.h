#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Order/WpOrder.h"

class AWpBuilding;
class UWpEntityComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnAvailableOrdersChanged, const TSet<EWpOrderType>&);

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
	void Initialize(APlayerController* InPlayerController);

	void Tick();

	void SelectOrderType(const EWpOrderType OrderType);

	void SelectBuildingToProduce(const uint32 BuildingConfigID);

	void AddUnitToProduce(const uint32 UnitConfigID);
	void RemoveUnitToProduce(const uint32 UnitConfigID);
	TMap<uint32, uint32> GetUnitToProduceConfigIDs();

	void OnLMBStarted();
	void OnLMBTriggered();
	void OnLMBCompleted();
	void OnRMBStarted();
	void OnRMBTriggered();
	void OnRMBCompleted();

	FOnAvailableOrdersChanged OnAvailableOrdersChanged = {};

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

	uint32 BelligerentID = 0;

	EWpOrderSenderState OrderSenderState = EWpOrderSenderState::DefaultOrder;

	FVector SelectEntitiesStartLocation = FVector::ZeroVector;
	TMap<FWpDefaultOrders, TArray<UWpEntityComponent*>> SelectedEntitiesPerDefaultOrders = {};

	EWpOrderType SelectedOrderType = EWpOrderType::Undefined;
	FVector OrderStartLocation = FVector::ZeroVector;

	uint32 BuildingToProduceConfigID = 0;
	TObjectPtr<AWpBuilding> BuildingToProduce = nullptr;

	TArray<TMap<uint32, uint32>> UnitToProduceConfigIDNumbersPerProducer = {};
};
