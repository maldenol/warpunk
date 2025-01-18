#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Order/WpOrder.h"
#include "Warpunk/Game/Order/WpOrderHandlerWrapper.h"
#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"

#include "Components/ActorComponent.h"

#include "WpOrderReceiverComponent.generated.h"

class UWidgetComponent;

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class UWpOrderReceiverComponent final : public UActorComponent, public IWpPooledObjectInterface
{
	GENERATED_BODY()

public:
	DECLARE_EVENT_OneParam(FWpOrderReceiver, FOrderReceivedEvent, const FWpOrder&);

	UWpOrderReceiverComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* const ThisTickFunction) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const FWpPooledObjectData& PooledObjectData) override;

	bool ReceiveOrder(const FWpOrder& Order, const bool bOverride);
	FWpOrder GetOrder() const;

	void SetDefaultOrders(const FWpDefaultOrders& InDefaultOrders);
	const FWpDefaultOrders& GetDefaultOrders() const;
	void SetSupportedOrders(const TArray<EWpOrderType>& InSupportedOrders);
	TArray<EWpOrderType> GetSupportedOrders() const;
	void SetShouldProxyOrders(const bool bInShouldProxyOrders);

	void Select(const bool bSelected);

	void NotifyOrderReceived(const FWpOrder& Order);

	void SetUnitToProduceConfigIdNumbers(const TArray<FWpConfigIdNumber>& InUnitToProduceConfigIdNumbers);
	const TArray<FWpConfigIdNumber>& GetUnitToProduceConfigIdNumbers() const;

	FOrderReceivedEvent& OnOrderReceived();

private:
	void ActivateOrder(const FWpOrder& Order);
	void ReactivateOrder(FWpOrderHandler* const OrderHandler, const FWpOrder& Order);
	void DeactivateOrder(FWpOrderHandler* const OrderHandler);
	void ExecuteOrder(const EWpOrderType OrderType, const FWpOrder& Order);

	void CreateSelectionWidgetComponentIfNull();

	UPROPERTY(VisibleInstanceOnly, Replicated)
	FWpDefaultOrders DefaultOrders = {};
	UPROPERTY(VisibleInstanceOnly, Replicated)
	TArray<EWpOrderType> SupportedOrders = {};
	UPROPERTY(VisibleInstanceOnly)
	bool bShouldProxyOrders = false;

	FWpOrderHandlerWrapper CurrentOrderHandler = {};
	TQueue<FWpOrder> OrderQueue = {};
	UPROPERTY(Replicated)
	TArray<FWpConfigIdNumber> UnitToProduceConfigIdNumbers = {};

	UPROPERTY(VisibleInstanceOnly)
	UWidgetComponent* SelectionWidgetComponent = nullptr;

	FOrderReceivedEvent OrderReceivedEvent = {};

	static const FName SelectionWidgetComponentName;

	static const TSet<EWpOrderType> ImmediateOrders;
	static const TSet<EWpOrderType> ProductionProxyOrders;
};
