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
#include "Warpunk/Game/Pool/WpPooledActorVisibilityManager.h"

#include "Kismet/KismetSystemLibrary.h"

const FName UWpOrderSenderComponent::OverrideInputKeyName = TEXT("LeftShift");

bool FWpSelectedEntityCategory::operator==(const FWpSelectedEntityCategory& Other) const
{
	return (DefaultOrders == Other.DefaultOrders) && (EntityClass == Other.EntityClass);
}

uint32 GetTypeHash(const FWpSelectedEntityCategory& Other)
{
	uint32 Hash = GetTypeHash(Other.DefaultOrders);
	Hash = HashCombine(Hash, GetTypeHash(Other.EntityClass));
	return Hash;
}

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
	if (BuildingToProduce)
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

	if (SelectedEntities.IsEmpty())
	{
		return;
	}

	PopulateUnitToProduceConfigIdNumbersPerProducer();

	for (const IWpEntityInterface* const SelectedEntity : SelectedEntities)
	{
		if (uint32* const UnitToProduceNumber = UnitToProduceConfigIdNumbersPerProducer[SelectedEntity].Find(UnitConfigId))
		{
			*UnitToProduceNumber = *UnitToProduceNumber + 1;
		}
		else
		{
			UnitToProduceConfigIdNumbersPerProducer[SelectedEntity].Add(UnitConfigId, 1);
		}
	}

	OrderSenderState = EWpOrderSenderState::ProduceUnitOrder;

	SendOrders(EWpOrderType::ProduceUnit);

	OrderSenderState = EWpOrderSenderState::DefaultOrder;
}

void UWpOrderSenderComponent::RemoveUnitToProduce(const uint32 UnitConfigId)
{
	ensureAlways(UnitConfigId > 0);

	if (SelectedEntities.IsEmpty())
	{
		return;
	}

	PopulateUnitToProduceConfigIdNumbersPerProducer();

	for (const IWpEntityInterface* const SelectedEntity : SelectedEntities)
	{
		if (uint32* const UnitToProduceNumber = UnitToProduceConfigIdNumbersPerProducer[SelectedEntity].Find(UnitConfigId))
		{
			*UnitToProduceNumber = *UnitToProduceNumber - 1;

			if (*UnitToProduceNumber == 0)
			{
				UnitToProduceConfigIdNumbersPerProducer[SelectedEntity].Remove(UnitConfigId);
			}
		}
	}

	OrderSenderState = EWpOrderSenderState::ProduceUnitOrder;

	SendOrders(EWpOrderType::ProduceUnit);

	OrderSenderState = EWpOrderSenderState::DefaultOrder;
}

TMap<uint32, uint32> UWpOrderSenderComponent::GetUnitsToProduce()
{
	if (SelectedEntities.IsEmpty())
	{
		return {};
	}

	PopulateUnitToProduceConfigIdNumbersPerProducer();

	TMap<uint32, uint32> UnitToProduceClassNumbersTotal;
	for (const TPair<const IWpEntityInterface*, TMap<uint32, uint32>>& KeyValue : UnitToProduceConfigIdNumbersPerProducer)
	{
		const TMap<uint32, uint32>& UnitToProduceConfigIdNumbers = KeyValue.Value;
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

		if (Order.TargetEntityActor)
		{
			const bool bTargetEntityIsPooledActor = Order.TargetEntityActor->Implements<UWpPooledActorInterface>();
			if (!bTargetEntityIsPooledActor)
			{
				continue;
			}

			const bool bTargetEntityIsVisibleToPlayer = AWpPooledActorVisibilityManager::IsVisibleForTeam(GetTeamId(), Order.TargetEntityActor);
			if (!bTargetEntityIsVisibleToPlayer)
			{
				continue;
			}
		}

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
	if (!GetPlayerController()->GetPlayerState<AWpGamePlayerState>())
	{
		return;
	}

	const TSet<IWpEntityInterface*> PreviouslySelectedEntities = SelectedEntities;
	for (IWpEntityInterface* const SelectedEntity : PreviouslySelectedEntities)
	{
		SelectEntity(SelectedEntity, false);
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
			if ((EntityState == EWpEntityState::Wreck) || (SelectedActor->IsA<AWpUnit>() && (EntityState != EWpEntityState::Operational)))
			{
				continue;
			}

			const FWpDefaultOrders& DefaultOrders = OrderReceiverComponent->GetDefaultOrders();
			const UClass* const EntityClass = Cast<AActor>(SelectedEntity)->GetClass();
			const FWpSelectedEntityCategory Category = {DefaultOrders, EntityClass};

			if (!SelectedEntitiesPerCategory.Contains(Category))
			{
				SelectedEntitiesPerCategory.Add(Category, {});
			}
			SelectedEntitiesPerCategory[Category].Add(SelectedEntity);
		}
	}

	if (const TArray<IWpEntityInterface*>* const NewSelectedEntities = GetSelectedEntitiesWithDefaultOrders(&SelectedEntitiesDefaultOrders))
	{
		for (IWpEntityInterface* const SelectedEntity : *NewSelectedEntities)
		{
			SelectEntity(SelectedEntity, true);
		}

		for (TPair<FWpSelectedEntityCategory, TArray<IWpEntityInterface*>>& KeyValue : SelectedEntitiesPerCategory)
		{
			KeyValue.Value.Empty(KeyValue.Value.Num());
		}
	}

	UpdateAvailableOrderTypes();
}

const TArray<IWpEntityInterface*>* UWpOrderSenderComponent::GetSelectedEntitiesWithDefaultOrders(FWpDefaultOrders* const OutDefaultOrders) const
{
	const TArray<IWpEntityInterface*>* MaxNumEntities = nullptr;
	uint32 MaxNum = 0;
	for (const TPair<FWpSelectedEntityCategory, TArray<IWpEntityInterface*>>& KeyValue : SelectedEntitiesPerCategory)
	{
		const uint32 Num = KeyValue.Value.Num();
		if (MaxNum < Num)
		{
			MaxNum = Num;
			MaxNumEntities = &KeyValue.Value;

			if (OutDefaultOrders)
			{
				const FWpSelectedEntityCategory& MaxNumCategory = KeyValue.Key;
				*OutDefaultOrders = MaxNumCategory.DefaultOrders;
			}
		}
	}

	if (MaxNumEntities)
	{
		return MaxNumEntities;
	}

	if (OutDefaultOrders)
	{
		*OutDefaultOrders = {};
	}
	return nullptr;
}

void UWpOrderSenderComponent::SelectEntity(IWpEntityInterface* const Entity, const bool bSelected)
{
	UWpOrderReceiverComponent* const OrderReceiverComponent = Entity->GetOrderReceiverComponent();
	OrderReceiverComponent->Select(bSelected);

	UWpEnduranceComponent* const EnduranceComponent = Entity->GetEnduranceComponent();
	if (bSelected)
	{
		ensureAlways(!SelectedEntities.Contains(Entity));
		SelectedEntities.Add(Entity);

		const FDelegateHandle DelegateHandle = EnduranceComponent->OnEntityStateChanged().AddUObject(this, &UWpOrderSenderComponent::OnSelectedEntityStateChanged);
		OnSelectedEntityStateChangedDelegateHandlesPerSelectedEntity.Emplace(Entity, DelegateHandle);
	}
	else if (OnSelectedEntityStateChangedDelegateHandlesPerSelectedEntity.Contains(Entity))
	{
		ensureAlways(SelectedEntities.Contains(Entity));
		SelectedEntities.Remove(Entity);

		const FDelegateHandle DelegateHandle = OnSelectedEntityStateChangedDelegateHandlesPerSelectedEntity[Entity];
		OnSelectedEntityStateChangedDelegateHandlesPerSelectedEntity.Remove(Entity);
		EnduranceComponent->OnEntityStateChanged().Remove(DelegateHandle);
	}
}

void UWpOrderSenderComponent::UpdateAvailableOrderTypes()
{
	SelectedEntitiesAvailableOrderTypes.Empty(SelectedEntitiesAvailableOrderTypes.Num());

	for (const IWpEntityInterface* const SelectedEntity : SelectedEntities)
	{
		const UWpOrderReceiverComponent* const OrderReceiverComponent = SelectedEntity->GetOrderReceiverComponent();
		SelectedEntitiesAvailableOrderTypes.Append(OrderReceiverComponent->GetSupportedOrders());
	}

	AvailableOrdersChangedEvent.Broadcast(SelectedEntitiesAvailableOrderTypes);
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
	if (!GetPlayerController()->GetPlayerState<AWpGamePlayerState>())
	{
		return;
	}

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
	Order.Type = (OrderType != EWpOrderType::Undefined) ? OrderType : SelectedEntitiesDefaultOrders.Get(GetBelligerentId(), Hit.GetActor());
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
	for (IWpEntityInterface* const SelectedEntity : SelectedEntities)
	{
		if (OrderSenderState == EWpOrderSenderState::ProduceUnitOrder)
		{
			Order.UnitToProduceConfigIdNumbers = FWpConfigIdNumber::FromMap(UnitToProduceConfigIdNumbersPerProducer[SelectedEntity]);
		}

		FWpNetOrder NetOrder;
		NetOrder.EntityActor = Cast<AActor>(SelectedEntity);
		NetOrder.Order = Order;
		NetOrders.Add(NetOrder);
	}
	ServerSendOrders(NetOrders, bOverride);

	OrderStartLocation = FVector::ZeroVector;
}

void UWpOrderSenderComponent::UpdateBuildingPlan()
{
	if (!GetPlayerController()->GetPlayerState<AWpGamePlayerState>())
	{
		return;
	}

	const FHitResult Hit = LineTraceUnderCursor();

	if (BuildingToProduce)
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
			PooledEntityData.TeamId = GetTeamId();

			BuildingToProduce->AcquirePooledObject(PooledEntityData);
			BuildingToProduce->ReleasePooledObject();
			BuildingToProduce->SetActorHiddenInGame(false);
			BuildingToProduce->OnPooledActorVisibilityChanged(true);
		}
	}
}

void UWpOrderSenderComponent::PopulateUnitToProduceConfigIdNumbersPerProducer()
{
	if (!SelectedEntities.IsEmpty())
	{
		UnitToProduceConfigIdNumbersPerProducer.Empty(SelectedEntities.Num());

		for (const IWpEntityInterface* const SelectedEntity : SelectedEntities)
		{
			UnitToProduceConfigIdNumbersPerProducer.Emplace(
				SelectedEntity,
				FWpConfigIdNumber::ToMap(SelectedEntity->GetOrderReceiverComponent()->GetUnitToProduceConfigIdNumbers()));
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
	const AWpGamePlayerState* const PlayerState = GetPlayerController()->GetPlayerState<AWpGamePlayerState>();
	ensureAlways(PlayerState);
	return PlayerState->GetBelligerentId();
}

uint32 UWpOrderSenderComponent::GetTeamId() const
{
	const AWpGamePlayerState* const PlayerState = GetPlayerController()->GetPlayerState<AWpGamePlayerState>();
	ensureAlways(PlayerState);
	return PlayerState->GetTeamId();
}

void UWpOrderSenderComponent::OnSelectedEntityStateChanged(IWpEntityInterface* const SelectedEntity, const EWpEntityState EntityState)
{
	if (EntityState == EWpEntityState::Wreck)
	{
		SelectEntity(SelectedEntity, false);

		UpdateAvailableOrderTypes();
	}
}
