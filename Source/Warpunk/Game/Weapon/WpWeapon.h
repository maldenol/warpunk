#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"

class UWpEntityComponent;

class FWpWeapon : public IWpPooledObjectInterface
{
public:
	virtual ~FWpWeapon() override
	{
	}

	virtual void OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const void* const CustomData) override;

	void Initialize(UWpEntityComponent* const InOwnerEntityComponent);

	void Tick(const float DeltaTime);

	bool TryFire(const UWpEntityComponent* const TargetEntityComponent);
	bool TryFire(const FVector& TargetLocation);

	virtual bool CanReach(const FVector& TargetLocation) const;

	float GetRange() const;
	UWpEntityComponent* GetCurrentTarget() const;
	void SetPriorityTarget(UWpEntityComponent* const InPriorityTargetEntityComponent);

protected:
	virtual void Fire(const FVector& TargetLocation);
	FRotator GetDirection(const FVector& TargetLocation) const;

	float GetDamage() const;
	const FVector& GetFireLocation() const;

	UWpEntityComponent* GetOwnerEntityComponent();
	const UWpEntityComponent* GetOwnerEntityComponent() const;

private:
	void FindNewTarget();

	UWpEntityComponent* OwnerEntityComponent = nullptr;

	float Range = 0.0f;
	float Cooldown = 0.0f;
	float Damage = 0.0f;
	FVector FireLocation = FVector::ZeroVector;

	float LastFireTime = 0.0f;
	UWpEntityComponent* CurrentTargetEntityComponent = nullptr;
	UWpEntityComponent* PriorityTargetEntityComponent = nullptr;
};
