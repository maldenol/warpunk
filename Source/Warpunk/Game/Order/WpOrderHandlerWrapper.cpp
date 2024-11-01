#include "WpOrderHandlerWrapper.h"

FWpOrderHandler* FWpOrderHandlerWrapper::Set(const EWpOrderType InOrderType)
{
	FWpOrderHandler* OrderHandler = nullptr;

	switch (InOrderType)
	{
	case EWpOrderType::Move:
		OrderHandlerVariant.Emplace<FWpMoveOrderHandler>();
		OrderHandler = &OrderHandlerVariant.Get<FWpMoveOrderHandler>();
		break;
	case EWpOrderType::Patrol:
		OrderHandlerVariant.Emplace<FWpPatrolOrderHandler>();
		OrderHandler = &OrderHandlerVariant.Get<FWpPatrolOrderHandler>();
		break;
	case EWpOrderType::Assist:
		OrderHandlerVariant.Emplace<FWpAssistOrderHandler>();
		OrderHandler = &OrderHandlerVariant.Get<FWpAssistOrderHandler>();
		break;
	case EWpOrderType::Attack:
		OrderHandlerVariant.Emplace<FWpAttackOrderHandler>();
		OrderHandler = &OrderHandlerVariant.Get<FWpAttackOrderHandler>();
		break;
	case EWpOrderType::Strike:
		OrderHandlerVariant.Emplace<FWpStrikeOrderHandler>();
		OrderHandler = &OrderHandlerVariant.Get<FWpStrikeOrderHandler>();
		break;
	case EWpOrderType::ProduceBuilding:
		OrderHandlerVariant.Emplace<FWpProduceBuildingOrderHandler>();
		OrderHandler = &OrderHandlerVariant.Get<FWpProduceBuildingOrderHandler>();
		break;
	case EWpOrderType::ProduceUnit:
		OrderHandlerVariant.Emplace<FWpProduceUnitOrderHandler>();
		OrderHandler = &OrderHandlerVariant.Get<FWpProduceUnitOrderHandler>();
		break;
	case EWpOrderType::ProductionProxy:
		OrderHandlerVariant.Emplace<FWpProductionProxyOrderHandler>();
		OrderHandler = &OrderHandlerVariant.Get<FWpProductionProxyOrderHandler>();
		break;
	case EWpOrderType::InfiniteUnitProductionOff:
		OrderHandlerVariant.Emplace<FWpInfiniteUnitProductionOffOrderHandler>();
		OrderHandler = &OrderHandlerVariant.Get<FWpInfiniteUnitProductionOffOrderHandler>();
		break;
	case EWpOrderType::InfiniteUnitProductionOn:
		OrderHandlerVariant.Emplace<FWpInfiniteUnitProductionOnOrderHandler>();
		OrderHandler = &OrderHandlerVariant.Get<FWpInfiniteUnitProductionOnOrderHandler>();
		break;
	case EWpOrderType::Repair:
		OrderHandlerVariant.Emplace<FWpRepairOrderHandler>();
		OrderHandler = &OrderHandlerVariant.Get<FWpRepairOrderHandler>();
		break;
	case EWpOrderType::Reclaim:
		OrderHandlerVariant.Emplace<FWpReclaimOrderHandler>();
		OrderHandler = &OrderHandlerVariant.Get<FWpReclaimOrderHandler>();
		break;
	case EWpOrderType::SelfDestruct:
		OrderHandlerVariant.Emplace<FWpSelfDestructOrderHandler>();
		OrderHandler = &OrderHandlerVariant.Get<FWpSelfDestructOrderHandler>();
		break;
	default:
		ensureAlways(false);
		OrderHandlerVariant.Emplace<std::monostate>();
		break;
	}

	OrderType = OrderHandler ? InOrderType : EWpOrderType::Undefined;

	return OrderHandler;
}

FWpOrderHandler* FWpOrderHandlerWrapper::Get()
{
	FWpOrderHandler* OrderHandler = nullptr;

	switch (OrderType)
	{
	case EWpOrderType::Move:
		OrderHandler = &OrderHandlerVariant.Get<FWpMoveOrderHandler>();
		break;
	case EWpOrderType::Patrol:
		OrderHandler = &OrderHandlerVariant.Get<FWpPatrolOrderHandler>();
		break;
	case EWpOrderType::Assist:
		OrderHandler = &OrderHandlerVariant.Get<FWpAssistOrderHandler>();
		break;
	case EWpOrderType::Attack:
		OrderHandler = &OrderHandlerVariant.Get<FWpAttackOrderHandler>();
		break;
	case EWpOrderType::Strike:
		OrderHandler = &OrderHandlerVariant.Get<FWpStrikeOrderHandler>();
		break;
	case EWpOrderType::ProduceBuilding:
		OrderHandler = &OrderHandlerVariant.Get<FWpProduceBuildingOrderHandler>();
		break;
	case EWpOrderType::ProduceUnit:
		OrderHandler = &OrderHandlerVariant.Get<FWpProduceUnitOrderHandler>();
		break;
	case EWpOrderType::ProductionProxy:
		OrderHandler = &OrderHandlerVariant.Get<FWpProductionProxyOrderHandler>();
		break;
	case EWpOrderType::InfiniteUnitProductionOff:
		OrderHandler = &OrderHandlerVariant.Get<FWpInfiniteUnitProductionOffOrderHandler>();
		break;
	case EWpOrderType::InfiniteUnitProductionOn:
		OrderHandler = &OrderHandlerVariant.Get<FWpInfiniteUnitProductionOnOrderHandler>();
		break;
	case EWpOrderType::Repair:
		OrderHandler = &OrderHandlerVariant.Get<FWpRepairOrderHandler>();
		break;
	case EWpOrderType::Reclaim:
		OrderHandler = &OrderHandlerVariant.Get<FWpReclaimOrderHandler>();
		break;
	case EWpOrderType::SelfDestruct:
		OrderHandler = &OrderHandlerVariant.Get<FWpSelfDestructOrderHandler>();
		break;
	default:
		break;
	}

	return OrderHandler;
}

const FWpOrderHandler* FWpOrderHandlerWrapper::Get() const
{
	return const_cast<FWpOrderHandlerWrapper*>(this)->Get();
}

void FWpOrderHandlerWrapper::Reset()
{
	OrderHandlerVariant.Emplace<std::monostate>();
	OrderType = EWpOrderType::Undefined;
}
