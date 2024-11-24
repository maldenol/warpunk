#include "WpSelfDestructOrderHandler.h"

#include "Warpunk/Game/Endurance/WpEnduranceComponent.h"
#include "Warpunk/Game/Entity/WpEntityInterface.h"

void FWpSelfDestructOrderHandler::Execute(const FWpOrder& InOrder)
{
	FWpOrderHandler::Execute(InOrder);

	GetEntity()->GetEnduranceComponent()->Destruct();
}
