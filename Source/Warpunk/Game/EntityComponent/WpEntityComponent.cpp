#include "WpEntityComponent.h"

#include "Warpunk/Game/Data/WpPooledEntityData.h"
#include "Warpunk/Game/EntityComponent/WpEntityInterface.h"

UWpEntityComponent::UWpEntityComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UWpEntityComponent::BeginPlay()
{
	Super::BeginPlay();

	GetOwner()->OnTakeAnyDamage.AddDynamic(this, &UWpEntityComponent::OnTakeAnyDamage);
}

void UWpEntityComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* const ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	OrderReceiver.Tick();
	Production.Tick(DeltaTime);
	WeaponArray.Tick(DeltaTime);
}

void UWpEntityComponent::OnPooledObjectInitialized()
{
	Endurance.Initialize(this);
	OrderReceiver.Initialize(this);
	Intel.Initialize(this);
	Production.Initialize(this);
	WeaponArray.Initialize(this);
}

void UWpEntityComponent::OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const void* const CustomData)
{
	if (bIsAcquired)
	{
		const FWpPooledEntityData* const PooledEntityData = static_cast<const FWpPooledEntityData*>(CustomData);

		ensureAlways(PooledEntityData->EntityConfigId > 0);
		ensureAlways(PooledEntityData->BelligerentId > 0);

		EntityConfigId = PooledEntityData->EntityConfigId;
		BelligerentMember.SetBelligerentId(PooledEntityData->BelligerentId);
	}

	SetComponentTickEnabled(bIsAcquired);
}

TArray<IWpPooledObjectInterface*> UWpEntityComponent::GetInnerPooledObjects()
{
	return
	{
		static_cast<IWpPooledObjectInterface*>(&Endurance),
		static_cast<IWpPooledObjectInterface*>(&OrderReceiver),
		static_cast<IWpPooledObjectInterface*>(&Intel),
		static_cast<IWpPooledObjectInterface*>(&Production),
		static_cast<IWpPooledObjectInterface*>(&WeaponArray),
	};
}

UWpEntityComponent::operator FWpBelligerentMember&()
{
	return BelligerentMember;
}

UWpEntityComponent::operator const FWpBelligerentMember&() const
{
	return BelligerentMember;
}

UWpEntityComponent::operator FWpEndurance&()
{
	return Endurance;
}

UWpEntityComponent::operator const FWpEndurance&() const
{
	return Endurance;
}

UWpEntityComponent::operator FWpOrderReceiver&()
{
	return OrderReceiver;
}

UWpEntityComponent::operator const FWpOrderReceiver&() const
{
	return OrderReceiver;
}

UWpEntityComponent::operator FWpIntel&()
{
	return Intel;
}

UWpEntityComponent::operator const FWpIntel&() const
{
	return Intel;
}

UWpEntityComponent::operator FWpProduction&()
{
	return Production;
}

UWpEntityComponent::operator const FWpProduction&() const
{
	return Production;
}

UWpEntityComponent::operator FWpWeaponArray&()
{
	return WeaponArray;
}

UWpEntityComponent::operator const FWpWeaponArray&() const
{
	return WeaponArray;
}

uint32 UWpEntityComponent::GetEntityConfigId() const
{
	return EntityConfigId;
}

void UWpEntityComponent::OnIntelVolumeBeginOverlap(UPrimitiveComponent* const OverlappedComponent, AActor* const OtherActor, UPrimitiveComponent* const OtherComp, const int32 OtherBodyIndex, const bool bFromSweep, const FHitResult& SweepResult)
{
	IWpEntityInterface* const Entity = Cast<IWpEntityInterface>(OtherActor);
	if (Entity && OtherActor != GetOwner())
	{
		Intel.AddIntel(Entity->GetEntityComponent());
	}
}

void UWpEntityComponent::OnIntelVolumeEndOverlap(UPrimitiveComponent* const OverlappedComponent, AActor* const OtherActor, UPrimitiveComponent* const OtherComp, const int32 OtherBodyIndex)
{
	IWpEntityInterface* const Entity = Cast<IWpEntityInterface>(OtherActor);
	if (Entity && OtherActor != GetOwner())
	{
		Intel.RemoveIntel(Entity->GetEntityComponent());
	}
}

UWpEntityComponent::FDamageReceivedEvent& UWpEntityComponent::OnDamageReceived()
{
	return DamageReceivedEvent;
}

void UWpEntityComponent::OnTakeAnyDamage(AActor* const DamagedActor, const float Damage, const UDamageType* const DamageType, AController* const InstigatedBy, AActor* const DamageCauser)
{
	DamageReceivedEvent.Broadcast(Damage);
}
