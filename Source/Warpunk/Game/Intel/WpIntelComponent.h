#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"

#include "Components/StaticMeshComponent.h"

#include "WpIntelComponent.generated.h"

enum class EWpEntityState : uint8;
class IWpEntityInterface;

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class UWpIntelComponent final : public UStaticMeshComponent, public IWpPooledObjectInterface
{
	GENERATED_BODY()

public:
	UWpIntelComponent();

	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnRep_IsActive() override;

	virtual void OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const FWpPooledObjectData& PooledObjectData) override;

	const TSet<IWpEntityInterface*>& GetVisibleEntities() const;

private:
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* const OverlappedComponent, AActor* const OtherActor, UPrimitiveComponent* const OtherComp, const int32 OtherBodyIndex, const bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* const OverlappedComponent, AActor* const OtherActor, UPrimitiveComponent* const OtherComp, const int32 OtherBodyIndex);

	UFUNCTION()
	void OnRep_bMeshIsVisible();
	UFUNCTION()
	void OnRep_MeshTransform();

	void InitializeMesh();

	void OnEntityStateChanged(IWpEntityInterface* const Entity, const EWpEntityState EntityState);
	void OnMapLoaded();

	void SetMeshIsVisibile(const bool bInMeshIsVisible);
	void UpdateMeshVisibilityBasedOnTeam();

	TSet<IWpEntityInterface*> VisibleEntities = {};

	bool bCanCheckTeam = false;
	UPROPERTY(ReplicatedUsing=OnRep_bMeshIsVisible)
	bool bMeshIsVisible = false;
	UPROPERTY(ReplicatedUsing=OnRep_MeshTransform)
	FTransform MeshTransform = FTransform::Identity;

	FDelegateHandle OnPlayerStateReplicatedDelegateHandle = {};

	static const FName MeshCollisionProfileName;
};
