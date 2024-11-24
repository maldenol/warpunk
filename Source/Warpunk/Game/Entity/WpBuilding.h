#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Entity/WpEntityInterface.h"
#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"

#include "GameFramework/Actor.h"

#include "WpBuilding.generated.h"

class UWpBelligerentComponent;
class UWpEnduranceComponent;
class UWpIntelComponent;
class UWpOrderReceiverComponent;
class UWpProductionComponent;
class UWpWeaponComponent;

class UBoxComponent;

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class AWpBuilding final : public AActor, public IWpEntityInterface, public IWpPooledObjectInterface
{
	GENERATED_BODY()

public:
	AWpBuilding();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual UWpBelligerentComponent* GetBelligerentComponent() override;
	virtual const UWpBelligerentComponent* GetBelligerentComponent() const override;

	virtual UWpEnduranceComponent* GetEnduranceComponent() override;
	virtual const UWpEnduranceComponent* GetEnduranceComponent() const override;

	virtual UWpIntelComponent* GetIntelComponent() override;
	virtual const UWpIntelComponent* GetIntelComponent() const override;

	virtual UWpOrderReceiverComponent* GetOrderReceiverComponent() override;
	virtual const UWpOrderReceiverComponent* GetOrderReceiverComponent() const override;

	virtual UWpProductionComponent* GetProductionComponent() override;
	virtual const UWpProductionComponent* GetProductionComponent() const override;

	virtual UWpWeaponComponent* GetWeaponComponent() override;
	virtual const UWpWeaponComponent* GetWeaponComponent() const override;

	virtual uint32 GetConfigId() const override;

	virtual void OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData) override;
	virtual TArray<IWpPooledObjectInterface*> GetInnerPooledObjects() override;

private:
	UFUNCTION()
	void OnRep_MeshComponentTransform();
	UFUNCTION()
	void OnRep_MeshComponentSkeletalMesh();
	UFUNCTION()
	void OnRep_MeshComponentMaterial();
	UFUNCTION()
	void OnRep_MeshComponentAnimInstanceClass();
	UFUNCTION()
	void OnRep_BoxComponentExtent();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> BoxComponent = nullptr;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USkeletalMeshComponent> MeshComponent = nullptr;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UWpBelligerentComponent> BelligerentComponent = nullptr;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UWpEnduranceComponent> EnduranceComponent = nullptr;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UWpIntelComponent> IntelComponent = nullptr;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UWpOrderReceiverComponent> OrderReceiverComponent = nullptr;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UWpProductionComponent> ProductionComponent = nullptr;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UWpWeaponComponent> WeaponComponent = nullptr;

	UPROPERTY(VisibleInstanceOnly)
	uint32 ConfigId = 0;

	UPROPERTY(ReplicatedUsing=OnRep_MeshComponentTransform)
	FTransform MeshComponentTransform = FTransform::Identity;
	UPROPERTY(ReplicatedUsing=OnRep_MeshComponentSkeletalMesh)
	TObjectPtr<USkeletalMesh> MeshComponentSkeletalMesh = nullptr;
	UPROPERTY(ReplicatedUsing=OnRep_MeshComponentMaterial)
	TObjectPtr<UMaterialInterface> MeshComponentMaterial = nullptr;
	UPROPERTY(ReplicatedUsing=OnRep_MeshComponentAnimInstanceClass)
	TSubclassOf<UAnimInstance> MeshComponentAnimInstanceClass = nullptr;
	UPROPERTY(ReplicatedUsing=OnRep_BoxComponentExtent)
	FVector BoxComponentExtent = FVector::ZeroVector;

	static const FName BoxComponentName;
	static const FName MeshComponentName;
	static const FName BelligerentComponentName;
	static const FName EnduranceComponentName;
	static const FName IntelComponentName;
	static const FName OrderReceiverComponentName;
	static const FName ProductionComponentName;
	static const FName WeaponComponentName;

	static const FName BoxComponentCollisionProfileName;
	static const FName MeshComponentCollisionProfileName;
};
