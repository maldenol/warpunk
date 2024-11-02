#include "WpEndurance.h"

#include "Warpunk/Game/Data/WpEntityConfig.h"
#include "Warpunk/Game/Data/WpPooledEntityData.h"
#include "Warpunk/Game/EntityComponent/WpEntityComponent.h"
#include "Warpunk/Game/GameMode/WpGameGameState.h"
#include "Warpunk/Game/Pool/WpActorPool.h"
#include "Warpunk/Game/UI/WpEntityEnduranceWidget.h"

#include "Components/WidgetComponent.h"

const FName FWpEndurance::EnduranceWidgetComponentName = TEXT("EnduranceWidget");

void FWpEndurance::OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const void* const CustomData)
{
	if (bIsAcquired)
	{
		const FWpPooledEntityData* const PooledEntityData = static_cast<const FWpPooledEntityData*>(CustomData);
		const FWpEnduranceConfig& EnduranceConfig = PooledEntityData->EntityConfig->EnduranceConfig;

		ensureAlways(EnduranceConfig.MaxEndurance >= 1.0f);
		ensureAlways(EnduranceConfig.MatterCost >= 1.0f);

		MaxEndurance = EnduranceConfig.MaxEndurance;
		MatterCost = EnduranceConfig.MatterCost;
		EntityState = EWpEntityState::NotProduced;
		CurrentEndurance = 0.0f;
	}
}

void FWpEndurance::Initialize(UWpEntityComponent* const InOwnerEntityComponent)
{
	OwnerEntityComponent = InOwnerEntityComponent;

	OwnerEntityComponent->OnDamageReceived.AddRaw(this, &FWpEndurance::ReceiveDamage);

	EnduranceWidgetComponent = NewObject<UWidgetComponent>(OwnerEntityComponent->GetOwner(), UWidgetComponent::StaticClass(), EnduranceWidgetComponentName);
	EnduranceWidgetComponent->RegisterComponent();
	EnduranceWidgetComponent->AttachToComponent(OwnerEntityComponent->GetOwner()->GetRootComponent(), FAttachmentTransformRules{EAttachmentRule::KeepRelative, false});

	EnduranceWidgetComponent->SetCollisionProfileName(TEXT("NoCollision"));
	EnduranceWidgetComponent->SetNotifyRigidBodyCollision(false);
	EnduranceWidgetComponent->SetGenerateOverlapEvents(false);

	EnduranceWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	EnduranceWidgetComponent->SetDrawAtDesiredSize(true);

	const TSubclassOf<UUserWidget> EnduranceWidgetClass = OwnerEntityComponent->GetWorld()->GetGameState<AWpGameGameState>()->EnduranceWidgetClass;
	ensureAlways(EnduranceWidgetClass);
	EnduranceWidgetComponent->SetWidgetClass(EnduranceWidgetClass);

	Cast<UWpEntityEnduranceWidget>(EnduranceWidgetComponent->GetWidget())->SetEndurance(this);
}

bool FWpEndurance::CanBeRepaired(const bool bCanProduce) const
{
	const bool bEnduranceIsNotFull = CurrentEndurance < MaxEndurance;
	const bool bIsOperational = EntityState == EWpEntityState::Operational;
	const bool bIsNotButCanBeProduced = (EntityState == EWpEntityState::NotProduced) && bCanProduce;
	return bEnduranceIsNotFull && (bIsOperational || bIsNotButCanBeProduced);
}

void FWpEndurance::ReceiveRepair(const float DeltaMatter)
{
	const float DeltaEndurance = DeltaMatter * MaxEndurance / MatterCost;
	float NewEndurance = CurrentEndurance + DeltaEndurance;

	if (NewEndurance >= MaxEndurance)
	{
		NewEndurance = MaxEndurance;

		if (EntityState == EWpEntityState::NotProduced)
		{
			EntityState = EWpEntityState::Operational;
		}
	}

	SetCurrentEndurance(NewEndurance);
}

void FWpEndurance::ReceiveReclaim(const float DeltaMatter)
{
}

void FWpEndurance::ReceiveDamage(const float DeltaEndurance)
{
	const float NewEndurance = CurrentEndurance - DeltaEndurance;
	if (NewEndurance <= 0.0f)
	{
		SetCurrentEndurance(0.0f);
		Destruct();
	}
	else
	{
		SetCurrentEndurance(NewEndurance);
	}
}

void FWpEndurance::Destruct()
{
	AActor* const OwnerActor = OwnerEntityComponent->GetOwner();

	if (AWpActorPool* const ActorPool = AWpActorPool::Instance(OwnerActor->GetClass()))
	{
		ActorPool->Release(OwnerActor);
	}
	else
	{
		OwnerActor->Destroy();
	}
}

float FWpEndurance::GetMaxEndurance() const
{
	return MaxEndurance;
}

float FWpEndurance::GetMatterCost() const
{
	return MatterCost;
}

EWpEntityState FWpEndurance::GetEntityState() const
{
	return EntityState;
}

void FWpEndurance::SetCurrentEndurance(const float InCurrentEndurance)
{
	CurrentEndurance = InCurrentEndurance;

	OnEnduranceChanged.Broadcast(CurrentEndurance, MaxEndurance);
}

float FWpEndurance::GetCurrentEndurance() const
{
	return CurrentEndurance;
}
