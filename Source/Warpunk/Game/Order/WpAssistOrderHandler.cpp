#include "WpAssistOrderHandler.h"

#include "Warpunk/Game/EntityComponent/WpEntityComponent.h"

#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"

void FWpAssistOrderHandler::SetOwnerEntityComponent(UWpEntityComponent* const InOwnerEntityComponent)
{
	FWpOrderHandler::SetOwnerEntityComponent(InOwnerEntityComponent);

	AIController = GetOwnerEntityComponent()->GetOwner<APawn>()->GetController<AAIController>();
	ensureAlways(AIController);

	OrderReceiver = &WP_AS_ORDER_RECEIVER_CONST(GetOwnerEntityComponent());
}

void FWpAssistOrderHandler::Reactivate()
{
	FWpOrderHandler::Reactivate();

	if (!IsValid(GetOrder().TargetEntityComponent))
	{
		Finish();
		return;
	}

	MimicOrderReceiver = MakePimpl<FWpOrderReceiver>();
	MimicOrderReceiver->Initialize(GetOwnerEntityComponent());
	MimicOrderReceiver->SetIsActive(true);
	MimicOrderReceiver->SetDefaultOrders(OrderReceiver->GetDefaultOrders());
	MimicOrderReceiver->SetSupportedOrders(OrderReceiver->GetSupportedOrders());
	MimicOrderReceiver->SetShouldProxyOrders(false);

	FWpOrderReceiver& TargetOrderReceiver = WP_AS_ORDER_RECEIVER(GetOrder().TargetEntityComponent);

	if (OnOrderReceivedDelegateHandle.IsValid())
	{
		TargetOrderReceiver.OnOrderReceived().Remove(OnOrderReceivedDelegateHandle);
	}
	OnOrderReceivedDelegateHandle = TargetOrderReceiver.OnOrderReceived().AddRaw(this, &FWpAssistOrderHandler::OnOrderReceived);

	OnOrderReceived(TargetOrderReceiver.GetOrder());
}

void FWpAssistOrderHandler::Tick()
{
	FWpOrderHandler::Tick();

	if (bCanAssist)
	{
		MimicOrderReceiver->Tick();
	}
	else
	{
		static constexpr float Radius = 300.0f;
		const float Distance2 = FVector::DistSquared2D(GetOwnerEntityComponent()->GetOwner()->GetActorLocation(), GetOrder().TargetEntityComponent->GetOwner()->GetActorLocation());
		const bool bIsInRange = Distance2 <= Radius * Radius;
		const bool bIsMoving = AIController->GetMoveStatus() != EPathFollowingStatus::Type::Idle;
		if (!bIsInRange && !bIsMoving)
		{
			AIController->MoveToActor(GetOrder().TargetEntityComponent->GetOwner(), Radius, false, true, false, nullptr, false);
		}
	}
}

void FWpAssistOrderHandler::Deactivate()
{
	if (IsValid(GetOrder().TargetEntityComponent))
	{
		WP_AS_ORDER_RECEIVER(GetOrder().TargetEntityComponent).OnOrderReceived().Remove(OnOrderReceivedDelegateHandle);
	}
	OnOrderReceivedDelegateHandle.Reset();
	MimicOrderReceiver = nullptr;

	if (IsValid(AIController))
	{
		AIController->StopMovement();
	}

	FWpOrderHandler::Deactivate();
}

void FWpAssistOrderHandler::OnOrderReceived(const FWpOrder& InOrder)
{
	if (!IsValid(GetOrder().TargetEntityComponent))
	{
		Finish();
		return;
	}

	if (!bCanAssist)
	{
		AIController->StopMovement();
		bCanAssist = true;
	}

	if (MimicOrderReceiver->ReceiveOrder(InOrder, true))
	{
		return;
	}

	FWpOrder RepairOrder;
	RepairOrder.Type = EWpOrderType::Repair;
	RepairOrder.TargetEntityComponent = GetOrder().TargetEntityComponent;
	if (MimicOrderReceiver->ReceiveOrder(RepairOrder, true))
	{
		return;
	}

	bCanAssist = false;
}
