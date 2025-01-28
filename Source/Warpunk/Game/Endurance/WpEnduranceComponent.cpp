#include "WpEnduranceComponent.h"

#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Entity/WpEntityInterface.h"
#include "Warpunk/Game/Entity/WpPooledEntityData.h"
#include "Warpunk/Game/Pool/WpActorPool.h"
#include "Warpunk/Game/UI/WpGameEntityEnduranceWidget.h"
#include "Warpunk/Global/GameInstance/WpGameInstance.h"

#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"

const FName UWpEnduranceComponent::EnduranceWidgetComponentName = TEXT("EnduranceWidget");

UWpEnduranceComponent::UWpEnduranceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
}

void UWpEnduranceComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner()->HasAuthority())
	{
		GetOwner()->OnTakeAnyDamage.AddDynamic(this, &UWpEnduranceComponent::OnTakeAnyDamage);
	}

	EnduranceWidgetComponent = NewObject<UWidgetComponent>(GetOwner(), UWidgetComponent::StaticClass(), EnduranceWidgetComponentName);
	EnduranceWidgetComponent->RegisterComponent();
	EnduranceWidgetComponent->AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules{EAttachmentRule::KeepRelative, false});

	EnduranceWidgetComponent->SetCollisionProfileName(TEXT("NoCollision"));
	EnduranceWidgetComponent->SetNotifyRigidBodyCollision(false);
	EnduranceWidgetComponent->SetGenerateOverlapEvents(false);

	EnduranceWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	EnduranceWidgetComponent->SetDrawAtDesiredSize(true);

	const TSubclassOf<UWpGameEntityEnduranceWidget>& EnduranceWidgetClass = GetWorld()->GetGameInstance<UWpGameInstance>()->EnduranceWidgetClass;
	ensureAlways(EnduranceWidgetClass);
	EnduranceWidgetComponent->SetWidgetClass(EnduranceWidgetClass);

	Cast<UWpGameEntityEnduranceWidget>(EnduranceWidgetComponent->GetWidget())->SetEnduranceComponent(this);
}

void UWpEnduranceComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UWpEnduranceComponent, MaxEndurance);
	DOREPLIFETIME(UWpEnduranceComponent, EntityState);
	DOREPLIFETIME(UWpEnduranceComponent, CurrentEndurance);
}

void UWpEnduranceComponent::OnRep_IsActive()
{
	// Override to not enable tick
}

void UWpEnduranceComponent::OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
{
	ensureAlways(GetOwner()->HasAuthority());

	if (bIsAcquired)
	{
		const FWpPooledEntityData& PooledEntityData = static_cast<const FWpPooledEntityData&>(PooledObjectData);
		const FWpEnduranceConfig& EnduranceConfig = PooledEntityData.EntityConfig->EnduranceConfig;

		ensureAlways(EnduranceConfig.MaxEndurance >= 1.0f);
		ensureAlways(EnduranceConfig.MatterCost >= 1.0f);

		MaxEndurance = EnduranceConfig.MaxEndurance;
		MatterCost = EnduranceConfig.MatterCost;
		SetEntityState(EWpEntityState::NotProduced);
		CurrentEndurance = 0.0f;
	}

	SetActive(bIsAcquired);
}

bool UWpEnduranceComponent::CanBeRepaired(const bool bCanProduce) const
{
	ensureAlways(IsActive());

	const bool bEnduranceIsNotFull = CurrentEndurance < MaxEndurance;
	const bool bIsOperational = EntityState == EWpEntityState::Operational;
	const bool bIsNotButCanBeProduced = (EntityState == EWpEntityState::NotProduced) && bCanProduce;
	return bEnduranceIsNotFull && (bIsOperational || bIsNotButCanBeProduced);
}

void UWpEnduranceComponent::ReceiveRepair(const float DeltaMatter)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	const float DeltaEndurance = DeltaMatter * MaxEndurance / MatterCost;
	float NewEndurance = CurrentEndurance + DeltaEndurance;

	if (NewEndurance >= MaxEndurance)
	{
		NewEndurance = MaxEndurance;

		if (EntityState == EWpEntityState::NotProduced)
		{
			SetEntityState(EWpEntityState::Operational);
		}
	}

	SetCurrentEndurance(NewEndurance);
}

void UWpEnduranceComponent::ReceiveReclaim(const float DeltaMatter)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());
}

void UWpEnduranceComponent::ReceiveDamage(const float DeltaEndurance)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

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

void UWpEnduranceComponent::Destruct()
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	SetEntityState(EWpEntityState::Wreck);

	AActor* const OwnerActor = GetOwner();
	if (AWpActorPool* const ActorPool = AWpActorPool::Instance(OwnerActor->GetClass()))
	{
		ActorPool->Release(OwnerActor);
	}
	else
	{
		OwnerActor->Destroy();
	}
}

float UWpEnduranceComponent::GetMaxEndurance() const
{
	ensureAlways(IsActive());

	return MaxEndurance;
}

float UWpEnduranceComponent::GetMatterCost() const
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	return MatterCost;
}

EWpEntityState UWpEnduranceComponent::GetEntityState() const
{
	return EntityState;
}

void UWpEnduranceComponent::SetCurrentEndurance(const float InCurrentEndurance)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	CurrentEndurance = InCurrentEndurance;
	OnRep_CurrentEndurance();
}

float UWpEnduranceComponent::GetCurrentEndurance() const
{
	ensureAlways(IsActive());

	return CurrentEndurance;
}

UWidgetComponent* UWpEnduranceComponent::GetEnduranceWidgetComponent()
{
	return EnduranceWidgetComponent;
}

UWpEnduranceComponent::FEntityStateChangedEvent& UWpEnduranceComponent::OnEntityStateChanged()
{
	return EntityStateChangedEvent;
}

UWpEnduranceComponent::FCurrentEnduranceChangedEvent& UWpEnduranceComponent::OnCurrentEnduranceChanged()
{
	return CurrentEnduranceChangedEvent;
}

void UWpEnduranceComponent::OnTakeAnyDamage(AActor* const DamagedActor, const float Damage, const UDamageType* const DamageType, AController* const InstigatedBy, AActor* const DamageCauser)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	ReceiveDamage(Damage);
}

void UWpEnduranceComponent::OnRep_EntityState()
{
	EntityStateChangedEvent.Broadcast(Cast<IWpEntityInterface>(GetOwner()), EntityState);
}

void UWpEnduranceComponent::OnRep_CurrentEndurance()
{
	CurrentEnduranceChangedEvent.Broadcast(CurrentEndurance, MaxEndurance);
}

void UWpEnduranceComponent::SetEntityState(const EWpEntityState InEntityState)
{
	ensureAlways(GetOwner()->HasAuthority());

	EntityState = InEntityState;
	OnRep_EntityState();
}
