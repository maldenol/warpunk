#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"

#include "Components/StaticMeshComponent.h"

#include "WpIntelComponent.generated.h"

class AWpEntity;
enum class EWpEntityState : uint8;

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class UWpIntelComponent final : public UStaticMeshComponent, public IWpPooledObjectInterface
{
	GENERATED_BODY()

public:
	UWpIntelComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const FWpPooledObjectData& PooledObjectData) override;
	virtual void OnPooledObjectVisibilityChanged(const bool bIsVisible) override;

	const TSet<AWpEntity*>& GetVisibleEntities() const;

private:
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* const OverlappedComponent, AActor* const OtherActor, UPrimitiveComponent* const OtherComp, const int32 OtherBodyIndex, const bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* const OverlappedComponent, AActor* const OtherActor, UPrimitiveComponent* const OtherComp, const int32 OtherBodyIndex);

	UFUNCTION()
	void OnRep_bMeshIsVisible();

	void InitializeMesh();

	void OnEntityStateChanged(AWpEntity* const Entity, const EWpEntityState EntityState);
	void OnLoaded();

	void SetMeshIsVisibile(const bool bInMeshIsVisible);
	void UpdateMeshVisibilityBasedOnTeam();

	TSet<AWpEntity*> VisibleEntities = {};

	bool bCanCheckTeam = false;
	UPROPERTY(ReplicatedUsing=OnRep_bMeshIsVisible)
	bool bMeshIsVisible = false;

	FDelegateHandle OnPlayerStateReplicatedDelegateHandle = {};

public:
	static const FName MeshCollisionProfileName;
};
