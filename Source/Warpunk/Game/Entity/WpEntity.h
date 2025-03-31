#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Entity/WpEntityType.h"
#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"

#include "GameFramework/Pawn.h"

#include "WpEntity.generated.h"

class UWpBelligerentComponent;
class UWpEnduranceComponent;
class UWpEntityMovementComponent;
class UWpIntelComponent;
class UWpOrderReceiverComponent;
class UWpProductionComponent;
class UWpWeaponComponent;

class UCapsuleComponent;
class UNavModifierComponent;

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class AWpEntity : public APawn, public IWpPooledObjectInterface
{
	GENERATED_BODY()

public:
	AWpEntity();

	virtual void BeginPlay() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual bool GetNetDormancy(const FVector& ViewPos, const FVector& ViewDir, AActor* const Viewer, AActor* const ViewTarget, UActorChannel* const InChannel, const float Time, const bool bLowBandwidth) override;

	virtual void PostNetReceiveLocationAndRotation() override;

	virtual void OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData) override;
	virtual void OnPooledObjectVisibilityChanged(const bool bIsVisible) override;
	virtual TArray<IWpPooledObjectInterface*> GetInnerPooledObjects() override;

	UCapsuleComponent* GetCollisionComponent();
	const UCapsuleComponent* GetCollisionComponent() const;

	UStaticMeshComponent* GetMeshComponent();
	const UStaticMeshComponent* GetMeshComponent() const;

	UWpEntityMovementComponent* GetEntityMovementComponent();
	const UWpEntityMovementComponent* GetEntityMovementComponent() const;

	UWpBelligerentComponent* GetBelligerentComponent();
	const UWpBelligerentComponent* GetBelligerentComponent() const;

	UWpEnduranceComponent* GetEnduranceComponent();
	const UWpEnduranceComponent* GetEnduranceComponent() const;

	UWpIntelComponent* GetIntelComponent();
	const UWpIntelComponent* GetIntelComponent() const;

	UWpOrderReceiverComponent* GetOrderReceiverComponent();
	const UWpOrderReceiverComponent* GetOrderReceiverComponent() const;

	UWpProductionComponent* GetProductionComponent();
	const UWpProductionComponent* GetProductionComponent() const;

	UWpWeaponComponent* GetWeaponComponent();
	const UWpWeaponComponent* GetWeaponComponent() const;

	uint32 GetEntityConfigId() const;
	EWpEntityType GetEntityType() const;

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCapsuleComponent> CollisionComponent = nullptr;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> MeshComponent = nullptr;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UNavModifierComponent> NavModifierComponent = nullptr;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UWpEntityMovementComponent> MovementComponent = nullptr;
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

private:
	UFUNCTION()
	void OnRep_EntityConfigId();

	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing=OnRep_EntityConfigId)
	uint32 EntityConfigId = 0;

	UPROPERTY(VisibleInstanceOnly)
	EWpEntityType EntityType = EWpEntityType::Undefined;

public:
	static const FName CollisionComponentName;
	static const FName MeshComponentName;
	static const FName NavModifierComponentName;
	static const FName MovementComponentName;
	static const FName BelligerentComponentName;
	static const FName EnduranceComponentName;
	static const FName IntelComponentName;
	static const FName OrderReceiverComponentName;
	static const FName ProductionComponentName;
	static const FName WeaponComponentName;

	static const FName CollisionComponentCollisionProfileName;
	static const FName MeshComponentCollisionProfileName;
};
