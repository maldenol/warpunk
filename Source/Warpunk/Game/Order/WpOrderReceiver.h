#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Order/WpOrder.h"
#include "Warpunk/Game/Order/WpOrderHandlerWrapper.h"
#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"

class UWpEntityComponent;

class UWidgetComponent;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnOrderReceived, const FWpOrder&);

class FWpOrderReceiver final : public IWpPooledObjectInterface
{
public:
	virtual ~FWpOrderReceiver() override;

	virtual void OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const void* const CustomData) override;

	void Initialize(UWpEntityComponent* const InOwnerEntityComponent);

	void SetIsActive(const bool bInIsActive);
	bool IsActive() const;

	void Tick();

	bool ReceiveOrder(const FWpOrder& Order, const bool bOverride);
	FWpOrder GetOrder() const;

	void SetDefaultOrders(const FWpDefaultOrders& InDefaultOrders);
	const FWpDefaultOrders& GetDefaultOrders() const;
	void SetSupportedOrders(const TSet<EWpOrderType>& InSupportedOrders);
	TSet<EWpOrderType> GetSupportedOrders() const;
	void SetShouldProxyOrders(const bool bInShouldProxyOrders);

	void Select(const bool bSelected);

	FOnOrderReceived OnOrderReceived = {};

private:
	void ActivateOrder(const FWpOrder& Order);
	void ReactivateOrder(FWpOrderHandler* const OrderHandler, const FWpOrder& Order);
	void DeactivateOrder(FWpOrderHandler* const OrderHandler);
	void ExecuteOrder(const EWpOrderType OrderType, const FWpOrder& Order);

	UWpEntityComponent* OwnerEntityComponent = nullptr;
	bool bIsActive = false;

	FWpDefaultOrders DefaultOrders = {};
	TSet<EWpOrderType> SupportedOrders = {};
	bool bShouldProxyOrders = false;

	FWpOrderHandlerWrapper CurrentOrderHandler = {};
	TQueue<FWpOrder> OrderQueue = {};

	UWidgetComponent* SelectionWidgetComponent = nullptr;
};
