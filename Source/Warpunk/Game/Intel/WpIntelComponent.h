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

	virtual void OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const FWpPooledObjectData& PooledObjectData) override;

	void AddIntel(IWpEntityInterface* const Entity);
	void RemoveIntel(IWpEntityInterface* const Entity);

	const TArray<IWpEntityInterface*>& GetIntel() const;

private:
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* const OverlappedComponent, AActor* const OtherActor, UPrimitiveComponent* const OtherComp, const int32 OtherBodyIndex, const bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* const OverlappedComponent, AActor* const OtherActor, UPrimitiveComponent* const OtherComp, const int32 OtherBodyIndex);

	UFUNCTION()
	void OnRep_bIsMeshVisible();
	UFUNCTION()
	void OnRep_MeshTransform();

	void InitializeMesh();

	void OnEntityStateChanged(const EWpEntityState EntityState);

	void SetMeshVisibility(const bool bInIsMeshVisible);
	void UpdateMeshVisibilityBasedOnTeam();

	void OnPlayerStateReplicated();

	TArray<IWpEntityInterface*> Intel = {};

	UPROPERTY(ReplicatedUsing=OnRep_bIsMeshVisible)
	bool bIsMeshVisible = false;
	UPROPERTY(ReplicatedUsing=OnRep_MeshTransform)
	FTransform MeshTransform = FTransform::Identity;

	FDelegateHandle OnPlayerStateReplicatedDelegateHandle = {};

	static const FName MeshCollisionProfileName;
};
