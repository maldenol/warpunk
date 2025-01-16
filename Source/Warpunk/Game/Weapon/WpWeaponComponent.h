#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"
#include "Warpunk/Game/Weapon/WpWeapon.h"
#include "Warpunk/Game/Weapon/WpWeaponWrapper.h"

#include "Components/StaticMeshComponent.h"

#include "WpWeaponComponent.generated.h"

#define WP_ENTITY_MAX_WEAPON 4

enum class EWpEntityState : uint8;
struct FWpWeaponConfig;
class IWpEntityInterface;

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class UWpWeaponComponent final : public UStaticMeshComponent, public IWpPooledObjectInterface
{
	GENERATED_BODY()

public:
	UWpWeaponComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* const ThisTickFunction) override;

	virtual void OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const FWpPooledObjectData& PooledObjectData) override;

	FWpWeapon* GetWeapon(const uint32 Index);
	const FWpWeapon* GetWeapon(const uint32 Index) const;

	const TArray<IWpEntityInterface*>& GetEntitiesInReach() const;

private:
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* const OverlappedComponent, AActor* const OtherActor, UPrimitiveComponent* const OtherComp, const int32 OtherBodyIndex, const bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* const OverlappedComponent, AActor* const OtherActor, UPrimitiveComponent* const OtherComp, const int32 OtherBodyIndex);

	void InitializeMesh();

	void OnEntityStateChanged(const EWpEntityState EntityState);

	TStaticArray<FWpWeaponWrapper, WP_ENTITY_MAX_WEAPON> Weapons = {};

	bool bManualControlOnly = false;

	TArray<IWpEntityInterface*> EntitiesInReach = {};

	static const FName MeshCollisionProfileName;
};
