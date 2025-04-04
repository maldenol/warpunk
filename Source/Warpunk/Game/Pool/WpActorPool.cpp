#include "WpActorPool.h"

#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"

AWpActorPool::AWpActorPool()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = false;
}

void AWpActorPool::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Instances.Remove(FWpActorPoolInstanceKey{GetWorld(), Class});

	Super::EndPlay(EndPlayReason);
}

void AWpActorPool::Reserve(const TSubclassOf<AActor>& InClass, const uint32 Number)
{
	ensureAlways(HasAuthority());

	ensureAlways(InClass);
	ensureAlways(Number > 0);

	Class = InClass;

	Instances.Add(FWpActorPoolInstanceKey{GetWorld(), Class}, this);

	FreeActors.Reserve(Number);

	FActorSpawnParameters ActorSpawnParameters;
	ActorSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (uint32 Index = 0; Index < Number; ++Index)
	{
		AActor* const Actor = GetWorld()->SpawnActor(Class, nullptr, nullptr, ActorSpawnParameters);

		Cast<IWpPooledObjectInterface>(Actor)->ReleasePooledObject();

		FreeActors.Add(Actor);
	}
}

AActor* AWpActorPool::Acquire(const FVector& Location, const FRotator& Rotation, const FWpPooledObjectData& PooledObjectData)
{
	ensureAlways(HasAuthority());

	if (FreeActors.IsEmpty())
	{
		return nullptr;
	}

	AActor* const Actor = *FreeActors.CreateIterator();
	FreeActors.Remove(Actor);

	Actor->SetActorLocationAndRotation(Location, Rotation, false);

	Cast<IWpPooledObjectInterface>(Actor)->AcquirePooledObject(PooledObjectData);

	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.0f, FColor::Green, Actor->GetActorNameOrLabel());

	return Actor;
}

void AWpActorPool::Release(AActor* const Actor)
{
	ensureAlways(HasAuthority());

	ensureAlways(Actor);

	Cast<IWpPooledObjectInterface>(Actor)->ReleasePooledObject();

	Actor->SetActorLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator, false);

	FreeActors.Add(Actor);

	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.0f, FColor::Red, Actor->GetActorNameOrLabel());
}

AWpActorPool* AWpActorPool::Instance(const UWorld* const World, const TSubclassOf<AActor>& Class)
{
	ensureAlways(World);
	ensureAlways(Class);

	if (AWpActorPool** const Ptr = Instances.Find(FWpActorPoolInstanceKey{World, Class}))
	{
		return *Ptr;
	}

	return nullptr;
}

AActor* AWpActorPool::Acquire(const UWorld* const World, const TSubclassOf<AActor>& Class, const FVector& Location, const FRotator& Rotation, const FWpPooledObjectData& PooledObjectData)
{
	return Instance(World, Class)->Acquire(Location, Rotation, PooledObjectData);
}

void AWpActorPool::Release(const TSubclassOf<AActor>& Class, AActor* const Actor)
{
	Instance(Actor->GetWorld(), Class)->Release(Actor);
}

TMap<AWpActorPool::FWpActorPoolInstanceKey, AWpActorPool*> AWpActorPool::Instances = {};
