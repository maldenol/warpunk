#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Info.h"

#include "WpActorPool.generated.h"

struct FWpPooledObjectData;

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class AWpActorPool final : public AInfo
{
	GENERATED_BODY()

public:
	AWpActorPool();

	virtual void Tick(const float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void Reserve(const TSubclassOf<AActor>& InClass, const uint32 Number);

	AActor* Acquire(const FVector& Location, const FRotator& Rotation, const FWpPooledObjectData& PooledObjectData);
	void Release(AActor* const Actor);

private:
	void SpawnActors(const uint32 Number);
	void BeginSpawnAdditionalActors();

	TSubclassOf<AActor> Class = nullptr;

	TSet<AActor*> AllActors = {};
	TSet<AActor*> FreeActors = {};

	uint32 AdditionalActorSpawnNum = 0;
	float AdditionalActorSpawnTime = 0.0f;

public:
	static AWpActorPool* Instance(const UWorld* const World, const TSubclassOf<AActor>& Class);

	static AActor* Acquire(const UWorld* const World, const TSubclassOf<AActor>& Class, const FVector& Location, const FRotator& Rotation, const FWpPooledObjectData& PooledObjectData);
	static void Release(const TSubclassOf<AActor>& Class, AActor* const Actor);

	template <class T>
	static T* Acquire(const UWorld* const World, const FVector& Location, const FRotator& Rotation, const FWpPooledObjectData& PooledObjectData)
	{
		return Cast<T>(Acquire(World, T::StaticClass(), Location, Rotation, PooledObjectData));
	}

	template <class T>
	static void Release(AActor* const Actor)
	{
		Release(T::StaticClass(), Actor);
	}

	static const float AdditionalActorSpawnDelay;

private:
	using FWpInstanceKey = TPair<const UWorld*, TSubclassOf<AActor>>;

	static TMap<FWpInstanceKey, AWpActorPool*> Instances;
};
