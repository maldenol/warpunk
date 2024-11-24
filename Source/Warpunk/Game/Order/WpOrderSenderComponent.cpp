#include "WpOrderSenderComponent.h"

#include "Warpunk/Game/Belligerent/WpBelligerentComponent.h"
#include "Warpunk/Game/Endurance/WpEnduranceComponent.h"
#include "Warpunk/Game/Entity/WpBuilding.h"
#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Entity/WpEntityInterface.h"
#include "Warpunk/Game/Entity/WpPooledEntityData.h"
#include "Warpunk/Game/Entity/WpUnit.h"
#include "Warpunk/Game/GameMode/WpGameGameState.h"
#include "Warpunk/Game/Order/WpOrderReceiverComponent.h"
#include "Warpunk/Game/Player/WpGamePlayerState.h"

#include "Kismet/KismetSystemLibrary.h"

const FName UWpOrderSenderComponent::OverrideInputKeyName = TEXT("LeftShift");

UWpOrderSenderComponent::UWpOrderSenderComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicatedByDefault(false);
}

void UWpOrderSenderComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* const ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (OrderSenderState == EWpOrderSenderState::ProduceBuildingOrder)
	{
		UpdateBuildingPlan();
	}
}

void UWpOrderSenderComponent::SelectOrderType(const EWpOrderType OrderType)
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

void UWpOrderSenderComponent::SelectBuildingToProduce(const uint32 BuildingConfigId)
{
	if (IsValid(BuildingToProduce))
	{
		BuildingToProduce->Destroy();
		BuildingToProduce = nullptr;
	}

	if (BuildingConfigId > 0)
	{
		OrderSenderState = EWpOrderSenderState::ProduceBuildingOrder;

		BuildingToProduceConfigId = BuildingConfigId;
	}
	else
	{
		OrderSenderState = EWpOrderSenderState::DefaultOrder;
	}
}

void UWpOrderSenderComponent::AddUnitToProduce(const uint32 UnitConfigId)
{
	ensureAlways(UnitConfigId > 0);

	const TArray<IWpEntityInterface*> SelectedEntities = GetSelectedEntities(nullptr);
	if (!SelectedEntities.IsEmpty())
	{
		PopulateUnitToProduceConfigIdNumbersPerProducer(SelectedEntities);

		for (int32 Index = 0; Index < SelectedEntities.Num(); ++Index)
		{
			if (uint32* const UnitToProduceNumber = UnitToProduceConfigIdNumbersPerProducer[Index].Find(UnitConfigId))
			{
				*UnitToProduceNumber = *UnitToProduceNumber + 1;
			}
			else
			{
				UnitToProduceConfigIdNumbersPerProducer[Index].Add(UnitConfigId, 1);
			}
		}

		OrderSenderState = EWpOrderSenderState::ProduceUnitOrder;

		SendOrders(EWpOrderType::ProduceUnit);

		OrderSenderState = EWpOrderSenderState::DefaultOrder;
	}
}

void UWpOrderSenderComponent::RemoveUnitToProduce(const uint32 UnitConfigId)
{
	ensureAlways(UnitConfigId > 0);

	const TArray<IWpEntityInterface*> SelectedEntities = GetSelectedEntities(nullptr);
	if (!SelectedEntities.IsEmpty())
	{
		PopulateUnitToProduceConfigIdNumbersPerProducer(SelectedEntities);

		for (int32 Index = 0; Index < SelectedEntities.Num(); ++Index)
		{
			if (uint32* const UnitToProduceNumber = UnitToProduceConfigIdNumbersPerProducer[Index].Find(UnitConfigId))
			{
				*UnitToProduceNumber = *UnitToProduceNumber - 1;

				if (*UnitToProduceNumber == 0)
				{
					UnitToProduceConfigIdNumbersPerProducer[Index].Remove(UnitConfigId);
				}
			}
		}

		OrderSenderState = EWpOrderSenderState::ProduceUnitOrder;

		SendOrders(EWpOrderType::ProduceUnit);

		OrderSenderState = EWpOrderSenderState::DefaultOrder;
	}
}

TMap<uint32, uint32> UWpOrderSenderComponent::GetUnitsToProduce()
{
	const TArray<IWpEntityInterface*> SelectedEntities = GetSelectedEntities(nullptr);
	PopulateUnitToProduceConfigIdNumbersPerProducer(SelectedEntities);

	TMap<uint32, uint32> UnitToProduceClassNumbersTotal;
	for (const TMap<uint32, uint32>& UnitToProduceConfigIdNumbers : UnitToProduceConfigIdNumbersPerProducer)
	{
		for (const TPair<uint32, uint32>& UnitToProduceConfigIdNumber : UnitToProduceConfigIdNumbers)
		{
			const uint32 UnitToProduceConfigId = UnitToProduceConfigIdNumber.Key;
			const uint32 UnitToProduceNumber = UnitToProduceConfigIdNumber.Value;
			if (uint32* const UnitToProduceNumberTotal = UnitToProduceClassNumbersTotal.Find(UnitToProduceConfigId))
			{
				*UnitToProduceNumberTotal = *UnitToProduceNumberTotal + UnitToProduceNumber;
			}
			else
			{
				UnitToProduceClassNumbersTotal.Add(UnitToProduceConfigId, UnitToProduceNumber);
			}
		}
	}

	return UnitToProduceClassNumbersTotal;
}

void UWpOrderSenderComponent::OnLMBStarted()
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

void UWpOrderSenderComponent::OnLMBTriggered()
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

void UWpOrderSenderComponent::OnLMBCompleted()
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

void UWpOrderSenderComponent::OnRMBStarted()
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

void UWpOrderSenderComponent::OnRMBTriggered()
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

void UWpOrderSenderComponent::OnRMBCompleted()
{
	switch (OrderSenderState)
	{
	case EWpOrderSenderState::DefaultOrder:
		SendOrders(SelectedOrderType);
	default:
		break;
	}
}

UWpOrderSenderComponent::FAvailableOrdersChangedEvent& UWpOrderSenderComponent::OnAvailableOrdersChanged()
{
	return AvailableOrdersChangedEvent;
}

void UWpOrderSenderComponent::ServerSendOrders_Implementation(const TArray<FWpNetOrder>& NetOrders, const bool bOverride)
{
	for (const FWpNetOrder& NetOrder : NetOrders)
	{
		IWpEntityInterface* const Entity = Cast<IWpEntityInterface>(NetOrder.EntityActor);

		const bool bEntityBelongsToSameBelligerentAsPlayer = Entity->GetBelligerentComponent()->BelongsToSameBelligerent(GetBelligerentId());
		if (!bEntityBelongsToSameBelligerentAsPlayer)
		{
			continue;
		}

		const FWpOrder& Order = NetOrder.Order;

		if (Order.Type == EWpOrderType::Undefined)
		{
			continue;
		}

		Entity->GetOrderReceiverComponent()->ReceiveOrder(Order, bOverride);
	}
}

void UWpOrderSenderComponent::SetSelectEntitiesStartLocation()
{
	const FHitResult Hit = LineTraceUnderCursor();
	if (Hit.bBlockingHit)
	{
		SelectEntitiesStartLocation = Hit.Location;
	}
}

void UWpOrderSenderComponent::SelectEntities()
{
	for (const TTuple<FWpDefaultOrders, TArray<IWpEntityInterface*>>& KeyValue : SelectedEntitiesPerDefaultOrders)
	{
		for (IWpEntityInterface* const SelectedEntity : SelectedEntitiesPerDefaultOrders[KeyValue.Key])
		{
			SelectedEntity->GetOrderReceiverComponent()->Select(false);
		}
		SelectedEntitiesPerDefaultOrders[KeyValue.Key].Empty();
	}

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

	DrawDebugBox(GetWorld(), BoxLocation, BoxExtent, FColor::Yellow);

	TArray<AActor*> SelectedActors;
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel2));
	UKismetSystemLibrary::BoxOverlapActors(GetWorld(), BoxLocation, BoxExtent, ObjectTypes, AActor::StaticClass(), {}, SelectedActors);

	for (AActor* const SelectedActor : SelectedActors)
	{
		if (!IsValid(SelectedActor))
		{
			continue;
		}

		if (IWpEntityInterface* const SelectedEntity = Cast<IWpEntityInterface>(SelectedActor))
		{
			const UWpBelligerentComponent* const BelligerentComponent = SelectedEntity->GetBelligerentComponent();
			if (!BelligerentComponent->BelongsToSameBelligerent(GetBelligerentId()))
			{
				continue;
			}

			const UWpOrderReceiverComponent* const OrderReceiverComponent = SelectedEntity->GetOrderReceiverComponent();
			if (!OrderReceiverComponent->IsActive())
			{
				continue;
			}

			const EWpEntityState EntityState = SelectedEntity->GetEnduranceComponent()->GetEntityState();
			if (EntityState == EWpEntityState::Wreck || SelectedActor->IsA<AWpUnit>() && EntityState != EWpEntityState::Operational)
			{
				continue;
			}

			const FWpDefaultOrders& DefaultOrders = OrderReceiverComponent->GetDefaultOrders();
			if (!SelectedEntitiesPerDefaultOrders.Contains(DefaultOrders))
			{
				SelectedEntitiesPerDefaultOrders.Add(DefaultOrders, {});
			}
			SelectedEntitiesPerDefaultOrders[DefaultOrders].Add(SelectedEntity);
		}
	}

	const TArray<IWpEntityInterface*> SelectedEntities = GetSelectedEntities(nullptr);
	TSet<EWpOrderType> AvailableOrderTypes;
	if (!SelectedEntities.IsEmpty())
	{
		for (IWpEntityInterface* const SelectedEntity : SelectedEntities)
		{
			UWpOrderReceiverComponent* const OrderReceiverComponent = SelectedEntity->GetOrderReceiverComponent();
			OrderReceiverComponent->Select(true);
			AvailableOrderTypes.Append(OrderReceiverComponent->GetSupportedOrders());
		}
	}

	AvailableOrdersChangedEvent.Broadcast(AvailableOrderTypes);
}

TArray<IWpEntityInterface*> UWpOrderSenderComponent::GetSelectedEntities(FWpDefaultOrders* const OutDefaultOrders) const
{
	const TArray<IWpEntityInterface*>* MaxNumEntities = nullptr;
	uint32 MaxNum = 0;
	for (const TTuple<FWpDefaultOrders, TArray<IWpEntityInterface*>>& KeyValue : SelectedEntitiesPerDefaultOrders)
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

void UWpOrderSenderComponent::SetOrderStartLocation()
{
	const FHitResult Hit = LineTraceUnderCursor();
	if (Hit.bBlockingHit)
	{
		OrderStartLocation = Hit.Location;
	}
}

void UWpOrderSenderComponent::SendOrders(const EWpOrderType OrderType)
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
	const TArray<IWpEntityInterface*> SelectedEntities = GetSelectedEntities(&DefaultOrders);
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
	Order.Type = (OrderType != EWpOrderType::Undefined) ? OrderType : DefaultOrders.Get(GetBelligerentId(), Hit.GetActor());
	Order.TargetLocation = Hit.Location;

	Order.Radius = FVector::Dist2D(Hit.Location, OrderStartLocation);
	if (Order.Radius < 100.0f)
	{
		Order.Radius = 0.0f;
	}
	else if (Order.Type == EWpOrderType::Patrol)
	{
		Order.TargetLocation = OrderStartLocation;

		DrawDebugCircle(GetWorld(), Order.TargetLocation, Order.Radius, 64, FColor::Green, false, 3.0f, 0, 0, FVector{0.0, 1.0, 0.0}, FVector{1.0, 0.0, 0.0});
	}

	if (Cast<IWpEntityInterface>(Hit.GetActor()))
	{
		Order.TargetEntityActor = Hit.GetActor();
	}

	if (OrderSenderState == EWpOrderSenderState::ProduceBuildingOrder)
	{
		Order.BuildingToProduceConfigId = BuildingToProduceConfigId;
	}

	bool bOverride = !GetPlayerController()->IsInputKeyDown(OverrideInputKeyName);
	if (OrderSenderState == EWpOrderSenderState::ProduceUnitOrder)
	{
		bOverride = true;
	}
	Order.bOverride = bOverride;

	TArray<FWpNetOrder> NetOrders;
	NetOrders.Reserve(SelectedEntities.Num());
	for (int32 Index = 0; Index < SelectedEntities.Num(); ++Index)
	{
		if (OrderSenderState == EWpOrderSenderState::ProduceUnitOrder)
		{
			Order.UnitToProduceConfigIdNumbers = FWpConfigIdNumber::FromMap(UnitToProduceConfigIdNumbersPerProducer[Index]);
		}

		FWpNetOrder NetOrder;
		NetOrder.EntityActor = Cast<AActor>(SelectedEntities[Index]);
		NetOrder.Order = Order;
		NetOrders.Add(NetOrder);
	}
	ServerSendOrders(NetOrders, bOverride);

	OrderStartLocation = FVector::ZeroVector;
}

void UWpOrderSenderComponent::UpdateBuildingPlan()
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
			BuildingToProduce = GetWorld()->SpawnActor<AWpBuilding>();
			BuildingToProduce->SetReplicates(false);

			const UDataTable* const EntityConfigs = GetWorld()->GetGameState<AWpGameGameState>()->GetEntityConfigs();
			const FName RowName = FName{FString::FromInt(BuildingToProduceConfigId)};
			const FWpEntityConfig& EntityConfig = *EntityConfigs->FindRow<FWpEntityConfig>(RowName, nullptr);

			FWpPooledEntityData PooledEntityData;
			PooledEntityData.EntityConfig = &EntityConfig;
			PooledEntityData.EntityConfigId = BuildingToProduceConfigId;
			PooledEntityData.BelligerentId = GetBelligerentId();

			BuildingToProduce->AcquirePooledObject(PooledEntityData);

			BuildingToProduce->ReleasePooledObject();
			BuildingToProduce->SetActorHiddenInGame(false);
		}
	}
}

void UWpOrderSenderComponent::PopulateUnitToProduceConfigIdNumbersPerProducer(const TArray<IWpEntityInterface*>& SelectedEntities)
{
	if (!SelectedEntities.IsEmpty())
	{
		UnitToProduceConfigIdNumbersPerProducer.SetNum(SelectedEntities.Num());

		for (int32 Index = 0; Index < SelectedEntities.Num(); ++Index)
		{
			const IWpEntityInterface* const SelectedEntity = SelectedEntities[Index];
			UnitToProduceConfigIdNumbersPerProducer[Index] = FWpConfigIdNumber::ToMap(SelectedEntity->GetOrderReceiverComponent()->GetUnitToProduceConfigIdNumbers());
		}
	}
}

FHitResult UWpOrderSenderComponent::LineTraceUnderCursor() const
{
	FHitResult Hit;
	GetPlayerController()->GetHitResultUnderCursor(ECC_Camera, false, Hit);
	DrawDebugPoint(GetWorld(), Hit.Location, 10, FColor::Red);
	return Hit;
}

APlayerController* UWpOrderSenderComponent::GetPlayerController() const
{
	return GetOwner<APawn>()->GetController<APlayerController>();
}

uint32 UWpOrderSenderComponent::GetBelligerentId() const
{
	return GetPlayerController()->GetPlayerState<AWpGamePlayerState>()->GetBelligerentId();
}
