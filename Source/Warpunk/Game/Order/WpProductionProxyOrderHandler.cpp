#include "WpProductionProxyOrderHandler.h"

#include "Warpunk/Game/EntityComponent/WpEntityComponent.h"
#include "Warpunk/Game/Production/WpProduction.h"

void FWpProductionProxyOrderHandler::Execute(const FWpOrder& InOrder)
{
	FWpOrderHandler::Execute(InOrder);

	FWpProduction& Production = WP_AS_PRODUCTION(GetOwnerEntityComponent());
	ensureAlways(Production.IsActive());
	Production.ReceiveProxyOrder(InOrder, InOrder.bOverride);
}
