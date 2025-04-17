#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"

class AWpEntity;
enum class EWpEntityState : uint8;

class FWpWeapon : public IWpPooledObjectInterface
{
public:
	virtual ~FWpWeapon() override
	{
	}

	virtual void OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData) override;

	void Initialize(AWpEntity* const InOwnerEntity);

	void Tick(const float DeltaTime);

	bool MaybeFire(const AWpEntity* const Entity);
	bool MaybeFire(const FVector& Location);

	bool CanReach(const AWpEntity* const Entity) const;
	virtual bool CanReach(const FVector& Location) const;

	float GetRange() const;
	float GetMinRange() const;
	void ClearCurrentTargetEntity();
	AWpEntity* GetCurrentTargetEntity() const;
	void SetPriorityTargetEntity(AWpEntity* const InPriorityTargetEntity);

protected:
	virtual void Fire(const FVector& Location);
	FRotator GetDirection(const FVector& Location) const;

	float GetDamage() const;
	const FVector& GetFireLocation() const;

	AWpEntity* GetOwnerEntity();
	const AWpEntity* GetOwnerEntity() const;

private:
	void SetCurrentTargetEntity(AWpEntity* const NewTargetEntity);

	void UpdateCurrentTargetEntity();

	void OnCurrentTargetEntityStateChanged(AWpEntity* const Entity, const EWpEntityState EntityState);

	AWpEntity* OwnerEntity = nullptr;

	float Range = 0.0f;
	float MinRange = 0.0f;
	float Cooldown = 0.0f;
	float Damage = 0.0f;
	FVector FireLocation = FVector::ZeroVector;

	float LastFireTime = 0.0f;
	AWpEntity* CurrentTargetEntity = nullptr;
	AWpEntity* PriorityTargetEntity = nullptr;

	float LastUpdateCurrentTargetEntityTime = 0.0f;

public:
	static const float UpdateCurrentTargetEntityCooldown;
};
