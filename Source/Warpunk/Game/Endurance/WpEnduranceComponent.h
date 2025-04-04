#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"

#include "Components/ActorComponent.h"

#include "WpEnduranceComponent.generated.h"

class AWpEntity;
class UWidgetComponent;

UENUM(NotBlueprintable, NotBlueprintType)
enum class EWpEntityState : uint8
{
	NotProduced,
	Operational,
	Wreck,
};

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class UWpEnduranceComponent final : public UActorComponent, public IWpPooledObjectInterface
{
	GENERATED_BODY()

public:
	DECLARE_EVENT_TwoParams(UWpEnduranceComponent, FEntityStateChangedEvent, AWpEntity*, EWpEntityState);

	DECLARE_EVENT_TwoParams(UWpEnduranceComponent, FCurrentEnduranceChangedEvent, float, float);

	UWpEnduranceComponent();

	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData) override;
	virtual void OnPooledObjectVisibilityChanged(const bool bIsVisible) override;

	bool CanBeRepaired(const bool bCanProduce) const;
	void ReceiveRepair(const float DeltaMatter);
	void ReceiveReclaim(const float DeltaMatter);
	void ReceiveDamage(const float DeltaEndurance);
	void Destruct();

	void SetMaxEndurance(const float InMaxEndurance);
	float GetMaxEndurance() const;
	float GetMatterCost() const;
	EWpEntityState GetEntityState() const;
	void SetCurrentEndurance(const float InCurrentEndurance);
	float GetCurrentEndurance() const;

	UWidgetComponent* GetEnduranceWidgetComponent();

	FEntityStateChangedEvent& OnEntityStateChanged();
	FCurrentEnduranceChangedEvent& OnCurrentEnduranceChanged();

private:
	UFUNCTION()
	void OnTakeAnyDamage(AActor* const DamagedActor, const float Damage, const UDamageType* const DamageType, AController* const InstigatedBy, AActor* const DamageCauser);

	UFUNCTION()
	void OnRep_EntityState();
	UFUNCTION()
	void OnRep_CurrentEndurance();

	void SetEntityState(const EWpEntityState InEntityState);

	UPROPERTY(VisibleInstanceOnly)
	float MaxEndurance = 0.0f;
	UPROPERTY(VisibleInstanceOnly)
	float MatterCost = 0.0f;

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing=OnRep_EntityState)
	EWpEntityState EntityState = EWpEntityState::NotProduced;
	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing=OnRep_CurrentEndurance)
	float CurrentEndurance = 0.0f;

	UPROPERTY(VisibleInstanceOnly)
	UWidgetComponent* EnduranceWidgetComponent = nullptr;

	FEntityStateChangedEvent EntityStateChangedEvent = {};
	FCurrentEnduranceChangedEvent CurrentEnduranceChangedEvent = {};

public:
	static const FName EnduranceWidgetComponentName;
};
