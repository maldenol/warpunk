#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Order/WpOrderHandler.h"

class FWpInfiniteUnitProductionOffOrderHandler final : public FWpOrderHandler
{
public:
	virtual void Execute(const FWpOrder& InOrder) override;
};
