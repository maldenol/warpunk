#include "WpSelfDestructOrderHandler.h"

#include "Warpunk/Game/Endurance/WpEndurance.h"
#include "Warpunk/Game/EntityComponent/WpEntityComponent.h"

void FWpSelfDestructOrderHandler::Execute(const FWpOrder& InOrder)
{
	FWpOrderHandler::Execute(InOrder);

	FWpEndurance& Endurance = WP_AS_ENDURANCE(GetOwnerEntityComponent());
	Endurance.Destruct();
}
