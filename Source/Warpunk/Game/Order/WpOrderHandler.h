#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Order/WpOrder.h"

class IWpEntityInterface;

class FWpOrderHandler
{
public:
	virtual ~FWpOrderHandler()
	{
	}

	virtual void Initialize(IWpEntityInterface* const InEntity);

	virtual void Execute(const FWpOrder& InOrder);

	virtual void Reactivate();
	virtual void Tick();
	virtual void Deactivate();

	virtual void SetOrder(const FWpOrder& InOrder);
	const FWpOrder& GetOrder() const;

	bool IsRunning() const;
	bool HasFinished() const;

protected:
	void Finish();

	IWpEntityInterface* GetEntity();
	const IWpEntityInterface* GetEntity() const;

private:
	IWpEntityInterface* Entity = nullptr;

	FWpOrder Order = {};

	bool bIsRunning = false;
	bool bHasFinished = false;
};
