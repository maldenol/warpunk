#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Order/WpOrder.h"

class UWpEntityComponent;

class FWpOrderHandler
{
public:
	virtual ~FWpOrderHandler()
	{
	}

	virtual void SetOwnerEntityComponent(UWpEntityComponent* const InOwnerEntityComponent);

	virtual void Execute(const FWpOrder& InOrder);

	virtual void Reactivate();
	virtual void Tick();
	virtual void Deactivate();

	void SetOrder(const FWpOrder& InOrder);
	const FWpOrder& GetOrder() const;

	bool IsRunning() const;
	bool HasFinished() const;

protected:
	void Finish();

	UWpEntityComponent* GetOwnerEntityComponent();
	const UWpEntityComponent* GetOwnerEntityComponent() const;

private:
	UWpEntityComponent* OwnerEntityComponent = nullptr;

	FWpOrder Order = {};

	bool bIsRunning = false;
	bool bHasFinished = false;
};
