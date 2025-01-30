#include "WpOrderHandler.h"

void FWpOrderHandler::Initialize(AWpEntity* const InEntity)
{
	Entity = InEntity;
}

void FWpOrderHandler::Execute(const FWpOrder& InOrder)
{
}

void FWpOrderHandler::Reactivate()
{
	bIsRunning = true;
	bHasFinished = false;
}

void FWpOrderHandler::Tick(const float CurrentTime)
{
	ensureAlways(ShouldTick(CurrentTime));

	LastTickTime = CurrentTime;
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

bool FWpOrderHandler::ShouldTick(const float CurrentTime) const
{
	return CurrentTime >= LastTickTime + TickCooldown;
}

void FWpOrderHandler::Finish()
{
	bHasFinished = true;
}

AWpEntity* FWpOrderHandler::GetEntity()
{
	return Entity;
}

const AWpEntity* FWpOrderHandler::GetEntity() const
{
	return Entity;
}

void FWpOrderHandler::SetTickCooldown(const float InTickCooldown)
{
	TickCooldown = InTickCooldown;
}
