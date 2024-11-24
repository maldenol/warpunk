#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Order/WpOrderHandler.h"

class UWpOrderReceiverComponent;

class AAIController;

class FWpAssistOrderHandler final : public FWpOrderHandler
{
public:
	virtual void Initialize(IWpEntityInterface* const InEntity) override;

	virtual void Reactivate() override;
	virtual void Tick() override;
	virtual void Deactivate() override;

private:
	void OnOrderReceived(const FWpOrder& InOrder);

	AAIController* AIController = nullptr;

	const UWpOrderReceiverComponent* OrderReceiverComponent = nullptr;
	bool bCanAssist = false;
	UWpOrderReceiverComponent* MimicOrderReceiverComponent = nullptr;

	FDelegateHandle OnOrderReceivedDelegateHandle = {};

	static const FName MimicOrderReceiverComponentName;
};
