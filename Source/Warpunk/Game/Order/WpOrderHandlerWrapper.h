#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Order/WpAssistOrderHandler.h"
#include "Warpunk/Game/Order/WpAttackOrderHandler.h"
#include "Warpunk/Game/Order/WpInfiniteUnitProductionOffOrderHandler.h"
#include "Warpunk/Game/Order/WpInfiniteUnitProductionOnOrderHandler.h"
#include "Warpunk/Game/Order/WpMoveOrderHandler.h"
#include "Warpunk/Game/Order/WpPatrolOrderHandler.h"
#include "Warpunk/Game/Order/WpProduceBuildingOrderHandler.h"
#include "Warpunk/Game/Order/WpProduceUnitOrderHandler.h"
#include "Warpunk/Game/Order/WpProductionProxyOrderHandler.h"
#include "Warpunk/Game/Order/WpReclaimOrderHandler.h"
#include "Warpunk/Game/Order/WpRepairOrderHandler.h"
#include "Warpunk/Game/Order/WpSelfDestructOrderHandler.h"
#include "Warpunk/Game/Order/WpStrikeOrderHandler.h"

class FWpOrderHandlerWrapper final
{
	using FWpOrderHandlerVariant = TVariant
	<
		std::monostate,
		FWpMoveOrderHandler,
		FWpPatrolOrderHandler,
		FWpAssistOrderHandler,
		FWpAttackOrderHandler,
		FWpStrikeOrderHandler,
		FWpProduceBuildingOrderHandler,
		FWpProduceUnitOrderHandler,
		FWpProductionProxyOrderHandler,
		FWpInfiniteUnitProductionOffOrderHandler,
		FWpInfiniteUnitProductionOnOrderHandler,
		FWpRepairOrderHandler,
		FWpReclaimOrderHandler,
		FWpSelfDestructOrderHandler
	>;

public:
	FWpOrderHandler* Set(const EWpOrderType InOrderType);
	FWpOrderHandler* Get();
	const FWpOrderHandler* Get() const;
	void Reset();

private:
	FWpOrderHandlerVariant OrderHandlerVariant = {};
	EWpOrderType OrderType = EWpOrderType::Undefined;
};
