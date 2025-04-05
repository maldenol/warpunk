#include "WpEnduranceComponent.h"

#include "Warpunk/Game/Entity/WpEntity.h"
#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Entity/WpPooledEntityData.h"
#include "Warpunk/Game/Pool/WpActorPool.h"
#include "Warpunk/Game/UI/WpGameEntityEnduranceWidget.h"
#include "Warpunk/Global/GameInstance/WpGameInstance.h"

#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"

const FName UWpEnduranceComponent::EnduranceWidgetComponentName = TEXT("EnduranceWidget");

UWpEnduranceComponent::UWpEnduranceComponent()
{
	bAutoActivate = false;
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
	EnduranceWidgetComponent->AttachToComponent(GetOwner<AWpEntity>()->GetMeshBaseComponent(), FAttachmentTransformRules{EAttachmentRule::KeepRelative, false});

	EnduranceWidgetComponent->SetIsReplicated(false);
	EnduranceWidgetComponent->SetCollisionProfileName(TEXT("NoCollision"));
	EnduranceWidgetComponent->SetNotifyRigidBodyCollision(false);
	EnduranceWidgetComponent->SetGenerateOverlapEvents(false);

	EnduranceWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	EnduranceWidgetComponent->SetDrawAtDesiredSize(true);

	const TSubclassOf<UWpGameEntityEnduranceWidget>& EnduranceWidgetClass = GetWorld()->GetGameInstance<UWpGameInstance>()->GetEnduranceWidgetClass();
	ensureAlways(EnduranceWidgetClass);
	EnduranceWidgetComponent->SetWidgetClass(EnduranceWidgetClass);

	Cast<UWpGameEntityEnduranceWidget>(EnduranceWidgetComponent->GetWidget())->SetEnduranceComponent(this);

	EnduranceWidgetComponent->SetVisibility(false);
}

void UWpEnduranceComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DISABLE_REPLICATED_PRIVATE_PROPERTY(UActorComponent, bIsActive);

	DOREPLIFETIME(UWpEnduranceComponent, EntityState);
	DOREPLIFETIME(UWpEnduranceComponent, CurrentEndurance);
}

void UWpEnduranceComponent::OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
{
	if (!bIsAcquired)
	{
		ensureAlways(GetOwner()->HasAuthority());
	}

	if (bIsAcquired)
	{
		const FWpPooledEntityData& PooledEntityData = static_cast<const FWpPooledEntityData&>(PooledObjectData);
		const FWpEnduranceConfig& EnduranceConfig = PooledEntityData.EntityConfig->EnduranceConfig;

		SetMaxEndurance(EnduranceConfig.MaxEndurance);
		MatterCost = EnduranceConfig.MatterCost;

		if (GetOwner()->HasAuthority())
		{
			SetEntityState(EWpEntityState::NotProduced);
			SetCurrentEndurance(0.0f);
		}
	}

	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	SetActive(bIsAcquired);
}

void UWpEnduranceComponent::OnPooledObjectVisibilityChanged(const bool bIsVisible)
{
	if (!GetOwner()->HasAuthority())
	{
		SetActive(bIsVisible);
	}

	if (EnduranceWidgetComponent)
	{
		EnduranceWidgetComponent->SetVisibility(bIsVisible);
	}
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

	AWpActorPool* const ActorPool = AWpActorPool::Instance(GetWorld(), GetOwner()->GetClass());
	ActorPool->Release(GetOwner());
}

void UWpEnduranceComponent::SetMaxEndurance(const float InMaxEndurance)
{
	MaxEndurance = InMaxEndurance;
	OnRep_CurrentEndurance();
}

float UWpEnduranceComponent::GetMaxEndurance() const
{
	return MaxEndurance;
}

float UWpEnduranceComponent::GetMatterCost() const
{
	return MatterCost;
}

EWpEntityState UWpEnduranceComponent::GetEntityState() const
{
	return EntityState;
}

void UWpEnduranceComponent::SetCurrentEndurance(const float InCurrentEndurance)
{
	ensureAlways(GetOwner()->HasAuthority());

	CurrentEndurance = InCurrentEndurance;
	OnRep_CurrentEndurance();
}

float UWpEnduranceComponent::GetCurrentEndurance() const
{
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
	EntityStateChangedEvent.Broadcast(GetOwner<AWpEntity>(), EntityState);
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
