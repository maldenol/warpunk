#include "WpProductionProxyOrderHandler.h"

#include "Warpunk/Game/Entity/WpEntityInterface.h"
#include "Warpunk/Game/Production/WpProductionComponent.h"

void FWpProductionProxyOrderHandler::Execute(const FWpOrder& InOrder)
{
	FWpOrderHandler::Execute(InOrder);

	GetEntity()->GetProductionComponent()->ReceiveProxyOrder(InOrder, InOrder.bOverride);
}
