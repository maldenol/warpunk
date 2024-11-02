#include "WpOrderSender.h"

#include "Warpunk/Game/Data/WpEntityConfig.h"
#include "Warpunk/Game/Data/WpPooledEntityData.h"
#include "Warpunk/Game/Entity/WpBuilding.h"
#include "Warpunk/Game/Entity/WpUnit.h"
#include "Warpunk/Game/EntityComponent/WpEntityComponent.h"
#include "Warpunk/Game/EntityComponent/WpEntityInterface.h"
#include "Warpunk/Game/GameMode/WpGameGameState.h"

#include "Kismet/KismetSystemLibrary.h"

const FName FWpOrderSender::OverrideInputKeyName = TEXT("LeftShift");

void FWpOrderSender::Initialize(APlayerController* InPlayerController)
{
	PlayerController = InPlayerController;

	// const AWpGameGameState* const GameGameState = PlayerController->GetWorld()->GetGameState<AWpGameGameState>();
	BelligerentID = 0; // GameGameState->GetPlayerBelligerentID(PlayerController); TODO
}

void FWpOrderSender::Tick()
{
	if (OrderSenderState == EWpOrderSenderState::ProduceBuildingOrder)
	{
		UpdateBuildingPlan();
	}
}

void FWpOrderSender::SelectOrderType(const EWpOrderType OrderType)
{
	switch (OrderType)
	{
	case EWpOrderType::Stop:
	case EWpOrderType::InfiniteUnitProductionOff:
	case EWpOrderType::InfiniteUnitProductionOn:
		SendOrders(OrderType);
		break;
	default:
		OrderSenderState = EWpOrderSenderState::SelectedOrder;
		SelectedOrderType = OrderType;
		break;
	}
}

void FWpOrderSender::SelectBuildingToProduce(const uint32 BuildingConfigID)
{
	if (IsValid(BuildingToProduce))
	{
		BuildingToProduce->Destroy();
		BuildingToProduce = nullptr;
	}

	if (BuildingConfigID > 0)
	{
		OrderSenderState = EWpOrderSenderState::ProduceBuildingOrder;

		BuildingToProduceConfigID = BuildingConfigID;
	}
	else
	{
		OrderSenderState = EWpOrderSenderState::DefaultOrder;
	}
}

void FWpOrderSender::AddUnitToProduce(const uint32 UnitConfigID)
{
	ensureAlways(UnitConfigID > 0);

	const TArray<UWpEntityComponent*> SelectedEntities = GetSelectedEntities(nullptr);
	if (!SelectedEntities.IsEmpty())
	{
		PopulateUnitToProduceClassNumbersPerProducer(SelectedEntities);

		for (int32 Index = 0; Index < SelectedEntities.Num(); ++Index)
		{
			const UWpEntityComponent* const SelectedEntityComponent = SelectedEntities[Index];
			if (IsValid(SelectedEntityComponent))
			{
				if (uint32* const UnitToProduceNumber = UnitToProduceConfigIDNumbersPerProducer[Index].Find(UnitConfigID))
				{
					*UnitToProduceNumber = *UnitToProduceNumber + 1;
				}
				else
				{
					UnitToProduceConfigIDNumbersPerProducer[Index].Add(UnitConfigID, 1);
				}
			}
		}

		OrderSenderState = EWpOrderSenderState::ProduceUnitOrder;

		SendOrders(EWpOrderType::ProduceUnit);

		OrderSenderState = EWpOrderSenderState::DefaultOrder;
	}
}

void FWpOrderSender::RemoveUnitToProduce(const uint32 UnitConfigID)
{
	ensureAlways(UnitConfigID > 0);

	const TArray<UWpEntityComponent*> SelectedEntities = GetSelectedEntities(nullptr);
	if (!SelectedEntities.IsEmpty())
	{
		PopulateUnitToProduceClassNumbersPerProducer(SelectedEntities);

		for (int32 Index = 0; Index < SelectedEntities.Num(); ++Index)
		{
			const UWpEntityComponent* const SelectedEntityComponent = SelectedEntities[Index];
			if (IsValid(SelectedEntityComponent))
			{
				if (uint32* const UnitToProduceNumber = UnitToProduceConfigIDNumbersPerProducer[Index].Find(UnitConfigID))
				{
					*UnitToProduceNumber = *UnitToProduceNumber - 1;

					if (*UnitToProduceNumber == 0)
					{
						UnitToProduceConfigIDNumbersPerProducer[Index].Remove(UnitConfigID);
					}
				}
			}
		}

		OrderSenderState = EWpOrderSenderState::ProduceUnitOrder;

		SendOrders(EWpOrderType::ProduceUnit);

		OrderSenderState = EWpOrderSenderState::DefaultOrder;
	}
}

TMap<uint32, uint32> FWpOrderSender::GetUnitToProduceConfigIDs()
{
	const TArray<UWpEntityComponent*> SelectedEntities = GetSelectedEntities(nullptr);
	PopulateUnitToProduceClassNumbersPerProducer(SelectedEntities);

	TMap<uint32, uint32> UnitToProduceClassNumbersTotal = {};
	for (const TMap<uint32, uint32>& UnitToProduceConfigIDNumbers : UnitToProduceConfigIDNumbersPerProducer)
	{
		for (const TPair<uint32, uint32>& UnitToProduceConfigIDNumber : UnitToProduceConfigIDNumbers)
		{
			const uint32 UnitToProduceConfigID = UnitToProduceConfigIDNumber.Key;
			const uint32 UnitToProduceNumber = UnitToProduceConfigIDNumber.Value;
			if (uint32* const UnitToProduceNumberTotal = UnitToProduceClassNumbersTotal.Find(UnitToProduceConfigID))
			{
				*UnitToProduceNumberTotal = *UnitToProduceNumberTotal + UnitToProduceNumber;
			}
			else
			{
				UnitToProduceClassNumbersTotal.Add(UnitToProduceConfigID, UnitToProduceNumber);
			}
		}
	}

	return UnitToProduceClassNumbersTotal;
}

void FWpOrderSender::OnLMBStarted()
{
	switch (OrderSenderState)
	{
	case EWpOrderSenderState::DefaultOrder:
		SetSelectEntitiesStartLocation();
		break;
	case EWpOrderSenderState::SelectedOrder:
	case EWpOrderSenderState::ProduceBuildingOrder:
		SetOrderStartLocation();
		break;
	default:
		break;
	}
}

void FWpOrderSender::OnLMBTriggered()
{
	switch (OrderSenderState)
	{
	case EWpOrderSenderState::DefaultOrder:
		SelectEntities();
		break;
	case EWpOrderSenderState::SelectedOrder:
		// DrawOrderArea();
		break;
	default:
		break;
	}
}

void FWpOrderSender::OnLMBCompleted()
{
	switch (OrderSenderState)
	{
	case EWpOrderSenderState::DefaultOrder:
		SelectEntitiesStartLocation = FVector::ZeroVector;
		break;
	case EWpOrderSenderState::SelectedOrder:
		SendOrders(SelectedOrderType);
		break;
	case EWpOrderSenderState::ProduceBuildingOrder:
		SendOrders(EWpOrderType::ProduceBuilding);
		SelectBuildingToProduce(0);
		break;
	default:
		break;
	}

	OrderSenderState = EWpOrderSenderState::DefaultOrder;
	SelectedOrderType = EWpOrderType::Undefined;
}

void FWpOrderSender::OnRMBStarted()
{
	switch (OrderSenderState)
	{
	case EWpOrderSenderState::DefaultOrder:
		SetOrderStartLocation();
		break;
	case EWpOrderSenderState::SelectedOrder:
		OrderSenderState = EWpOrderSenderState::DefaultOrder;
		SelectedOrderType = EWpOrderType::Undefined;
		break;
	case EWpOrderSenderState::ProduceBuildingOrder:
		SelectBuildingToProduce(0);
		break;
	default:
		break;
	}
}

void FWpOrderSender::OnRMBTriggered()
{
	switch (OrderSenderState)
	{
	case EWpOrderSenderState::DefaultOrder:
		// DrawOrderArea();
		break;
	default:
		break;
	}
}

void FWpOrderSender::OnRMBCompleted()
{
	switch (OrderSenderState)
	{
	case EWpOrderSenderState::DefaultOrder:
		SendOrders(SelectedOrderType);
	default:
		break;
	}
}

void FWpOrderSender::SetSelectEntitiesStartLocation()
{
	const FHitResult Hit = LineTraceUnderCursor();
	if (Hit.bBlockingHit)
	{
		SelectEntitiesStartLocation = Hit.Location;
	}
}

void FWpOrderSender::SelectEntities()
{
	if (SelectEntitiesStartLocation.IsNearlyZero())
	{
		return;
	}

	const FHitResult Hit = LineTraceUnderCursor();
	if (!Hit.bBlockingHit)
	{
		return;
	}

	const FVector BoxLocation = SelectEntitiesStartLocation + (Hit.Location - SelectEntitiesStartLocation) / 2.0f;
	FVector BoxExtent = (BoxLocation - SelectEntitiesStartLocation).GetAbs();
	static constexpr float BoxExtentZ = 10000.0f;
	BoxExtent.Z = BoxExtentZ;

	DrawDebugBox(PlayerController->GetWorld(), BoxLocation, BoxExtent, FColor::Yellow);

	TArray<AActor*> SelectedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel2));
	UKismetSystemLibrary::BoxOverlapActors(PlayerController->GetWorld(), BoxLocation, BoxExtent, ObjectTypes, AActor::StaticClass(), {}, SelectedActors);

	for (const TTuple<FWpDefaultOrders, TArray<UWpEntityComponent*>>& KeyValue : SelectedEntitiesPerDefaultOrders)
	{
		for (UWpEntityComponent* const SelectedEntityComponent : SelectedEntitiesPerDefaultOrders[KeyValue.Key])
		{
			if (IsValid(SelectedEntityComponent))
			{
				WP_AS_ORDER_RECEIVER(SelectedEntityComponent).Select(false);
			}
		}
		SelectedEntitiesPerDefaultOrders[KeyValue.Key].Empty();
	}

	for (AActor* const SelectedActor : SelectedActors)
	{
		if (!IsValid(SelectedActor))
		{
			continue;
		}

		if (IWpEntityInterface* const SelectedEntity = Cast<IWpEntityInterface>(SelectedActor))
		{
			UWpEntityComponent* const SelectedEntityComponent = SelectedEntity->GetEntityComponent();

			const FWpOrderReceiver& OrderReceiver = *SelectedEntityComponent;
			if (!OrderReceiver.IsActive())
			{
				continue;
			}

			const EWpEntityState EntityState = WP_AS_ENDURANCE_CONST(SelectedEntityComponent).GetEntityState();
			if (EntityState == EWpEntityState::Wreck || SelectedActor->IsA<AWpUnit>() && EntityState != EWpEntityState::Operational)
			{
				continue;
			}

			const FWpDefaultOrders& DefaultOrders = OrderReceiver.GetDefaultOrders();
			if (!SelectedEntitiesPerDefaultOrders.Contains(DefaultOrders))
			{
				SelectedEntitiesPerDefaultOrders.Add(DefaultOrders, {});
			}
			SelectedEntitiesPerDefaultOrders[DefaultOrders].Add(SelectedEntityComponent);
		}
	}

	const TArray<UWpEntityComponent*> SelectedEntities = GetSelectedEntities(nullptr);
	TSet<EWpOrderType> AvailableOrderTypes = {};
	if (!SelectedEntities.IsEmpty())
	{
		for (UWpEntityComponent* const SelectedEntityComponent : SelectedEntities)
		{
			if (!IsValid(SelectedEntityComponent))
			{
				continue;
			}

			FWpOrderReceiver& OrderReceiver = *SelectedEntityComponent;
			OrderReceiver.Select(true);
			AvailableOrderTypes.Append(OrderReceiver.GetSupportedOrders());
		}
	}

	OnAvailableOrdersChanged.Broadcast(AvailableOrderTypes);
}

TArray<UWpEntityComponent*> FWpOrderSender::GetSelectedEntities(FWpDefaultOrders* const OutDefaultOrders) const
{
	const TArray<UWpEntityComponent*>* MaxNumEntities = nullptr;
	uint32 MaxNum = 0;
	for (const TTuple<FWpDefaultOrders, TArray<UWpEntityComponent*>>& KeyValue : SelectedEntitiesPerDefaultOrders)
	{
		const uint32 Num = SelectedEntitiesPerDefaultOrders[KeyValue.Key].Num();
		if (MaxNum < Num)
		{
			MaxNum = Num;
			MaxNumEntities = &KeyValue.Value;
			if (OutDefaultOrders)
			{
				*OutDefaultOrders = KeyValue.Key;
			}
		}
	}

	if (MaxNumEntities)
	{
		return *MaxNumEntities;
	}

	return {};
}

void FWpOrderSender::SetOrderStartLocation()
{
	const FHitResult Hit = LineTraceUnderCursor();
	if (Hit.bBlockingHit)
	{
		OrderStartLocation = Hit.Location;
	}
}

void FWpOrderSender::SendOrders(const EWpOrderType OrderType)
{
	switch (OrderType)
	{
	case EWpOrderType::Stop:
	case EWpOrderType::ProduceUnit:
	case EWpOrderType::InfiniteUnitProductionOff:
	case EWpOrderType::InfiniteUnitProductionOn:
	case EWpOrderType::SelfDestruct:
		break;
	default:
		if (OrderStartLocation.IsNearlyZero())
		{
			return;
		}
		break;
	}

	FWpDefaultOrders DefaultOrders;
	const TArray<UWpEntityComponent*> SelectedEntities = GetSelectedEntities(&DefaultOrders);
	if (SelectedEntities.IsEmpty())
	{
		OrderStartLocation = FVector::ZeroVector;
		return;
	}

	const FHitResult Hit = LineTraceUnderCursor();
	if (!Hit.bBlockingHit)
	{
		OrderStartLocation = FVector::ZeroVector;
		return;
	}

	FWpOrder Order;
	Order.Type = (OrderType != EWpOrderType::Undefined) ? OrderType : DefaultOrders.Get(BelligerentID, Hit.GetActor());
	Order.TargetLocation = Hit.Location;

	Order.Radius = FVector::Dist2D(Hit.Location, OrderStartLocation);
	if (Order.Radius < 100.0f)
	{
		Order.Radius = 0.0f;
	}
	else if (Order.Type == EWpOrderType::Patrol)
	{
		Order.TargetLocation = OrderStartLocation;

		DrawDebugCircle(PlayerController->GetWorld(), Order.TargetLocation, Order.Radius, 64, FColor::Green, false, 3.0f, 0, 0, FVector{0.0, 1.0, 0.0}, FVector{1.0, 0.0, 0.0});
	}

	if (IWpEntityInterface* const Entity = Cast<IWpEntityInterface>(Hit.GetActor()))
	{
		Order.TargetEntityComponent = Entity->GetEntityComponent();
	}

	if (OrderSenderState == EWpOrderSenderState::ProduceBuildingOrder)
	{
		Order.BuildingToProduceConfigID = BuildingToProduceConfigID;
	}

	bool bOverride = !PlayerController->IsInputKeyDown(OverrideInputKeyName);
	if (OrderSenderState == EWpOrderSenderState::ProduceUnitOrder)
	{
		bOverride = true;
	}
	Order.bOverride = bOverride;

	for (int32 Index = 0; Index < SelectedEntities.Num(); ++Index)
	{
		UWpEntityComponent* const SelectedEntityComponent = SelectedEntities[Index];
		if (!IsValid(SelectedEntityComponent))
		{
			continue;
		}

		if (OrderSenderState == EWpOrderSenderState::ProduceUnitOrder)
		{
			Order.UnitToProduceConfigIDNumbers = UnitToProduceConfigIDNumbersPerProducer[Index];
		}

		WP_AS_ORDER_RECEIVER(SelectedEntityComponent).ReceiveOrder(Order, bOverride);
	}

	OrderStartLocation = FVector::ZeroVector;
}

void FWpOrderSender::UpdateBuildingPlan()
{
	const FHitResult Hit = LineTraceUnderCursor();

	if (IsValid(BuildingToProduce))
	{
		if (Hit.bBlockingHit)
		{
			BuildingToProduce->SetActorLocation(Hit.Location);
		}
	}
	else
	{
		if (Hit.bBlockingHit)
		{
			BuildingToProduce = PlayerController->GetWorld()->SpawnActor<AWpBuilding>();

			const UDataTable* const EntityConfigs = PlayerController->GetWorld()->GetGameState<AWpGameGameState>()->GetEntityConfigs();
			const FName RowName = FName{FString::FromInt(BuildingToProduceConfigID)};
			const FWpEntityConfig& EntityConfig = *EntityConfigs->FindRow<FWpEntityConfig>(RowName, nullptr);

			FWpPooledEntityData PooledEntityData;
			PooledEntityData.EntityConfig = &EntityConfig;
			PooledEntityData.EntityConfigID = BuildingToProduceConfigID;
			PooledEntityData.BelligerentID = BelligerentID;

			BuildingToProduce->AcquirePooledObject(&PooledEntityData);

			BuildingToProduce->ReleasePooledObject();
			BuildingToProduce->SetActorHiddenInGame(false);
		}
	}
}

void FWpOrderSender::PopulateUnitToProduceClassNumbersPerProducer(const TArray<UWpEntityComponent*>& SelectedEntities)
{
	if (!SelectedEntities.IsEmpty())
	{
		UnitToProduceConfigIDNumbersPerProducer.SetNum(SelectedEntities.Num());

		for (int32 Index = 0; Index < SelectedEntities.Num(); ++Index)
		{
			const UWpEntityComponent* const SelectedEntityComponent = SelectedEntities[Index];
			if (!IsValid(SelectedEntityComponent))
			{
				continue;
			}

			UnitToProduceConfigIDNumbersPerProducer[Index] = WP_AS_ORDER_RECEIVER_CONST(SelectedEntityComponent).GetOrder().UnitToProduceConfigIDNumbers;
		}
	}
}

FHitResult FWpOrderSender::LineTraceUnderCursor() const
{
	FHitResult Hit;
	PlayerController->GetHitResultUnderCursor(ECC_Camera, false, Hit);
	DrawDebugPoint(PlayerController->GetWorld(), Hit.Location, 10, FColor::Red);
	return Hit;
}
