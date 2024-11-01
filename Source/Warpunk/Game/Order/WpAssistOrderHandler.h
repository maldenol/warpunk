#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Order/WpOrderHandler.h"

class FWpOrderReceiver;

class AAIController;

class FWpAssistOrderHandler final : public FWpOrderHandler
{
public:
	virtual void SetOwnerEntityComponent(UWpEntityComponent* const InOwnerEntityComponent) override;

	virtual void Reactivate() override;
	virtual void Tick() override;
	virtual void Deactivate() override;

private:
	void OnOrderReceived(const FWpOrder& InOrder);

	AAIController* AIController = nullptr;

	const FWpOrderReceiver* OrderReceiver = nullptr;

	bool bCanAssist = false;

	TPimplPtr<FWpOrderReceiver> MimicOrderReceiver = nullptr;

	FDelegateHandle OnOrderReceivedDelegateHandle = {};
};
