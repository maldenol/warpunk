#include "WpOrderHandler.h"

void FWpOrderHandler::SetOwnerEntityComponent(UWpEntityComponent* const InOwnerEntityComponent)
{
	OwnerEntityComponent = InOwnerEntityComponent;
}

void FWpOrderHandler::Execute(const FWpOrder& InOrder)
{
}

void FWpOrderHandler::Reactivate()
{
	bIsRunning = true;
	bHasFinished = false;
}

void FWpOrderHandler::Tick()
{
}

void FWpOrderHandler::Deactivate()
{
	bIsRunning = false;
	bHasFinished = false;
}

void FWpOrderHandler::SetOrder(const FWpOrder& InOrder)
{
	Order = InOrder;
}

const FWpOrder& FWpOrderHandler::GetOrder() const
{
	return Order;
}

bool FWpOrderHandler::IsRunning() const
{
	return bIsRunning;
}

bool FWpOrderHandler::HasFinished() const
{
	return bHasFinished;
}

void FWpOrderHandler::Finish()
{
	bHasFinished = true;
}

UWpEntityComponent* FWpOrderHandler::GetOwnerEntityComponent()
{
	return OwnerEntityComponent;
}

const UWpEntityComponent* FWpOrderHandler::GetOwnerEntityComponent() const
{
	return OwnerEntityComponent;
}
