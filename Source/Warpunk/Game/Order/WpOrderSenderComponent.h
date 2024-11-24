#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Order/WpOrder.h"

#include "Components/ActorComponent.h"

#include "WpOrderSenderComponent.generated.h"

class AWpBuilding;

USTRUCT(NotBlueprintable, NotBlueprintType)
struct FWpNetOrder
{
	GENERATED_BODY()

	UPROPERTY()
	AActor* EntityActor = nullptr;
	UPROPERTY()
	FWpOrder Order = {};
};

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class UWpOrderSenderComponent : public UActorComponent
{
	GENERATED_BODY()

	enum class EWpOrderSenderState : uint8
	{
		DefaultOrder,
		SelectedOrder,
		ProduceBuildingOrder,
		ProduceUnitOrder,
	};

public:
	DECLARE_EVENT_OneParam(FWpOrderSender, FAvailableOrdersChangedEvent, const TSet<EWpOrderType>&);

	UWpOrderSenderComponent();

	virtual void TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* const ThisTickFunction) override;

	void SelectOrderType(const EWpOrderType OrderType);

	void SelectBuildingToProduce(const uint32 BuildingConfigId);

	void AddUnitToProduce(const uint32 UnitConfigId);
	void RemoveUnitToProduce(const uint32 UnitConfigId);
	TMap<uint32, uint32> GetUnitsToProduce();

	void OnLMBStarted();
	void OnLMBTriggered();
	void OnLMBCompleted();
	void OnRMBStarted();
	void OnRMBTriggered();
	void OnRMBCompleted();

	FAvailableOrdersChangedEvent& OnAvailableOrdersChanged();

private:
	UFUNCTION(Server, Reliable)
	void ServerSendOrders(const TArray<FWpNetOrder>& NetOrders, const bool bOverride);

	void SetSelectEntitiesStartLocation();
	void SelectEntities();
	TArray<IWpEntityInterface*> GetSelectedEntities(FWpDefaultOrders* const OutDefaultOrders) const;

	void SetOrderStartLocation();
	void SendOrders(const EWpOrderType OrderType);

	void UpdateBuildingPlan();

	void PopulateUnitToProduceConfigIdNumbersPerProducer(const TArray<IWpEntityInterface*>& SelectedEntities);

	FHitResult LineTraceUnderCursor() const;

	APlayerController* GetPlayerController() const;
	uint32 GetBelligerentId() const;

	EWpOrderSenderState OrderSenderState = EWpOrderSenderState::DefaultOrder;

	FVector SelectEntitiesStartLocation = FVector::ZeroVector;
	TMap<FWpDefaultOrders, TArray<IWpEntityInterface*>> SelectedEntitiesPerDefaultOrders = {};

	EWpOrderType SelectedOrderType = EWpOrderType::Undefined;
	FVector OrderStartLocation = FVector::ZeroVector;

	uint32 BuildingToProduceConfigId = 0;
	TObjectPtr<AWpBuilding> BuildingToProduce = nullptr;

	TArray<TMap<uint32, uint32>> UnitToProduceConfigIdNumbersPerProducer = {};

	FAvailableOrdersChangedEvent AvailableOrdersChangedEvent = {};

	static const FName OverrideInputKeyName;
};
