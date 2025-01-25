#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Order/WpOrderHandler.h"

class FWpAssistOrderHandler final : public FWpOrderHandler
{
public:
	virtual void Execute(const FWpOrder& InOrder) override;
};
