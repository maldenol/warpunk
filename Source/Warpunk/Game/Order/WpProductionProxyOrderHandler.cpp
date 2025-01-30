#include "WpProductionProxyOrderHandler.h"

#include "Warpunk/Game/Entity/WpEntity.h"
#include "Warpunk/Game/Production/WpProductionComponent.h"

void FWpProductionProxyOrderHandler::Execute(const FWpOrder& InOrder)
{
	FWpOrderHandler::Execute(InOrder);

	UWpProductionComponent* const ProductionComponent = GetEntity()->GetProductionComponent();
	ensureAlways(ProductionComponent->IsActive());
	ProductionComponent->ReceiveProxyOrder(InOrder, InOrder.bOverride);
}
