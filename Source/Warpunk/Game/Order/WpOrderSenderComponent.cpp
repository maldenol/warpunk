#include "WpOrderSenderComponent.h"

#include "Warpunk/Game/Belligerent/WpBelligerentComponent.h"
#include "Warpunk/Game/Endurance/WpEnduranceComponent.h"
#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Entity/WpLocalEntity.h"
#include "Warpunk/Game/Entity/WpPooledEntityData.h"
#include "Warpunk/Game/Order/WpOrderReceiverComponent.h"
#include "Warpunk/Game/Player/WpGamePlayerState.h"
#include "Warpunk/Game/Order/WpCommandMarker.h"
#include "Warpunk/Game/Pool/WpActorPool.h"
#include "Warpunk/Game/Pool/WpPooledActorVisibilityManager.h"
#include "Warpunk/Game/UI/WpSelectionMarqueeWidget.h"
#include "Warpunk/Global/GameInstance/WpGameInstance.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/KismetSystemLibrary.h"

const FName UWpOrderSenderComponent::OverrideInputKeyName = TEXT("LeftShift");

bool FWpSelectedEntityCategory::operator==(const FWpSelectedEntityCategory& Other) const
{
	return (DefaultOrders == Other.DefaultOrders) && (EntityType == Other.EntityType);
}

uint32 GetTypeHash(const FWpSelectedEntityCategory& Other)
{
	uint32 Hash = GetTypeHash(Other.DefaultOrders);
	Hash = HashCombine(Hash, GetTypeHash(Other.EntityType));
	return Hash;
}

UWpOrderSenderComponent::UWpOrderSenderComponent()
{
	bAutoActivate = true;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	SetIsReplicatedByDefault(false);
}

void UWpOrderSenderComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* const ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (OrderSenderState == EWpOrderSenderState::ProduceBuildingOrder)
	{
		UpdateBuildingPlan();
	}

	UpdateSelectionMarquee();
	UpdateOrderPreview();
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
		AWpActorPool::Release<AWpLocalEntity>(BuildingToProduce);
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

	for (const AWpEntity* const SelectedEntity : SelectedEntities)
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

	for (const AWpEntity* const SelectedEntity : SelectedEntities)
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
	for (const TPair<const AWpEntity*, TMap<uint32, uint32>>& KeyValue : UnitToProduceConfigIdNumbersPerProducer)
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
		bSelectingEntities = !SelectEntitiesStartLocation.IsNearlyZero();
		if (bSelectingEntities)
		{
			SelectEntitiesStartScreen = UWidgetLayoutLibrary::GetMousePositionOnViewport(this);
		}
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
		bSelectingEntities = false;
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
	case EWpOrderSenderState::ProduceBuildingOrder:
		SelectBuildingToProduce(0);
		break;
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
		const bool bEntityBelongsToSameBelligerentAsPlayer = NetOrder.Entity->GetBelligerentComponent()->BelongsToSameBelligerent(GetBelligerentId());
		if (!bEntityBelongsToSameBelligerentAsPlayer)
		{
			continue;
		}

		const FWpOrder& Order = NetOrder.Order;

		if (Order.TargetEntity)
		{
			const bool bTargetEntityIsVisibleToPlayer = AWpPooledActorVisibilityManager::IsVisibleForTeam(GetTeamId(), Order.TargetEntity);
			if (!bTargetEntityIsVisibleToPlayer)
			{
				continue;
			}
		}

		if (Order.Type == EWpOrderType::Undefined)
		{
			continue;
		}

		UWpOrderReceiverComponent* const OrderReceiverComponent = NetOrder.Entity->GetOrderReceiverComponent();
		if (OrderReceiverComponent->IsActive())
		{
			OrderReceiverComponent->ReceiveOrder(Order, bOverride);
		}
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
	const TSet<AWpEntity*> PreviouslySelectedEntities = SelectedEntities;
	for (AWpEntity* const SelectedEntity : PreviouslySelectedEntities)
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

	TArray<AActor*> SelectedActors;
	const TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes =
	{
		UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel2)
	};
	UKismetSystemLibrary::BoxOverlapActors(GetWorld(), BoxLocation, BoxExtent, ObjectTypes, AWpEntity::StaticClass(), {}, SelectedActors);

	for (AActor* const SelectedActor : SelectedActors)
	{
		if (AWpEntity* const SelectedEntity = Cast<AWpEntity>(SelectedActor))
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
			if ((EntityState == EWpEntityState::Wreck)
				|| ((SelectedEntity->GetEntityType() == EWpEntityType::Unit) && (EntityState != EWpEntityState::Operational)))
			{
				continue;
			}

			const FWpDefaultOrders& DefaultOrders = OrderReceiverComponent->GetDefaultOrders();
			const EWpEntityType EntityType = SelectedEntity->GetEntityType();
			const FWpSelectedEntityCategory Category = {DefaultOrders, EntityType};

			if (!SelectedEntitiesPerCategory.Contains(Category))
			{
				SelectedEntitiesPerCategory.Add(Category, {});
			}
			SelectedEntitiesPerCategory[Category].Add(SelectedEntity);
		}
	}

	if (const TArray<AWpEntity*>* const NewSelectedEntities = GetSelectedEntitiesWithDefaultOrders(&SelectedEntitiesDefaultOrders))
	{
		for (AWpEntity* const SelectedEntity : *NewSelectedEntities)
		{
			SelectEntity(SelectedEntity, true);
		}

		for (TPair<FWpSelectedEntityCategory, TArray<AWpEntity*>>& KeyValue : SelectedEntitiesPerCategory)
		{
			KeyValue.Value.Empty(KeyValue.Value.Num());
		}
	}

	UpdateAvailableOrderTypes();
}

const TArray<AWpEntity*>* UWpOrderSenderComponent::GetSelectedEntitiesWithDefaultOrders(FWpDefaultOrders* const OutDefaultOrders) const
{
	const TArray<AWpEntity*>* MaxNumEntities = nullptr;
	uint32 MaxNum = 0;
	for (const TPair<FWpSelectedEntityCategory, TArray<AWpEntity*>>& KeyValue : SelectedEntitiesPerCategory)
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

void UWpOrderSenderComponent::SelectEntity(AWpEntity* const Entity, const bool bSelected)
{
	UWpOrderReceiverComponent* const OrderReceiverComponent = Entity->GetOrderReceiverComponent();
	OrderReceiverComponent->Select(bSelected);

	UWpEnduranceComponent* const EnduranceComponent = Entity->GetEnduranceComponent();
	if (bSelected)
	{
		ensureAlways(!SelectedEntities.Contains(Entity));
		SelectedEntities.Add(Entity);

		EnduranceComponent->OnEntityStateChanged().AddUObject(this, &UWpOrderSenderComponent::OnSelectedEntityStateChanged);
	}
	else
	{
		SelectedEntities.Remove(Entity);

		EnduranceComponent->OnEntityStateChanged().RemoveAll(this);
	}
}

void UWpOrderSenderComponent::UpdateAvailableOrderTypes()
{
	SelectedEntitiesAvailableOrderTypes.Empty(SelectedEntitiesAvailableOrderTypes.Num());

	for (const AWpEntity* const SelectedEntity : SelectedEntities)
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
	}

	if (AWpEntity* const TargetEntity = Cast<AWpEntity>(Hit.GetActor()))
	{
		Order.TargetEntity = TargetEntity;
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
	for (AWpEntity* const SelectedEntity : SelectedEntities)
	{
		if (OrderSenderState == EWpOrderSenderState::ProduceUnitOrder)
		{
			Order.UnitToProduceConfigIdNumbers = FWpConfigIdNumber::FromMap(UnitToProduceConfigIdNumbersPerProducer[SelectedEntity]);
		}

		FWpNetOrder NetOrder;
		NetOrder.Entity = SelectedEntity;
		NetOrder.Order = Order;
		NetOrders.Add(NetOrder);
	}
	ServerSendOrders(NetOrders, bOverride);

	if (ShouldShowOrderMarker(Order.Type))
	{
		const FVector MarkerLocation = (Order.Type == EWpOrderType::Patrol) ? Order.TargetLocation : FVector{Hit.Location};
		const float MarkerRadius = (Order.Radius > 0.0f) ? Order.Radius : 120.0f;
		ShowOrderMarker(MarkerLocation, MarkerRadius, Order.Type);
	}
	if (OrderPreview)
	{
		OrderPreview->HidePreview();
	}

	OrderStartLocation = FVector::ZeroVector;
}

void UWpOrderSenderComponent::UpdateBuildingPlan()
{
	const FHitResult Hit = LineTraceUnderCursor();

	if (BuildingToProduce)
	{
		if (Hit.bBlockingHit)
		{
			BuildingToProduce->SetActorLocation(Hit.Location, false);
		}
	}
	else
	{
		if (Hit.bBlockingHit)
		{
			const FWpEntityConfig EntityConfig = GetOwner()->GetGameInstance<UWpGameInstance>()->GetEntityConfig(BuildingToProduceConfigId);

			FWpPooledEntityData PooledEntityData;
			PooledEntityData.EntityConfig = &EntityConfig;
			PooledEntityData.EntityConfigId = BuildingToProduceConfigId;
			PooledEntityData.BelligerentId = GetBelligerentId();
			PooledEntityData.TeamId = GetTeamId();

			BuildingToProduce = AWpActorPool::Acquire<AWpLocalEntity>(GetWorld(), FVector::ZeroVector, FRotator::ZeroRotator, PooledEntityData);
		}
	}
}

void UWpOrderSenderComponent::PopulateUnitToProduceConfigIdNumbersPerProducer()
{
	if (!SelectedEntities.IsEmpty())
	{
		UnitToProduceConfigIdNumbersPerProducer.Empty(SelectedEntities.Num());

		for (const AWpEntity* const SelectedEntity : SelectedEntities)
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
	return Hit;
}

void UWpOrderSenderComponent::EnsureOrderVisuals()
{
	if (!SelectionMarquee)
	{
		const TSubclassOf<UWpSelectionMarqueeWidget> MarqueeClass = LoadClass<UWpSelectionMarqueeWidget>(nullptr, TEXT("/Game/Blueprints/Game/UI/W_SelectionMarquee.W_SelectionMarquee_C"));
		ensureAlways(MarqueeClass);
		SelectionMarquee = CreateWidget<UWpSelectionMarqueeWidget>(GetPlayerController(), MarqueeClass);
		SelectionMarquee->AddToViewport(20);
	}

	if (!OrderPreview)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		OrderPreview = GetWorld()->SpawnActor<AWpCommandMarker>(AWpCommandMarker::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
		if (OrderPreview)
		{
			OrderPreview->HidePreview();
		}
	}
}

void UWpOrderSenderComponent::UpdateSelectionMarquee()
{
	EnsureOrderVisuals();
	if (!bSelectingEntities)
	{
		SelectionMarquee->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const FVector2D Cursor = UWidgetLayoutLibrary::GetMousePositionOnViewport(this);
	SelectionMarquee->SetScreenRect(SelectEntitiesStartScreen, Cursor);
}

void UWpOrderSenderComponent::UpdateOrderPreview()
{
	if (!OrderPreview)
	{
		EnsureOrderVisuals();
	}

	if ((OrderSenderState == EWpOrderSenderState::ProduceBuildingOrder) || OrderStartLocation.IsNearlyZero() || SelectedEntities.IsEmpty())
	{
		if (OrderPreview)
		{
			OrderPreview->HidePreview();
		}
		return;
	}

	const FHitResult Hit = LineTraceUnderCursor();
	if (!Hit.bBlockingHit)
	{
		OrderPreview->HidePreview();
		return;
	}

	EWpOrderType OrderType = SelectedOrderType;
	if (OrderType == EWpOrderType::Undefined)
	{
		OrderType = SelectedEntitiesDefaultOrders.Get(GetBelligerentId(), Hit.GetActor());
	}

	float Radius = FVector::Dist2D(Hit.Location, OrderStartLocation);
	if (Radius < 100.0f)
	{
		Radius = 0.0f;
	}

	const bool bPatrol = OrderType == EWpOrderType::Patrol;
	const FVector Center = bPatrol ? OrderStartLocation : FVector{Hit.Location};
	const float ShownRadius = (Radius > 0.0f) ? Radius : 120.0f;
	OrderPreview->Preview(Center, ShownRadius, AWpCommandMarker::ColorForOrder(OrderType));
}

void UWpOrderSenderComponent::ShowOrderMarker(const FVector& Location, const float Radius, const EWpOrderType OrderType)
{
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AWpCommandMarker* const Marker = GetWorld()->SpawnActor<AWpCommandMarker>(AWpCommandMarker::StaticClass(), Location, FRotator::ZeroRotator, Params);
	if (Marker)
	{
		Marker->Play(Location, Radius, AWpCommandMarker::ColorForOrder(OrderType));
	}
}

bool UWpOrderSenderComponent::ShouldShowOrderMarker(const EWpOrderType OrderType)
{
	switch (OrderType)
	{
	case EWpOrderType::Undefined:
	case EWpOrderType::Stop:
	case EWpOrderType::ProduceUnit:
	case EWpOrderType::ProductionProxy:
	case EWpOrderType::InfiniteUnitProductionOff:
	case EWpOrderType::InfiniteUnitProductionOn:
	case EWpOrderType::SelfDestruct:
		return false;
	default:
		return true;
	}
}

APlayerController* UWpOrderSenderComponent::GetPlayerController() const
{
	return GetOwner<APlayerController>();
}

uint32 UWpOrderSenderComponent::GetBelligerentId() const
{
	const AWpGamePlayerState* const PlayerState = GetPlayerController()->GetPlayerState<AWpGamePlayerState>();
	return PlayerState->GetBelligerentId();
}

uint32 UWpOrderSenderComponent::GetTeamId() const
{
	const AWpGamePlayerState* const PlayerState = GetPlayerController()->GetPlayerState<AWpGamePlayerState>();
	return PlayerState->GetTeamId();
}

void UWpOrderSenderComponent::OnSelectedEntityStateChanged(AWpEntity* const SelectedEntity, const EWpEntityState EntityState)
{
	if (EntityState == EWpEntityState::Wreck)
	{
		SelectEntity(SelectedEntity, false);

		UpdateAvailableOrderTypes();
	}
}
