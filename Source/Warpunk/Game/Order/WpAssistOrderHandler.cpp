#include "WpAssistOrderHandler.h"

#include "Warpunk/Game/Entity/WpEntityInterface.h"
#include "Warpunk/Game/Order/WpOrderReceiverComponent.h"

#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"

const FName FWpAssistOrderHandler::MimicOrderReceiverComponentName = TEXT("MimicOrderReceiverComponent");

FWpAssistOrderHandler::FWpAssistOrderHandler()
{
	SetTickCooldown(0.25f);
}

void FWpAssistOrderHandler::Initialize(IWpEntityInterface* const InEntity)
{
	FWpOrderHandler::Initialize(InEntity);

	AIController = Cast<APawn>(InEntity)->GetController<AAIController>();
	ensureAlways(AIController);

	OrderReceiverComponent = InEntity->GetOrderReceiverComponent();
}

void FWpAssistOrderHandler::Reactivate()
{
	FWpOrderHandler::Reactivate();

	if (!IsValid(GetOrder().TargetEntityActor))
	{
		Finish();
		return;
	}

	MimicOrderReceiverComponent = NewObject<UWpOrderReceiverComponent>(Cast<AActor>(GetEntity()), UWpOrderReceiverComponent::StaticClass(), MimicOrderReceiverComponentName);
	MimicOrderReceiverComponent->RegisterComponent();
	MimicOrderReceiverComponent->Activate();
	MimicOrderReceiverComponent->SetDefaultOrders(OrderReceiverComponent->GetDefaultOrders());
	MimicOrderReceiverComponent->SetSupportedOrders(OrderReceiverComponent->GetSupportedOrders());
	MimicOrderReceiverComponent->SetShouldProxyOrders(false);

	UWpOrderReceiverComponent* const TargetEntityOrderReceiverComponent = GetOrder().GetTargetEntity()->GetOrderReceiverComponent();
	if (!OnOrderReceivedDelegateHandle.IsValid())
	{
		OnOrderReceivedDelegateHandle = TargetEntityOrderReceiverComponent->OnOrderReceived().AddRaw(this, &FWpAssistOrderHandler::OnOrderReceived);
	}
	OnOrderReceived(TargetEntityOrderReceiverComponent->GetOrder());
}

void FWpAssistOrderHandler::Tick(const float CurrentTime)
{
	FWpOrderHandler::Tick(CurrentTime);

	if (!bCanAssist)
	{
		static constexpr float Radius = 300.0f;
		const float Distance2 = FVector::DistSquared2D(Cast<AActor>(GetEntity())->GetActorLocation(), GetOrder().TargetEntityActor->GetActorLocation());
		const bool bIsInRange = Distance2 <= Radius * Radius;
		const bool bIsMoving = AIController->GetMoveStatus() != EPathFollowingStatus::Type::Idle;
		if (!bIsInRange && !bIsMoving)
		{
			AIController->MoveToActor(GetOrder().TargetEntityActor, Radius, false, true, false, nullptr, false);
		}
	}
}

void FWpAssistOrderHandler::Deactivate()
{
	if (IsValid(GetOrder().TargetEntityActor))
	{
		GetOrder().GetTargetEntity()->GetOrderReceiverComponent()->OnOrderReceived().Remove(OnOrderReceivedDelegateHandle);
	}
	OnOrderReceivedDelegateHandle.Reset();

	MimicOrderReceiverComponent->DestroyComponent();
	MimicOrderReceiverComponent = nullptr;

	if (IsValid(AIController))
	{
		AIController->StopMovement();
	}

	FWpOrderHandler::Deactivate();
}

void FWpAssistOrderHandler::OnOrderReceived(const FWpOrder& InOrder)
{
	if (!IsValid(GetOrder().TargetEntityActor))
	{
		Finish();
		return;
	}

	if (!bCanAssist)
	{
		AIController->StopMovement();
		bCanAssist = true;
	}

	if (MimicOrderReceiverComponent->ReceiveOrder(InOrder, true))
	{
		return;
	}

	FWpOrder RepairOrder;
	RepairOrder.Type = EWpOrderType::Repair;
	RepairOrder.TargetEntityActor = GetOrder().TargetEntityActor;
	if (MimicOrderReceiverComponent->ReceiveOrder(RepairOrder, true))
	{
		return;
	}

	bCanAssist = false;
}
