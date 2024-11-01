#include "WpInfiniteUnitProductionOffOrderHandler.h"

#include "Warpunk/Game/EntityComponent/WpEntityComponent.h"
#include "Warpunk/Game/Production/WpProduction.h"

void FWpInfiniteUnitProductionOffOrderHandler::Execute(const FWpOrder& InOrder)
{
	FWpOrderHandler::Execute(InOrder);

	FWpProduction& Production = WP_AS_PRODUCTION(GetOwnerEntityComponent());
	ensureAlways(Production.IsActive());
	Production.SetInfiniteProduction(false);
}
