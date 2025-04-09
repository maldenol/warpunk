#include "WpActorPool.h"

#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"

const float AWpActorPool::AdditionalActorSpawnDelay = 0.5f;

AWpActorPool::AWpActorPool()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.SetTickFunctionEnable(false);

	bReplicates = false;
}

void AWpActorPool::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ensureAlways(HasAuthority());

	if (AdditionalActorSpawnNum == 0)
	{
		AdditionalActorSpawnTime = 0.0f;
		SetActorTickEnabled(false);
		return;
	}

	AdditionalActorSpawnTime += DeltaSeconds;
	if (AdditionalActorSpawnTime > AdditionalActorSpawnDelay)
	{
		SpawnActors(1);

		--AdditionalActorSpawnNum;
		AdditionalActorSpawnTime = 0.0f;

		if (AdditionalActorSpawnNum == 0)
		{
			SetActorTickEnabled(false);
		}
	}
}

void AWpActorPool::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Instances.Remove(FWpInstanceKey{GetWorld(), Class});

	Super::EndPlay(EndPlayReason);
}

void AWpActorPool::Reserve(const TSubclassOf<AActor>& InClass, const uint32 Number)
{
	ensureAlways(HasAuthority());

	ensureAlways(InClass);
	ensureAlways(Number > 0);

	Class = InClass;

	ensureAlways(!Instances.Contains(FWpInstanceKey{GetWorld(), Class}));
	Instances.Add(FWpInstanceKey{GetWorld(), Class}, this);

	AllActors.Reserve(Number);
	FreeActors.Reserve(Number);
	SpawnActors(Number);
}

AActor* AWpActorPool::Acquire(const FVector& Location, const FRotator& Rotation, const FWpPooledObjectData& PooledObjectData)
{
	ensureAlways(HasAuthority());

	if (FreeActors.IsEmpty())
	{
		BeginSpawnAdditionalActors();
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
	ensureAlways(AllActors.Contains(Actor));

	Cast<IWpPooledObjectInterface>(Actor)->ReleasePooledObject();

	Actor->SetActorLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator, false);

	FreeActors.Add(Actor);

	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.0f, FColor::Red, Actor->GetActorNameOrLabel());
}

void AWpActorPool::SpawnActors(const uint32 Number)
{
	FActorSpawnParameters ActorSpawnParameters;
	ActorSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (uint32 Index = 0; Index < Number; ++Index)
	{
		AActor* const Actor = GetWorld()->SpawnActor(Class, nullptr, nullptr, ActorSpawnParameters);

		Cast<IWpPooledObjectInterface>(Actor)->ReleasePooledObject();

		AllActors.Add(Actor);
		FreeActors.Add(Actor);
	}
}

void AWpActorPool::BeginSpawnAdditionalActors()
{
	if (AdditionalActorSpawnNum == 0)
	{
		AdditionalActorSpawnNum = FMath::Max(AllActors.Num() / 2, 2);

		const uint32 NewActorNum = AllActors.Num() + AdditionalActorSpawnNum;
		AllActors.Reserve(NewActorNum);
		FreeActors.Reserve(NewActorNum);

		SetActorTickEnabled(true);
	}

	--AdditionalActorSpawnNum;
	SpawnActors(1);
}

AWpActorPool* AWpActorPool::Instance(const UWorld* const World, const TSubclassOf<AActor>& Class)
{
	ensureAlways(World);
	ensureAlways(Class);

	if (AWpActorPool** const Ptr = Instances.Find(FWpInstanceKey{World, Class}))
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

TMap<AWpActorPool::FWpInstanceKey, AWpActorPool*> AWpActorPool::Instances = {};
