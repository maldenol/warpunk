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

	if (GetOrder().UnitToProduceConfigIDNumbers.IsEmpty())
	{
		if (OnRepairFinishedDelegateHandle.IsValid())
		{
			Production->OnRepairFinished.Remove(OnRepairFinishedDelegateHandle);
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

	TMap<uint32, uint32> UnitToProduceConfigIDNumbers = GetOrder().UnitToProduceConfigIDNumbers;
	TArray<uint32> UnitToProduceConfigIDs;
	UnitToProduceConfigIDNumbers.GetKeys(UnitToProduceConfigIDs);

	if (Production->GetInfiniteProduction())
	{
		for (const uint32 UnitToProduceConfigID : UnitToProduceConfigIDs)
		{
			if (const uint32* const ProducedUnitNumber = ProducedUnits.Find(UnitToProduceConfigID))
			{
				UnitToProduceConfigIDNumbers[UnitToProduceConfigID] -= *ProducedUnitNumber;
				if (UnitToProduceConfigIDNumbers[UnitToProduceConfigID] <= 0)
				{
					UnitToProduceConfigIDNumbers.Remove(UnitToProduceConfigID);
				}
			}
		}
		ProducedUnits.Empty();
	}

	UnitQueue.Empty();
	while (!UnitToProduceConfigIDNumbers.IsEmpty())
	{
		UnitToProduceConfigIDNumbers.GetKeys(UnitToProduceConfigIDs);
		for (const uint32 UnitToProduceConfigID : UnitToProduceConfigIDs)
		{
			UnitQueue.Add(UnitToProduceConfigID);

			--UnitToProduceConfigIDNumbers[UnitToProduceConfigID];
			if (UnitToProduceConfigIDNumbers[UnitToProduceConfigID] == 0)
			{
				UnitToProduceConfigIDNumbers.Remove(UnitToProduceConfigID);
			}
		}
	}
	UnitQueueIndex = 0;

	if (UWpEntityComponent* const CurrentTargetEntityComponent = Production->GetCurrentTarget())
	{
		const uint32 CurrentTargetEntityConfigID = CurrentTargetEntityComponent->GetEntityConfigID();
		if (!GetOrder().UnitToProduceConfigIDNumbers.Contains(CurrentTargetEntityConfigID))
		{
			if (OnRepairFinishedDelegateHandle.IsValid())
			{
				Production->OnRepairFinished.Remove(OnRepairFinishedDelegateHandle);
				OnRepairFinishedDelegateHandle.Reset();
			}

			Production->Stop();

			WP_AS_ENDURANCE(CurrentTargetEntityComponent).Destruct();

			BeginProduceUnit();
		}
		else
		{
			UnitQueue.RemoveSingle(CurrentTargetEntityConfigID);
			UnitQueue.Insert(CurrentTargetEntityConfigID, 0);
		}
	}
	else
	{
		BeginProduceUnit();
	}

	if (OnRepairFinishedDelegateHandle.IsValid())
	{
		Production->OnRepairFinished.Remove(OnRepairFinishedDelegateHandle);
	}
	OnRepairFinishedDelegateHandle = Production->OnRepairFinished.AddRaw(this, &FWpProduceUnitOrderHandler::OnRepairFinished);
}

void FWpProduceUnitOrderHandler::Deactivate()
{
	UnitQueue.Empty();
	ProducedUnits.Empty();

	Production->OnRepairFinished.Remove(OnRepairFinishedDelegateHandle);
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
	const uint32 ProducedUnitConfigID = EntityComponent->GetEntityConfigID();
	if (Production->GetInfiniteProduction())
	{
		if (uint32* const ProducedUnitNumber = ProducedUnits.Find(ProducedUnitConfigID))
		{
			*ProducedUnitNumber = *ProducedUnitNumber + 1;
		}
		else
		{
			ProducedUnits.Add(ProducedUnitConfigID, 1);
		}
	}
	else
	{
		FWpOrder NewOrder = GetOrder();

		--NewOrder.UnitToProduceConfigIDNumbers[ProducedUnitConfigID];
		if (NewOrder.UnitToProduceConfigIDNumbers[ProducedUnitConfigID] == 0)
		{
			NewOrder.UnitToProduceConfigIDNumbers.Remove(ProducedUnitConfigID);
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
