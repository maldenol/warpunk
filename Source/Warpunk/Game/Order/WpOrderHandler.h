#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Order/WpOrder.h"

class AWpEntity;

class FWpOrderHandler
{
public:
	virtual ~FWpOrderHandler()
	{
	}

	virtual void Initialize(AWpEntity* const InEntity);

	virtual void Execute(const FWpOrder& InOrder);

	virtual void Reactivate();
	virtual void Tick(const float CurrentTime);
	virtual void Deactivate();

	virtual void SetOrder(const FWpOrder& InOrder);
	const FWpOrder& GetOrder() const;

	bool IsRunning() const;
	bool HasFinished() const;

	bool ShouldTick(const float CurrentTime) const;

protected:
	void Finish();

	AWpEntity* GetEntity();
	const AWpEntity* GetEntity() const;

	void SetTickCooldown(const float InTickCooldown);

private:
	AWpEntity* Entity = nullptr;

	FWpOrder Order = {};

	bool bIsRunning = false;
	bool bHasFinished = false;

	float LastTickTime = 0.0f;
	float TickCooldown = 0.0f;
};
