#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"

class UWpEntityComponent;

class UWidgetComponent;

enum class EWpEntityState : uint8
{
	NotProduced,
	Operational,
	Wreck,
};

class FWpEndurance final : public IWpPooledObjectInterface
{
public:
	DECLARE_EVENT_TwoParams(FWpEndurance, FCurrentEnduranceChangedEvent, float, float);

	virtual void OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const void* const CustomData) override;

	void Initialize(UWpEntityComponent* const InOwnerEntityComponent);

	bool CanBeRepaired(const bool bCanProduce) const;
	void ReceiveRepair(const float DeltaMatter);
	void ReceiveReclaim(const float DeltaMatter);
	void ReceiveDamage(const float DeltaEndurance);
	void Destruct();

	float GetMaxEndurance() const;
	float GetMatterCost() const;
	EWpEntityState GetEntityState() const;
	void SetCurrentEndurance(const float InCurrentEndurance);
	float GetCurrentEndurance() const;

	static const FName EnduranceWidgetComponentName;

	FCurrentEnduranceChangedEvent& OnCurrentEnduranceChanged();

private:
	UWpEntityComponent* OwnerEntityComponent = nullptr;

	float MaxEndurance = 0.0f;
	float MatterCost = 0.0f;

	EWpEntityState EntityState = EWpEntityState::NotProduced;
	float CurrentEndurance = 0.0f;

	UWidgetComponent* EnduranceWidgetComponent = nullptr;

	FCurrentEnduranceChangedEvent CurrentEnduranceChangedEvent = {};
};
