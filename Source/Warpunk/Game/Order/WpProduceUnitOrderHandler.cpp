#include "WpProduceUnitOrderHandler.h"

#include "Warpunk/Game/EntityComponent/WpEntityComponent.h"
#include "Warpunk/Game/Production/WpProduction.h"

void FWpProduceUnitOrderHandler::SetOwnerEntityComponent(UWpEntityComponent* const InOwnerEntityComponent)
{
	FWpOrderHandler::SetOwnerEntityComponent(InOwnerEntityComponent);

	Production = &WP_AS_PRODUCTION(GetOwnerEntityComponent());
	ensureAlways(Production->IsActive());
}

void FWpProduceUnitOrderHandler::Reactivate()
{
	FWpOrderHandler::Reactivate();

	if (GetOrder().UnitToProduceConfigIdNumbers.IsEmpty())
	{
		if (OnRepairFinishedDelegateHandle.IsValid())
		{
			Production->OnRepairFinished().Remove(OnRepairFinishedDelegateHandle);
			OnRepairFinishedDelegateHandle.Reset();
		}

		UWpEntityComponent* const CurrentTargetEntityComponent = Production->GetCurrentTarget();

		Production->Stop();

		if (CurrentTargetEntityComponent)
		{
			WP_AS_ENDURANCE(CurrentTargetEntityComponent).Destruct();
		}

		Finish();
		return;
	}

	TMap<uint32, uint32> UnitToProduceConfigIdNumbers = GetOrder().UnitToProduceConfigIdNumbers;
	TArray<uint32> UnitToProduceConfigIds;
	UnitToProduceConfigIdNumbers.GetKeys(UnitToProduceConfigIds);

	if (Production->GetInfiniteProduction())
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
		ProducedUnits.Empty();
	}

	UnitQueue.Empty();
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

	if (UWpEntityComponent* const CurrentTargetEntityComponent = Production->GetCurrentTarget())
	{
		const uint32 CurrentTargetEntityConfigId = CurrentTargetEntityComponent->GetEntityConfigId();
		if (!GetOrder().UnitToProduceConfigIdNumbers.Contains(CurrentTargetEntityConfigId))
		{
			if (OnRepairFinishedDelegateHandle.IsValid())
			{
				Production->OnRepairFinished().Remove(OnRepairFinishedDelegateHandle);
				OnRepairFinishedDelegateHandle.Reset();
			}

			Production->Stop();

			WP_AS_ENDURANCE(CurrentTargetEntityComponent).Destruct();

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

	if (OnRepairFinishedDelegateHandle.IsValid())
	{
		Production->OnRepairFinished().Remove(OnRepairFinishedDelegateHandle);
	}
	OnRepairFinishedDelegateHandle = Production->OnRepairFinished().AddRaw(this, &FWpProduceUnitOrderHandler::OnRepairFinished);
}

void FWpProduceUnitOrderHandler::Deactivate()
{
	UnitQueue.Empty();
	ProducedUnits.Empty();

	Production->OnRepairFinished().Remove(OnRepairFinishedDelegateHandle);
	OnRepairFinishedDelegateHandle.Reset();
	Production->Stop();

	FWpOrderHandler::Deactivate();
}

void FWpProduceUnitOrderHandler::BeginProduceUnit()
{
	static constexpr float OffsetZ = 100.0f;
	const FVector UnitLocation = GetOwnerEntityComponent()->GetOwner()->GetActorLocation() + FVector::UpVector * OffsetZ;
	Production->StartProduce(UnitQueue[UnitQueueIndex], UnitLocation);
}

void FWpProduceUnitOrderHandler::OnRepairFinished(UWpEntityComponent* const EntityComponent)
{
	const uint32 ProducedUnitConfigId = EntityComponent->GetEntityConfigId();
	if (Production->GetInfiniteProduction())
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

		--NewOrder.UnitToProduceConfigIdNumbers[ProducedUnitConfigId];
		if (NewOrder.UnitToProduceConfigIdNumbers[ProducedUnitConfigId] == 0)
		{
			NewOrder.UnitToProduceConfigIdNumbers.Remove(ProducedUnitConfigId);
		}

		SetOrder(NewOrder);
	}

	++UnitQueueIndex;
	if (UnitQueueIndex < UnitQueue.Num())
	{
		BeginProduceUnit();
	}
	else if (Production->GetInfiniteProduction())
	{
		ProducedUnits.Empty();
		Reactivate();
	}
	else
	{
		Finish();
	}
}
