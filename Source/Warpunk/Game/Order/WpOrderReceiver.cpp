#include "WpOrderReceiver.h"

#include "Warpunk/Game/Data/WpEntityConfig.h"
#include "Warpunk/Game/Data/WpPooledEntityData.h"
#include "Warpunk/Game/EntityComponent/WpEntityComponent.h"
#include "Warpunk/Game/GameMode/WpGameGameState.h"

#include "Components/WidgetComponent.h"

const FName FWpOrderReceiver::SelectionWidgetComponentName = TEXT("SelectionWidget");

const TSet<EWpOrderType> FWpOrderReceiver::ImmediateOrders =
{
	EWpOrderType::InfiniteUnitProductionOff,
	EWpOrderType::InfiniteUnitProductionOn,
	EWpOrderType::SelfDestruct,
};

const TSet<EWpOrderType> FWpOrderReceiver::ProductionProxyOrders =
{
	EWpOrderType::Move,
	EWpOrderType::Patrol,
	EWpOrderType::Assist,
	EWpOrderType::Attack,
	EWpOrderType::Repair,
	EWpOrderType::Reclaim,
};

FWpOrderReceiver::~FWpOrderReceiver()
{
	if (FWpOrderHandler* const OrderHandler = CurrentOrderHandler.Get())
	{
		DeactivateOrder(OrderHandler);
	}
}

void FWpOrderReceiver::OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const void* const CustomData)
{
	if (bIsAcquired)
	{
		const FWpPooledEntityData* const PooledEntityData = static_cast<const FWpPooledEntityData*>(CustomData);
		const FWpOrderReceiverConfig* const OrderReceiverConfig = PooledEntityData->EntityConfig->OrderReceiverConfig.GetPtrOrNull();

		if (OrderReceiverConfig)
		{
			ensureAlways(!OrderReceiverConfig->SupportedOrders.IsEmpty());

			DefaultOrders = OrderReceiverConfig->DefaultOrders;
			SupportedOrders = OrderReceiverConfig->SupportedOrders;
			bShouldProxyOrders = OrderReceiverConfig->bShouldProxyOrders;
		}

		bIsAcquired = OrderReceiverConfig != nullptr;
	}

	if (!bIsAcquired)
	{
		if (FWpOrderHandler* const OrderHandler = CurrentOrderHandler.Get())
		{
			DeactivateOrder(OrderHandler);
		}

		OrderQueue.Empty();
	}

	SelectionWidgetComponent->SetVisibility(false);

	bIsActive = bIsAcquired;
}

void FWpOrderReceiver::Initialize(UWpEntityComponent* const InOwnerEntityComponent)
{
	OwnerEntityComponent = InOwnerEntityComponent;

	SelectionWidgetComponent = NewObject<UWidgetComponent>(OwnerEntityComponent->GetOwner(), UWidgetComponent::StaticClass(), SelectionWidgetComponentName);
	SelectionWidgetComponent->RegisterComponent();
	SelectionWidgetComponent->AttachToComponent(OwnerEntityComponent->GetOwner()->GetRootComponent(), FAttachmentTransformRules{EAttachmentRule::KeepRelative, false});

	SelectionWidgetComponent->SetCollisionProfileName(TEXT("NoCollision"));
	SelectionWidgetComponent->SetNotifyRigidBodyCollision(false);
	SelectionWidgetComponent->SetGenerateOverlapEvents(false);

	SelectionWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	SelectionWidgetComponent->SetDrawAtDesiredSize(true);

	const TSubclassOf<UUserWidget> SelectionWidgetClass = OwnerEntityComponent->GetWorld()->GetGameState<AWpGameGameState>()->SelectionWidgetClass;
	ensureAlways(SelectionWidgetClass);
	SelectionWidgetComponent->SetWidgetClass(SelectionWidgetClass);
}

void FWpOrderReceiver::SetIsActive(const bool bInIsActive)
{
	bIsActive = bInIsActive;
}

bool FWpOrderReceiver::IsActive() const
{
	return bIsActive;
}

void FWpOrderReceiver::Tick()
{
	if (!bIsActive)
	{
		return;
	}

	if (WP_AS_ENDURANCE_CONST(OwnerEntityComponent).GetEntityState() != EWpEntityState::Operational)
	{
		return;
	}

	if (FWpOrderHandler* const OrderHandler = CurrentOrderHandler.Get())
	{
		if (!OrderHandler->IsRunning())
		{
			OrderHandler->Reactivate();
		}
		else if (OrderHandler->HasFinished())
		{
			OrderHandler->Deactivate();
			CurrentOrderHandler.Reset();
		}
		else
		{
			OrderHandler->Tick();
		}
	}

	if (!CurrentOrderHandler.Get())
	{
		FWpOrder Order;
		if (OrderQueue.Dequeue(Order))
		{
			ActivateOrder(Order);
		}
	}
}

bool FWpOrderReceiver::ReceiveOrder(const FWpOrder& Order, const bool bOverride)
{
	ensureAlways(bIsActive);

	if (WP_AS_ENDURANCE_CONST(OwnerEntityComponent).GetEntityState() == EWpEntityState::Wreck)
	{
		return false;
	}

	const bool bIsProxyOrder = bShouldProxyOrders && ProductionProxyOrders.Contains(Order.Type);

	if (Order.Type == EWpOrderType::Stop)
	{
		OrderQueue.Empty();

		if (FWpOrderHandler* const OrderHandler = CurrentOrderHandler.Get())
		{
			DeactivateOrder(OrderHandler);
		}

		if (!bIsProxyOrder)
		{
			OnOrderReceived.Broadcast(Order);
		}
		return true;
	}

	if (!SupportedOrders.Contains(Order.Type))
	{
		return false;
	}

	if (Order.TargetEntityComponent == OwnerEntityComponent)
	{
		if (!bIsProxyOrder)
		{
			OnOrderReceived.Broadcast(Order);
		}
		return false;
	}

	if (ImmediateOrders.Contains(Order.Type))
	{
		ExecuteOrder(Order.Type, Order);
	}
	else if (bIsProxyOrder)
	{
		ExecuteOrder(EWpOrderType::ProductionProxy, Order);
	}
	else
	{
		if (bOverride)
		{
			OrderQueue.Empty();
		}

		if (FWpOrderHandler* const OrderHandler = CurrentOrderHandler.Get())
		{
			if (bOverride)
			{
				if (Order.Type == OrderHandler->GetOrder().Type)
				{
					ReactivateOrder(OrderHandler, Order);
				}
				else
				{
					DeactivateOrder(OrderHandler);
					ActivateOrder(Order);
				}
			}
			else
			{
				OrderQueue.Enqueue(Order);
			}
		}
		else
		{
			if (OrderQueue.IsEmpty())
			{
				ActivateOrder(Order);
			}
			else
			{
				OrderQueue.Enqueue(Order);
			}
		}
	}

	if (!bIsProxyOrder)
	{
		OnOrderReceived.Broadcast(Order);
	}
	return true;
}

FWpOrder FWpOrderReceiver::GetOrder() const
{
	ensureAlways(bIsActive);

	if (const FWpOrderHandler* const OrderHandler = CurrentOrderHandler.Get())
	{
		return OrderHandler->GetOrder();
	}

	return {};
}

void FWpOrderReceiver::SetDefaultOrders(const FWpDefaultOrders& InDefaultOrders)
{
	ensureAlways(bIsActive);

	DefaultOrders = InDefaultOrders;
}

const FWpDefaultOrders& FWpOrderReceiver::GetDefaultOrders() const
{
	ensureAlways(bIsActive);

	return DefaultOrders;
}

void FWpOrderReceiver::SetSupportedOrders(const TSet<EWpOrderType>& InSupportedOrders)
{
	ensureAlways(bIsActive);

	SupportedOrders = InSupportedOrders;
}

TSet<EWpOrderType> FWpOrderReceiver::GetSupportedOrders() const
{
	ensureAlways(bIsActive);

	return SupportedOrders;
}

void FWpOrderReceiver::SetShouldProxyOrders(const bool bInShouldProxyOrders)
{
	ensureAlways(bIsActive);

	bShouldProxyOrders = bInShouldProxyOrders;
}

void FWpOrderReceiver::Select(const bool bSelected)
{
	ensureAlways(bIsActive);

	SelectionWidgetComponent->SetVisibility(bSelected);
}

void FWpOrderReceiver::ActivateOrder(const FWpOrder& Order)
{
	ensureAlways(bIsActive);

	FWpOrderHandler* const OrderHandler = CurrentOrderHandler.Set(Order.Type);
	OrderHandler->SetOwnerEntityComponent(OwnerEntityComponent);
	OrderHandler->SetOrder(Order);
	if (WP_AS_ENDURANCE_CONST(OwnerEntityComponent).GetEntityState() == EWpEntityState::Operational)
	{
		OrderHandler->Reactivate();
	}
}

void FWpOrderReceiver::ReactivateOrder(FWpOrderHandler* const OrderHandler, const FWpOrder& Order)
{
	ensureAlways(bIsActive);

	OrderHandler->SetOrder(Order);
	if (WP_AS_ENDURANCE_CONST(OwnerEntityComponent).GetEntityState() == EWpEntityState::Operational)
	{
		OrderHandler->Reactivate();
	}
}

void FWpOrderReceiver::DeactivateOrder(FWpOrderHandler* const OrderHandler)
{
	ensureAlways(bIsActive);

	if (OrderHandler->IsRunning())
	{
		OrderHandler->Deactivate();
	}
	CurrentOrderHandler.Reset();
}

void FWpOrderReceiver::ExecuteOrder(const EWpOrderType OrderType, const FWpOrder& Order)
{
	ensureAlways(bIsActive);

	FWpOrderHandlerWrapper ImmediateOrderHandler;
	FWpOrderHandler* const OrderHandler = ImmediateOrderHandler.Set(OrderType);
	OrderHandler->SetOwnerEntityComponent(OwnerEntityComponent);
	OrderHandler->Execute(Order);
}
