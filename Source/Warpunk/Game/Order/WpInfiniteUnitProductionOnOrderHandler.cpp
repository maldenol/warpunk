#include "WpInfiniteUnitProductionOnOrderHandler.h"

#include "Warpunk/Game/Entity/WpEntityInterface.h"
#include "Warpunk/Game/Production/WpProductionComponent.h"

void FWpInfiniteUnitProductionOnOrderHandler::Execute(const FWpOrder& InOrder)
{
	FWpOrderHandler::Execute(InOrder);

	GetEntity()->GetProductionComponent()->EnableInfiniteProduction(true);
}
