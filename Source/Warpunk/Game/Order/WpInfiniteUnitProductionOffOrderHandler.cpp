#include "WpInfiniteUnitProductionOffOrderHandler.h"

#include "Warpunk/Game/Entity/WpEntityInterface.h"
#include "Warpunk/Game/Production/WpProductionComponent.h"

void FWpInfiniteUnitProductionOffOrderHandler::Execute(const FWpOrder& InOrder)
{
	FWpOrderHandler::Execute(InOrder);

	GetEntity()->GetProductionComponent()->EnableInfiniteProduction(false);
}
