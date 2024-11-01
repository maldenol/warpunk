#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Info.h"

#include "WpActorPool.generated.h"

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class AWpActorPool final : public AInfo
{
	GENERATED_BODY()

public:
	AWpActorPool();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void Reserve(const TSubclassOf<AActor>& InClass, const uint32 Number);

	AActor* Acquire(const FVector& Location, const FRotator& Rotation, const void* const CustomData);
	void Release(AActor* const Actor);

private:
	TSubclassOf<AActor> Class = nullptr;

	TSet<AActor*> FreeActors = {};

public:
	static AWpActorPool* Instance(const TSubclassOf<AActor>& Class);

	static AActor* Acquire(const TSubclassOf<AActor>& Class, const FVector& Location, const FRotator& Rotation, const void* const CustomData);
	static void Release(const TSubclassOf<AActor>& Class, AActor* const Actor);

	template <class T>
	static T* Acquire(const FVector& Location, const FRotator& Rotation, const void* const CustomData)
	{
		return Cast<T>(Acquire(T::StaticClass(), Location, Rotation, CustomData));
	}

	template <class T>
	static void Release(AActor* const Actor)
	{
		Release(T::StaticClass(), Actor);
	}

private:
	static TMap<TSubclassOf<AActor>, AWpActorPool*> Instances;
};
