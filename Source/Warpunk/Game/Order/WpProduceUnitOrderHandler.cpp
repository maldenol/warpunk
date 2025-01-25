#include "WpProduceUnitOrderHandler.h"

#include "Warpunk/Game/Endurance/WpEnduranceComponent.h"
#include "Warpunk/Game/Entity/WpEntityInterface.h"
#include "Warpunk/Game/Order/WpOrderReceiverComponent.h"
#include "Warpunk/Game/Production/WpProductionComponent.h"

void FWpProduceUnitOrderHandler::Initialize(IWpEntityInterface* const InEntity)
{
	FWpOrderHandler::Initialize(InEntity);

	ProductionComponent = GetEntity()->GetProductionComponent();
	ensureAlways(ProductionComponent->IsActive());
}

void FWpProduceUnitOrderHandler::Reactivate()
{
	FWpOrderHandler::Reactivate();

	if (OnRepairFinishedDelegateHandle.IsValid())
	{
		ProductionComponent->OnRepairFinished().Remove(OnRepairFinishedDelegateHandle);
	}

	if (GetOrder().UnitToProduceConfigIdNumbers.IsEmpty())
	{
		IWpEntityInterface* const CurrentTargetEntity = ProductionComponent->GetCurrentTargetEntity();

		ProductionComponent->Stop();

		if (CurrentTargetEntity)
		{
			CurrentTargetEntity->GetEnduranceComponent()->Destruct();
		}

		Finish();
		return;
	}

	TMap<uint32, uint32> UnitToProduceConfigIdNumbers = FWpConfigIdNumber::ToMap(GetOrder().UnitToProduceConfigIdNumbers);
	TArray<uint32> UnitToProduceConfigIds;
	UnitToProduceConfigIdNumbers.GetKeys(UnitToProduceConfigIds);

	if (ProductionComponent->IsInfiniteProductionEnabled())
	{
		for (const uint32 UnitToProduceConfigId : UnitToProduceConfigIds)
		{
			if (const uint32* const ProducedUnitNumber = ProducedUnits.Find(UnitToProduceConfigId))
			{
				UnitToProduceConfigIdNumbers[UnitToProduceConfigId] -= *ProducedUnitNumber;
				if (UnitToProduceConfigIdNumbers[UnitToProduceConfigId] <= 0)
				{
					UnitToProduceConfigIdNumbers.Remove(UnitToProduceConfigId);
				}
			}
		}
		ProducedUnits.Empty(ProducedUnits.Num());
	}

	UnitQueue.Empty(UnitQueue.Num());
	while (!UnitToProduceConfigIdNumbers.IsEmpty())
	{
		UnitToProduceConfigIdNumbers.GetKeys(UnitToProduceConfigIds);
		for (const uint32 UnitToProduceConfigId : UnitToProduceConfigIds)
		{
			UnitQueue.Add(UnitToProduceConfigId);

			--UnitToProduceConfigIdNumbers[UnitToProduceConfigId];
			if (UnitToProduceConfigIdNumbers[UnitToProduceConfigId] == 0)
			{
				UnitToProduceConfigIdNumbers.Remove(UnitToProduceConfigId);
			}
		}
	}
	UnitQueueIndex = 0;

	if (IWpEntityInterface* const CurrentTargetEntity = ProductionComponent->GetCurrentTargetEntity())
	{
		const uint32 CurrentTargetEntityConfigId = CurrentTargetEntity->GetConfigId();
		if (!GetOrder().UnitToProduceConfigIdNumbers.Contains(CurrentTargetEntityConfigId))
		{
			ProductionComponent->Stop();

			CurrentTargetEntity->GetEnduranceComponent()->Destruct();

			BeginProduceUnit();
		}
		else
		{
			UnitQueue.RemoveSingle(CurrentTargetEntityConfigId);
			UnitQueue.Insert(CurrentTargetEntityConfigId, 0);
		}
	}
	else
	{
		BeginProduceUnit();
	}

	bWasInfiniteProductionEnabled = ProductionComponent->IsInfiniteProductionEnabled();

	OnRepairFinishedDelegateHandle = ProductionComponent->OnRepairFinished().AddRaw(this, &FWpProduceUnitOrderHandler::OnRepairFinished);
}

void FWpProduceUnitOrderHandler::Deactivate()
{
	UnitQueue.Empty(UnitQueue.Num());
	ProducedUnits.Empty(ProducedUnits.Num());

	ProductionComponent->OnRepairFinished().Remove(OnRepairFinishedDelegateHandle);
	OnRepairFinishedDelegateHandle.Reset();
	ProductionComponent->Stop();

	FWpOrderHandler::Deactivate();
}

void FWpProduceUnitOrderHandler::SetOrder(const FWpOrder& InOrder)
{
	FWpOrderHandler::SetOrder(InOrder);

	GetEntity()->GetOrderReceiverComponent()->SetUnitToProduceConfigIdNumbers(GetOrder().UnitToProduceConfigIdNumbers);
}

void FWpProduceUnitOrderHandler::BeginProduceUnit()
{
	static constexpr float OffsetZ = 100.0f;
	const FVector UnitLocation = Cast<AActor>(GetEntity())->GetActorLocation() + FVector::UpVector * OffsetZ;
	ProductionComponent->StartProduce(UnitQueue[UnitQueueIndex], UnitLocation);
}

void FWpProduceUnitOrderHandler::OnRepairFinished(IWpEntityInterface* const InEntity)
{
	const uint32 ProducedUnitConfigId = InEntity->GetConfigId();
	if (ProductionComponent->IsInfiniteProductionEnabled())
	{
		if (uint32* const ProducedUnitNumber = ProducedUnits.Find(ProducedUnitConfigId))
		{
			*ProducedUnitNumber = *ProducedUnitNumber + 1;
		}
		else
		{
			ProducedUnits.Add(ProducedUnitConfigId, 1);
		}
	}
	else
	{
		FWpOrder NewOrder = GetOrder();

		FWpConfigIdNumber* const UnitToProduceConfigIdNumber = NewOrder.UnitToProduceConfigIdNumbers.FindByPredicate(
			[ProducedUnitConfigId](const FWpConfigIdNumber& ConfigIdNumber)
			{
				return ConfigIdNumber.ConfigId == ProducedUnitConfigId;
			});
		uint32& UnitToProduceNumber = UnitToProduceConfigIdNumber->Number;

		--UnitToProduceNumber;
		if (UnitToProduceNumber == 0)
		{
			NewOrder.UnitToProduceConfigIdNumbers.RemoveSingle(*UnitToProduceConfigIdNumber);
		}

		SetOrder(NewOrder);

		GetEntity()->GetOrderReceiverComponent()->SetUnitToProduceConfigIdNumbers(GetOrder().UnitToProduceConfigIdNumbers);
	}

	if (bWasInfiniteProductionEnabled && !ProductionComponent->IsInfiniteProductionEnabled())
	{
		ProducedUnits.Empty(ProducedUnits.Num());
		Reactivate();
		return;
	}

	++UnitQueueIndex;
	if (UnitQueueIndex < UnitQueue.Num())
	{
		BeginProduceUnit();
	}
	else if (ProductionComponent->IsInfiniteProductionEnabled())
	{
		ProducedUnits.Empty(ProducedUnits.Num());
		Reactivate();
	}
	else
	{
		Finish();
	}
}
