#include "WpInfiniteUnitProductionOnOrderHandler.h"

#include "Warpunk/Game/Entity/WpEntity.h"
#include "Warpunk/Game/Production/WpProductionComponent.h"

void FWpInfiniteUnitProductionOnOrderHandler::Execute(const FWpOrder& InOrder)
{
	FWpOrderHandler::Execute(InOrder);

	UWpProductionComponent* const ProductionComponent = GetEntity()->GetProductionComponent();
	ensureAlways(ProductionComponent->IsActive());
	ProductionComponent->EnableInfiniteProduction(true);
}
