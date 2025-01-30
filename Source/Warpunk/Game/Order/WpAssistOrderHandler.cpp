#include "WpAssistOrderHandler.h"

#include "Warpunk/Game/Endurance/WpEnduranceComponent.h"
#include "Warpunk/Game/Entity/WpEntity.h"
#include "Warpunk/Game/Order/WpOrderReceiverComponent.h"
#include "Warpunk/Game/Production/WpProductionComponent.h"

void FWpAssistOrderHandler::Execute(const FWpOrder& InOrder)
{
	FWpOrderHandler::Execute(InOrder);

	const AWpEntity* const TargetEntity = InOrder.TargetEntity;
	if (!TargetEntity)
	{
		return;
	}

	UWpOrderReceiverComponent* const OrderReceiverComponent = GetEntity()->GetOrderReceiverComponent();

	const UWpOrderReceiverComponent* const TargetOrderReceiverComponent = TargetEntity->GetOrderReceiverComponent();
	const UWpEnduranceComponent* const TargetEnduranceComponent = TargetEntity->GetEnduranceComponent();
	const bool bCanAssistOrder = TargetOrderReceiverComponent->IsActive() && (TargetEnduranceComponent->GetEntityState() == EWpEntityState::Operational);
	if (bCanAssistOrder)
	{
		const FWpOrder TargetOrder = TargetOrderReceiverComponent->GetOrder();

		const bool bCanAssistAttack = GetEntity()->GetWeaponComponent()->IsActive()
			&& (TargetOrder.Type == EWpOrderType::Attack);

		const bool bCanAssistProduction = GetEntity()->GetProductionComponent()->IsActive()
			&& ((TargetOrder.Type == EWpOrderType::ProduceBuilding)
				|| (TargetOrder.Type == EWpOrderType::ProduceUnit)
				|| (TargetOrder.Type == EWpOrderType::Repair)
				|| (TargetOrder.Type == EWpOrderType::Reclaim));

		const bool bCanAssistPatrol = OrderReceiverComponent->GetSupportedOrders().Contains(EWpOrderType::Patrol)
			&& (TargetOrder.Type == EWpOrderType::Patrol);

		if (bCanAssistAttack || bCanAssistProduction || bCanAssistPatrol)
		{
			OrderReceiverComponent->ReceiveOrder(TargetOrder, true);
			return;
		}
	}

	const bool bCanRepair = GetEntity()->GetProductionComponent()->IsActive()
		&& GetEntity()->GetProductionComponent()->CanRepair(TargetEntity);
	if (bCanRepair)
	{
		FWpOrder RepairOrder;
		RepairOrder.Type = EWpOrderType::Repair;
		RepairOrder.TargetEntity = InOrder.TargetEntity;
		OrderReceiverComponent->ReceiveOrder(RepairOrder, true);
		return;
	}

	if (OrderReceiverComponent->GetSupportedOrders().Contains(EWpOrderType::Move))
	{
		FWpOrder MoveOrder;
		MoveOrder.Type = EWpOrderType::Move;
		MoveOrder.TargetLocation = InOrder.TargetEntity->GetActorLocation();
		OrderReceiverComponent->ReceiveOrder(MoveOrder, true);
		return;
	}

	ensureAlways(false);
}
