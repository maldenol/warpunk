#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"

class IWpEntityInterface;

class FWpWeapon : public IWpPooledObjectInterface
{
public:
	virtual ~FWpWeapon() override
	{
	}

	virtual void OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData) override;

	void Initialize(IWpEntityInterface* const InOwnerEntity);

	void Tick(const float DeltaTime);

	bool TryFire(const IWpEntityInterface* const Entity);
	bool TryFire(const FVector& Location);

	bool CanReach(const IWpEntityInterface* const Entity) const;
	virtual bool CanReach(const FVector& Location) const;

	float GetRange() const;
	float GetMinRange() const;
	IWpEntityInterface* GetCurrentTargetEntity() const;
	void SetPriorityTargetEntity(IWpEntityInterface* const InPriorityTargetEntity);

protected:
	virtual void Fire(const FVector& Location);
	FRotator GetDirection(const FVector& Location) const;

	float GetDamage() const;
	const FVector& GetFireLocation() const;

	IWpEntityInterface* GetOwnerEntity();
	const IWpEntityInterface* GetOwnerEntity() const;

private:
	void UpdateCurrentTargetEntity();

	IWpEntityInterface* OwnerEntity = nullptr;

	float Range = 0.0f;
	float MinRange = 0.0f;
	float Cooldown = 0.0f;
	float Damage = 0.0f;
	FVector FireLocation = FVector::ZeroVector;

	float LastFireTime = 0.0f;
	IWpEntityInterface* CurrentTargetEntity = nullptr;
	IWpEntityInterface* PriorityTargetEntity = nullptr;
};
