#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"

#include "GameFramework/CharacterMovementComponent.h"

#include "WpUnitMovementComponent.generated.h"

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class UWpUnitMovementComponent final : public UCharacterMovementComponent, public IWpPooledObjectInterface
{
	GENERATED_BODY()

public:
	UWpUnitMovementComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData) override;

	void SetIsFirstPersonControlled(const bool bIsFirstPersonControlled);

private:
	void SetSpeed(const float InSpeed);

	UFUNCTION()
	void OnRep_Speed();

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing=OnRep_Speed)
	float Speed = 0.0f;
};
