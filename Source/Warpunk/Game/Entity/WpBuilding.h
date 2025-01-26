#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Entity/WpEntityInterface.h"
#include "Warpunk/Game/Pool/WpPooledActorInterface.h"

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
class AWpBuilding final : public AActor, public IWpEntityInterface, public IWpPooledActorInterface
{
	GENERATED_BODY()

public:
	AWpBuilding();

	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool GetNetDormancy(const FVector& ViewPos, const FVector& ViewDir, AActor* const Viewer, AActor* const ViewTarget, UActorChannel* const InChannel, const float Time, const bool bLowBandwidth) override;

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

	virtual void OnPooledActorVisibilityChanged(const bool bIsVisible) override;

private:
	UFUNCTION()
	void OnRep_ConfigId();

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

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing=OnRep_ConfigId)
	uint32 ConfigId = 0;

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
