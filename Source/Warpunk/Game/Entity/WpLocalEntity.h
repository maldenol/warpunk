#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Entity/WpEntity.h"

#include "WpLocalEntity.generated.h"

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class AWpLocalEntity final : public AWpEntity
{
	GENERATED_BODY()

public:
	AWpLocalEntity();

	virtual void BeginPlay() override;
	virtual void Tick(const float DeltaSeconds) override;

	virtual void OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData) override;
	virtual TArray<IWpPooledObjectInterface*> GetInnerPooledObjects() override;

	void RememberIntel();

private:
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* const OverlappedComponent, AActor* const OtherActor, UPrimitiveComponent* const OtherComp, const int32 OtherBodyIndex, const bool bFromSweep, const FHitResult& SweepResult);

	float TimeUntilReleased = 0.0f;

public:
	static const float RememberIntelReleaseDelay;
};
