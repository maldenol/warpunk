#include "WpOrderReceiverComponent.h"

#include "Warpunk/Game/Endurance/WpEnduranceComponent.h"
#include "Warpunk/Game/Entity/WpEntity.h"
#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Entity/WpPooledEntityData.h"
#include "Warpunk/Global/GameInstance/WpGameInstance.h"
#include "Warpunk/Game/Order/WpCommandMarker.h"

#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"

const FName UWpOrderReceiverComponent::SelectionWidgetComponentName = TEXT("SelectionWidget");

const TSet<EWpOrderType> UWpOrderReceiverComponent::ImmediateOrders =
{
	EWpOrderType::Assist,
	EWpOrderType::InfiniteUnitProductionOff,
	EWpOrderType::InfiniteUnitProductionOn,
	EWpOrderType::SelfDestruct,
};

const TSet<EWpOrderType> UWpOrderReceiverComponent::ProductionProxyOrders =
{
	EWpOrderType::Move,
	EWpOrderType::Patrol,
	EWpOrderType::Assist,
	EWpOrderType::Attack,
	EWpOrderType::Repair,
	EWpOrderType::Reclaim,
};

UWpOrderReceiverComponent::UWpOrderReceiverComponent()
{
	bAutoActivate = false;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetIsReplicatedByDefault(true);
}

void UWpOrderReceiverComponent::BeginPlay()
{
	Super::BeginPlay();

	CreateSelectionWidgetComponentIfNull();
}

void UWpOrderReceiverComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* const ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	if (GetOwner<AWpEntity>()->GetEnduranceComponent()->GetEntityState() != EWpEntityState::Operational)
	{
		return;
	}

	if (FWpOrderHandler* const OrderHandler = CurrentOrderHandler.Get())
	{
		if (!OrderHandler->IsRunning())
		{
			OrderHandler->Reactivate();
		}
		else if (OrderHandler->HasFinished())
		{
			OrderHandler->Deactivate();
			CurrentOrderHandler.Reset();
		}
		else
		{
			const float CurrentTime = GetWorld()->GetTimeSeconds();
			if (OrderHandler->ShouldTick(CurrentTime))
			{
				OrderHandler->Tick(CurrentTime);
			}
		}
	}

	if (!CurrentOrderHandler.Get())
	{
		FWpOrder Order;
		if (OrderQueue.Dequeue(Order))
		{
			ActivateOrder(Order);
		}
	}
}

void UWpOrderReceiverComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DISABLE_REPLICATED_PRIVATE_PROPERTY(UActorComponent, bIsActive);

	DOREPLIFETIME(UWpOrderReceiverComponent, UnitToProduceConfigIdNumbers);
}

void UWpOrderReceiverComponent::OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
{
	if (!bIsAcquired)
	{
		ensureAlways(GetOwner()->HasAuthority());
	}

	CreateSelectionWidgetComponentIfNull();

	if (bIsAcquired)
	{
		const FWpPooledEntityData& PooledEntityData = static_cast<const FWpPooledEntityData&>(PooledObjectData);
		const FWpOrderReceiverConfig* const OrderReceiverConfig = PooledEntityData.EntityConfig->OrderReceiverConfig.GetPtrOrNull();

		if (OrderReceiverConfig)
		{
			DefaultOrders = OrderReceiverConfig->DefaultOrders;
			SupportedOrders = OrderReceiverConfig->SupportedOrders.Array();
			bShouldProxyOrders = OrderReceiverConfig->bShouldProxyOrders;
		}

		bIsAcquired = OrderReceiverConfig != nullptr;
	}

	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	if (!bIsAcquired)
	{
		if (FWpOrderHandler* const OrderHandler = CurrentOrderHandler.Get())
		{
			DeactivateOrder(OrderHandler);
		}

		OrderQueue.Empty();

		Select(false);
	}

	SelectionWidgetComponent->SetVisibility(false);

	SetActive(bIsAcquired);
}

void UWpOrderReceiverComponent::OnPooledObjectVisibilityChanged(const bool bIsVisible)
{
	if (!GetOwner()->HasAuthority())
	{
		SetActive(bIsVisible);

		if (bIsVisible)
		{
			SetComponentTickEnabled(false);
		}
	}
}

void UWpOrderReceiverComponent::ReceiveOrder(const FWpOrder& Order, const bool bOverride)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	if (GetOwner<AWpEntity>()->GetEnduranceComponent()->GetEntityState() == EWpEntityState::Wreck)
	{
		return;
	}

	const bool bIsProxyOrder = bShouldProxyOrders && ProductionProxyOrders.Contains(Order.Type);

	if (Order.Type == EWpOrderType::Stop)
	{
		OrderQueue.Empty();

		if (FWpOrderHandler* const OrderHandler = CurrentOrderHandler.Get())
		{
			DeactivateOrder(OrderHandler);
		}

		if (!bIsProxyOrder)
		{
			OrderReceivedEvent.Broadcast(Order);
		}
		return;
	}

	if (!SupportedOrders.Contains(Order.Type))
	{
		return;
	}

	if (Order.TargetEntity == GetOwner<AWpEntity>())
	{
		if (!bIsProxyOrder)
		{
			OrderReceivedEvent.Broadcast(Order);
		}
		return;
	}

	if (ImmediateOrders.Contains(Order.Type))
	{
		ExecuteOrder(Order.Type, Order);
	}
	else if (bIsProxyOrder)
	{
		ExecuteOrder(EWpOrderType::ProductionProxy, Order);
	}
	else
	{
		if (bOverride)
		{
			OrderQueue.Empty();
		}

		if (FWpOrderHandler* const OrderHandler = CurrentOrderHandler.Get())
		{
			if (bOverride)
			{
				if (Order.Type == OrderHandler->GetOrder().Type)
				{
					ReactivateOrder(OrderHandler, Order);
				}
				else
				{
					DeactivateOrder(OrderHandler);
					ActivateOrder(Order);
				}
			}
			else
			{
				OrderQueue.Enqueue(Order);
			}
		}
		else
		{
			if (OrderQueue.IsEmpty())
			{
				ActivateOrder(Order);
			}
			else
			{
				OrderQueue.Enqueue(Order);
			}
		}

		OrderReceivedEvent.Broadcast(Order);
	}
}

FWpOrder UWpOrderReceiverComponent::GetOrder() const
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	if (const FWpOrderHandler* const OrderHandler = CurrentOrderHandler.Get())
	{
		return OrderHandler->GetOrder();
	}

	return {};
}

void UWpOrderReceiverComponent::SetDefaultOrders(const FWpDefaultOrders& InDefaultOrders)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	DefaultOrders = InDefaultOrders;
}

const FWpDefaultOrders& UWpOrderReceiverComponent::GetDefaultOrders() const
{
	ensureAlways(IsActive());

	return DefaultOrders;
}

void UWpOrderReceiverComponent::SetSupportedOrders(const TArray<EWpOrderType>& InSupportedOrders)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	SupportedOrders = InSupportedOrders;
}

const TArray<EWpOrderType>& UWpOrderReceiverComponent::GetSupportedOrders() const
{
	ensureAlways(IsActive());

	return SupportedOrders;
}

void UWpOrderReceiverComponent::SetShouldProxyOrders(const bool bInShouldProxyOrders)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	bShouldProxyOrders = bInShouldProxyOrders;
}

void UWpOrderReceiverComponent::Select(const bool bSelected)
{
	if (bSelected)
	{
		ensureAlways(IsActive());
		UpdateSelectionRing();
	}

	if (SelectionRing)
	{
		SelectionRing->SetVisibility(bSelected);
		SelectionRing->SetHiddenInGame(!bSelected);
	}

	if (SelectionWidgetComponent)
	{
		SelectionWidgetComponent->SetVisibility(false);
	}
}

void UWpOrderReceiverComponent::SetUnitToProduceConfigIdNumbers(const TArray<FWpConfigIdNumber>& InUnitToProduceConfigIdNumbers)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	UnitToProduceConfigIdNumbers = InUnitToProduceConfigIdNumbers;
}

const TArray<FWpConfigIdNumber>& UWpOrderReceiverComponent::GetUnitToProduceConfigIdNumbers() const
{
	ensureAlways(IsActive());

	return UnitToProduceConfigIdNumbers;
}

UWpOrderReceiverComponent::FOrderReceivedEvent& UWpOrderReceiverComponent::OnOrderReceived()
{
	ensureAlways(GetOwner()->HasAuthority());

	return OrderReceivedEvent;
}

void UWpOrderReceiverComponent::ActivateOrder(const FWpOrder& Order)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	FWpOrderHandler* const OrderHandler = CurrentOrderHandler.Set(Order.Type);
	OrderHandler->Initialize(GetOwner<AWpEntity>());
	OrderHandler->SetOrder(Order);
	if (GetOwner<AWpEntity>()->GetEnduranceComponent()->GetEntityState() == EWpEntityState::Operational)
	{
		OrderHandler->Reactivate();
	}
}

void UWpOrderReceiverComponent::ReactivateOrder(FWpOrderHandler* const OrderHandler, const FWpOrder& Order)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	OrderHandler->SetOrder(Order);
	if (GetOwner<AWpEntity>()->GetEnduranceComponent()->GetEntityState() == EWpEntityState::Operational)
	{
		OrderHandler->Reactivate();
	}
}

void UWpOrderReceiverComponent::DeactivateOrder(FWpOrderHandler* const OrderHandler)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	if (OrderHandler->IsRunning())
	{
		OrderHandler->Deactivate();
	}
	CurrentOrderHandler.Reset();
}

void UWpOrderReceiverComponent::ExecuteOrder(const EWpOrderType OrderType, const FWpOrder& Order)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	FWpOrderHandlerWrapper ImmediateOrderHandler;
	FWpOrderHandler* const OrderHandler = ImmediateOrderHandler.Set(OrderType);
	OrderHandler->Initialize(GetOwner<AWpEntity>());
	OrderHandler->Execute(Order);
}

void UWpOrderReceiverComponent::CreateSelectionWidgetComponentIfNull()
{
	if (SelectionWidgetComponent)
	{
		return;
	}

	SelectionWidgetComponent = NewObject<UWidgetComponent>(GetOwner(), UWidgetComponent::StaticClass(), SelectionWidgetComponentName);
	SelectionWidgetComponent->RegisterComponent();
	SelectionWidgetComponent->AttachToComponent(GetOwner<AWpEntity>()->GetMeshBaseComponent(), FAttachmentTransformRules{EAttachmentRule::KeepRelative, false});

	SelectionWidgetComponent->SetIsReplicated(false);
	SelectionWidgetComponent->SetCollisionProfileName(TEXT("NoCollision"));
	SelectionWidgetComponent->SetNotifyRigidBodyCollision(false);
	SelectionWidgetComponent->SetGenerateOverlapEvents(false);

	SelectionWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	SelectionWidgetComponent->SetDrawAtDesiredSize(true);

	const TSubclassOf<UUserWidget>& SelectionWidgetClass = GetWorld()->GetGameInstance<UWpGameInstance>()->GetSelectionWidgetClass();
	ensureAlways(SelectionWidgetClass);
	SelectionWidgetComponent->SetWidgetClass(SelectionWidgetClass);

	SelectionWidgetComponent->SetVisibility(false);

	if (SelectionRing)
	{
		return;
	}

	UStaticMesh* const Plane = AWpCommandMarker::MarkerMesh();
	UMaterialInterface* const MaterialInterface = AWpCommandMarker::MarkerMaterial();
	if (!Plane || !MaterialInterface)
	{
		return;
	}

	SelectionRing = NewObject<UStaticMeshComponent>(GetOwner(), UStaticMeshComponent::StaticClass(), TEXT("SelectionRing"));
	SelectionRing->RegisterComponent();
	SelectionRing->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules{EAttachmentRule::KeepRelative, false});
	SelectionRing->SetMobility(EComponentMobility::Movable);
	SelectionRing->SetStaticMesh(Plane);
	SelectionRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SelectionRing->SetCastShadow(false);
	SelectionRing->SetCanEverAffectNavigation(false);
	SelectionRing->SetGenerateOverlapEvents(false);
	SelectionRing->SetVisibility(false);
	SelectionRing->SetHiddenInGame(true);

	SelectionRingMaterial = UMaterialInstanceDynamic::Create(MaterialInterface, SelectionRing);
	SelectionRingMaterial->SetTextureParameterValue(TEXT("Texture"), AWpCommandMarker::RingTexture());
	SelectionRing->SetMaterial(0, SelectionRingMaterial);
}

void UWpOrderReceiverComponent::UpdateSelectionRing()
{
	if (!SelectionRing)
	{
		return;
	}

	const AWpEntity* const Entity = GetOwner<AWpEntity>();
	const FVector Extent = Entity->GetCollisionComponent()->GetCollisionShape().GetExtent();
	const float Diameter = FMath::Max(FMath::Max(Extent.X, Extent.Y) * 2.0f * 1.45f, 160.0f);
	SelectionRing->SetRelativeScale3D(FVector{Diameter / 100.0f, Diameter / 100.0f, 1.0f});
	SelectionRing->SetRelativeLocation(FVector{0.0f, 0.0f, -Extent.Z + 12.0f});

	const FLinearColor Color{1.0f, 0.86f, 0.22f, 1.0f};
	if (SelectionRingMaterial)
	{
		SelectionRingMaterial->SetVectorParameterValue(TEXT("Color"), Color * 0.35f);
		SelectionRingMaterial->SetScalarParameterValue(TEXT("Opacity"), 0.35f);
	}
}
