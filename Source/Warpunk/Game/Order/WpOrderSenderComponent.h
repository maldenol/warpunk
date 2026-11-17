#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Entity/WpEntityType.h"
#include "Warpunk/Game/Order/WpOrder.h"

#include "Components/ActorComponent.h"

#include "WpOrderSenderComponent.generated.h"

class AWpCommandMarker;
class AWpLocalEntity;
class UWpSelectionMarqueeWidget;
enum class EWpEntityState : uint8;

USTRUCT(NotBlueprintable, NotBlueprintType)
struct FWpNetOrder
{
	GENERATED_BODY()

	UPROPERTY(VisibleInstanceOnly)
	AWpEntity* Entity = nullptr;
	UPROPERTY(VisibleInstanceOnly)
	FWpOrder Order = {};
};

struct FWpSelectedEntityCategory
{
	FWpDefaultOrders DefaultOrders = {};
	EWpEntityType EntityType = EWpEntityType::Undefined;

	bool operator==(const FWpSelectedEntityCategory& Other) const;
	friend uint32 GetTypeHash(const FWpSelectedEntityCategory& Other);
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
	const TArray<AWpEntity*>* GetSelectedEntitiesWithDefaultOrders(FWpDefaultOrders* const OutDefaultOrders) const;

	void SelectEntity(AWpEntity* const Entity, const bool bSelected);
	void UpdateAvailableOrderTypes();

	void SetOrderStartLocation();
	void SendOrders(const EWpOrderType OrderType);

	void EnsureOrderVisuals();
	void UpdateSelectionMarquee();
	void UpdateOrderPreview();
	void ShowOrderMarker(const FVector& Location, const float Radius, const EWpOrderType OrderType);
	static bool ShouldShowOrderMarker(const EWpOrderType OrderType);

	void UpdateBuildingPlan();

	void PopulateUnitToProduceConfigIdNumbersPerProducer();

	FHitResult LineTraceUnderCursor() const;

	APlayerController* GetPlayerController() const;
	uint32 GetBelligerentId() const;
	uint32 GetTeamId() const;

	void OnSelectedEntityStateChanged(AWpEntity* const SelectedEntity, const EWpEntityState EntityState);

	EWpOrderSenderState OrderSenderState = EWpOrderSenderState::DefaultOrder;

	FVector SelectEntitiesStartLocation = FVector::ZeroVector;
	FVector2D SelectEntitiesStartScreen = FVector2D::ZeroVector;
	bool bSelectingEntities = false;
	TMap<FWpSelectedEntityCategory, TArray<AWpEntity*>> SelectedEntitiesPerCategory = {};

	TSet<AWpEntity*> SelectedEntities = {};
	TSet<EWpOrderType> SelectedEntitiesAvailableOrderTypes = {};
	FWpDefaultOrders SelectedEntitiesDefaultOrders = {};

	EWpOrderType SelectedOrderType = EWpOrderType::Undefined;
	FVector OrderStartLocation = FVector::ZeroVector;

	uint32 BuildingToProduceConfigId = 0;
	TObjectPtr<AWpLocalEntity> BuildingToProduce = nullptr;

	TMap<const AWpEntity*, TMap<uint32, uint32>> UnitToProduceConfigIdNumbersPerProducer = {};

	FAvailableOrdersChangedEvent AvailableOrdersChangedEvent = {};

	UPROPERTY()
	TObjectPtr<UWpSelectionMarqueeWidget> SelectionMarquee = nullptr;
	UPROPERTY()
	TObjectPtr<AWpCommandMarker> OrderPreview = nullptr;

public:
	static const FName OverrideInputKeyName;
};
