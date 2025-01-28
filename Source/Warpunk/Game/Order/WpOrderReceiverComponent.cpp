#include "WpOrderReceiverComponent.h"

#include "Warpunk/Game/Endurance/WpEnduranceComponent.h"
#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Entity/WpEntityInterface.h"
#include "Warpunk/Game/Entity/WpPooledEntityData.h"
#include "Warpunk/Global/GameInstance/WpGameInstance.h"

#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"

const FName UWpOrderReceiverComponent::SelectionWidgetComponentName = TEXT("SelectionWidget");

const TSet<EWpOrderType> UWpOrderReceiverComponent::ImmediateOrders =
{
	EWpOrderType::Assist,
	EWpOrderType::InfiniteUnitProductionOff,
	EWpOrderType::InfiniteUnitProductionOn,
	EWpOrderType::SelfDestruct,
};

const TSet<EWpOrderType> UWpOrderReceiverComponent::ProductionProxyOrders =
{
	EWpOrderType::Move,
	EWpOrderType::Patrol,
	EWpOrderType::Assist,
	EWpOrderType::Attack,
	EWpOrderType::Repair,
	EWpOrderType::Reclaim,
};

UWpOrderReceiverComponent::UWpOrderReceiverComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.SetTickFunctionEnable(false);

	SetIsReplicatedByDefault(true);
}

void UWpOrderReceiverComponent::BeginPlay()
{
	Super::BeginPlay();

	CreateSelectionWidgetComponentIfNull();
}

void UWpOrderReceiverComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* const ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	if (GetOwner<IWpEntityInterface>()->GetEnduranceComponent()->GetEntityState() != EWpEntityState::Operational)
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
			const float CurrentTime = GetWorld()->GetTimeSeconds();
			if (OrderHandler->ShouldTick(CurrentTime))
			{
				OrderHandler->Tick(CurrentTime);
			}
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

void UWpOrderReceiverComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UWpOrderReceiverComponent, DefaultOrders);
	DOREPLIFETIME(UWpOrderReceiverComponent, SupportedOrders);
	DOREPLIFETIME(UWpOrderReceiverComponent, UnitToProduceConfigIdNumbers);
}

void UWpOrderReceiverComponent::OnRep_IsActive()
{
	// Override to not enable tick
}

void UWpOrderReceiverComponent::OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
{
	ensureAlways(GetOwner()->HasAuthority());

	CreateSelectionWidgetComponentIfNull();

	if (bIsAcquired)
	{
		const FWpPooledEntityData& PooledEntityData = static_cast<const FWpPooledEntityData&>(PooledObjectData);
		const FWpOrderReceiverConfig* const OrderReceiverConfig = PooledEntityData.EntityConfig->OrderReceiverConfig.GetPtrOrNull();

		if (OrderReceiverConfig)
		{
			ensureAlways(!OrderReceiverConfig->SupportedOrders.IsEmpty());

			DefaultOrders = OrderReceiverConfig->DefaultOrders;
			SupportedOrders = OrderReceiverConfig->SupportedOrders.Array();
			bShouldProxyOrders = OrderReceiverConfig->bShouldProxyOrders;
		}

		bIsAcquired = (OrderReceiverConfig != nullptr);
	}

	if (!bIsAcquired)
	{
		if (FWpOrderHandler* const OrderHandler = CurrentOrderHandler.Get())
		{
			DeactivateOrder(OrderHandler);
		}

		OrderQueue.Empty();

		Select(false);
	}

	SelectionWidgetComponent->SetVisibility(false);

	SetActive(bIsAcquired);
	SetComponentTickEnabled(bIsAcquired);
}

void UWpOrderReceiverComponent::ReceiveOrder(const FWpOrder& Order, const bool bOverride)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	if (GetOwner<IWpEntityInterface>()->GetEnduranceComponent()->GetEntityState() == EWpEntityState::Wreck)
	{
		return;
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
			OrderReceivedEvent.Broadcast(Order);
		}
		return;
	}

	if (!SupportedOrders.Contains(Order.Type))
	{
		return;
	}

	if (Order.TargetEntityActor == GetOwner())
	{
		if (!bIsProxyOrder)
		{
			OrderReceivedEvent.Broadcast(Order);
		}
		return;
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

		OrderReceivedEvent.Broadcast(Order);
	}
}

FWpOrder UWpOrderReceiverComponent::GetOrder() const
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	if (const FWpOrderHandler* const OrderHandler = CurrentOrderHandler.Get())
	{
		return OrderHandler->GetOrder();
	}

	return {};
}

void UWpOrderReceiverComponent::SetDefaultOrders(const FWpDefaultOrders& InDefaultOrders)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	DefaultOrders = InDefaultOrders;
}

const FWpDefaultOrders& UWpOrderReceiverComponent::GetDefaultOrders() const
{
	ensureAlways(IsActive());

	return DefaultOrders;
}

void UWpOrderReceiverComponent::SetSupportedOrders(const TArray<EWpOrderType>& InSupportedOrders)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	SupportedOrders = InSupportedOrders;
}

const TArray<EWpOrderType>& UWpOrderReceiverComponent::GetSupportedOrders() const
{
	ensureAlways(IsActive());

	return SupportedOrders;
}

void UWpOrderReceiverComponent::SetShouldProxyOrders(const bool bInShouldProxyOrders)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	bShouldProxyOrders = bInShouldProxyOrders;
}

void UWpOrderReceiverComponent::Select(const bool bSelected)
{
	if (bSelected)
	{
		ensureAlways(IsActive());
	}

	SelectionWidgetComponent->SetVisibility(bSelected);
}

void UWpOrderReceiverComponent::SetUnitToProduceConfigIdNumbers(const TArray<FWpConfigIdNumber>& InUnitToProduceConfigIdNumbers)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	UnitToProduceConfigIdNumbers = InUnitToProduceConfigIdNumbers;
}

const TArray<FWpConfigIdNumber>& UWpOrderReceiverComponent::GetUnitToProduceConfigIdNumbers() const
{
	ensureAlways(IsActive());

	return UnitToProduceConfigIdNumbers;
}

UWpOrderReceiverComponent::FOrderReceivedEvent& UWpOrderReceiverComponent::OnOrderReceived()
{
	ensureAlways(GetOwner()->HasAuthority());

	return OrderReceivedEvent;
}

void UWpOrderReceiverComponent::ActivateOrder(const FWpOrder& Order)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	FWpOrderHandler* const OrderHandler = CurrentOrderHandler.Set(Order.Type);
	OrderHandler->Initialize(GetOwner<IWpEntityInterface>());
	OrderHandler->SetOrder(Order);
	if (GetOwner<IWpEntityInterface>()->GetEnduranceComponent()->GetEntityState() == EWpEntityState::Operational)
	{
		OrderHandler->Reactivate();
	}
}

void UWpOrderReceiverComponent::ReactivateOrder(FWpOrderHandler* const OrderHandler, const FWpOrder& Order)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	OrderHandler->SetOrder(Order);
	if (GetOwner<IWpEntityInterface>()->GetEnduranceComponent()->GetEntityState() == EWpEntityState::Operational)
	{
		OrderHandler->Reactivate();
	}
}

void UWpOrderReceiverComponent::DeactivateOrder(FWpOrderHandler* const OrderHandler)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	if (OrderHandler->IsRunning())
	{
		OrderHandler->Deactivate();
	}
	CurrentOrderHandler.Reset();
}

void UWpOrderReceiverComponent::ExecuteOrder(const EWpOrderType OrderType, const FWpOrder& Order)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	FWpOrderHandlerWrapper ImmediateOrderHandler;
	FWpOrderHandler* const OrderHandler = ImmediateOrderHandler.Set(OrderType);
	OrderHandler->Initialize(GetOwner<IWpEntityInterface>());
	OrderHandler->Execute(Order);
}

void UWpOrderReceiverComponent::CreateSelectionWidgetComponentIfNull()
{
	if (SelectionWidgetComponent)
	{
		return;
	}

	SelectionWidgetComponent = NewObject<UWidgetComponent>(GetOwner(), UWidgetComponent::StaticClass(), SelectionWidgetComponentName);
	SelectionWidgetComponent->RegisterComponent();
	SelectionWidgetComponent->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules{EAttachmentRule::KeepRelative, false});

	SelectionWidgetComponent->SetCollisionProfileName(TEXT("NoCollision"));
	SelectionWidgetComponent->SetNotifyRigidBodyCollision(false);
	SelectionWidgetComponent->SetGenerateOverlapEvents(false);

	SelectionWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	SelectionWidgetComponent->SetDrawAtDesiredSize(true);

	const TSubclassOf<UUserWidget>& SelectionWidgetClass = GetWorld()->GetGameInstance<UWpGameInstance>()->SelectionWidgetClass;
	ensureAlways(SelectionWidgetClass);
	SelectionWidgetComponent->SetWidgetClass(SelectionWidgetClass);

	SelectionWidgetComponent->SetVisibility(false);
}
