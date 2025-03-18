#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Entity/WpEntityType.h"
#include "Warpunk/Game/Movement/WpEntityMovementType.h"
#include "Warpunk/Game/Order/WpOrder.h"
#include "Warpunk/Game/Weapon/WpWeaponWrapper.h"

#include "WpEntityConfig.generated.h"

USTRUCT(NotBlueprintable, NotBlueprintType)
struct FWpMeshConfig final
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FTransform Transform = FTransform::Identity;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UStaticMesh> Mesh = nullptr;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UMaterialInterface> Material = nullptr;
};

USTRUCT(NotBlueprintable, NotBlueprintType)
struct FWpMovementConfig final
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	EWpEntityMovementType Type = EWpEntityMovementType::Undefined;

	UPROPERTY(EditAnywhere)
	float MaxMoveSpeed = 0.0f;
	UPROPERTY(EditAnywhere)
	float MaxMoveAccelerationTime = 0.0f;
	UPROPERTY(EditAnywhere)
	float MaxMoveInclineAngle = 0.0f;
	UPROPERTY(EditAnywhere)
	float MaxMoveBrakingTime = 0.0f;
	UPROPERTY(EditAnywhere)
	float MaxMoveInertiaTime = 0.0f;

	UPROPERTY(EditAnywhere)
	float MaxRotationSpeed = 0.0f;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "Type == EWpEntityMovementType::Legs || Type == EWpEntityMovementType::Tracks", EditConditionHides))
	float MaxRotationAccelerationTime = 0.0f;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "Type == EWpEntityMovementType::Legs || Type == EWpEntityMovementType::Tracks", EditConditionHides))
	float MaxRotationBrakingTime = 0.0f;

	UPROPERTY(EditAnywhere, meta = (EditCondition = "Type == EWpEntityMovementType::Wheels", EditConditionHides))
	float MinTurningRadius = 0.0f;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "Type == EWpEntityMovementType::Wheels", EditConditionHides))
	float MaxWheelEdgeRotationTime = 0.0f;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "Type == EWpEntityMovementType::Wheels", EditConditionHides))
	float MaxWheelCenterRotationTime = 0.0f;

	UPROPERTY(EditAnywhere)
	float MaxInclineAngle = 0.0f;

	UPROPERTY(EditAnywhere)
	float MaxStepUpHeight = 0.0f;
};

USTRUCT(NotBlueprintable, NotBlueprintType)
struct FWpEnduranceConfig final
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float MatterCost = 0.0f;
	UPROPERTY(EditAnywhere)
	float MaxEndurance = 0.0f;
};

USTRUCT(NotBlueprintable, NotBlueprintType)
struct FWpOrderReceiverConfig final
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FWpDefaultOrders DefaultOrders = {};
	UPROPERTY(EditAnywhere)
	TSet<EWpOrderType> SupportedOrders = {};
	UPROPERTY(EditAnywhere)
	bool bShouldProxyOrders = false;
};

USTRUCT(NotBlueprintable, NotBlueprintType)
struct FWpIntelConfig final
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float Range = 0.0f;
};

USTRUCT(NotBlueprintable, NotBlueprintType)
struct FWpProductionConfig final
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float Range = 0.0f;
	UPROPERTY(EditAnywhere)
	float MatterConsumption = 0.0f;
	UPROPERTY(EditAnywhere)
	float EnergyPerMatterRatio = 0.0f;
	UPROPERTY(EditAnywhere)
	bool bCanProduce = false;
	UPROPERTY(EditAnywhere)
	bool bIsSupport = false;
};

USTRUCT(NotBlueprintable, NotBlueprintType)
struct FWpWeaponConfig final
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float Range = 0.0f;
	UPROPERTY(EditAnywhere)
	float MinRange = 0.0f;
	UPROPERTY(EditAnywhere)
	float FireRate = 0.0f;
	UPROPERTY(EditAnywhere)
	EWpWeaponType Type = EWpWeaponType::Undefined;
	UPROPERTY(EditAnywhere)
	float Damage = 0.0f;
	UPROPERTY(EditAnywhere)
	FVector FireLocation = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "Type == EWpWeaponType::Projectile", EditConditionHides))
	bool bHighAngle = false;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "Type == EWpWeaponType::Projectile", EditConditionHides))
	float ProjectileRadius = 0.0f;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "Type == EWpWeaponType::Projectile", EditConditionHides))
	TObjectPtr<UStaticMesh> ProjectileMesh = nullptr;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "Type == EWpWeaponType::Projectile", EditConditionHides))
	FRotator ProjectileMeshRotation = FRotator::ZeroRotator;
	UPROPERTY(EditAnywhere, meta = (EditCondition = "Type == EWpWeaponType::Projectile", EditConditionHides))
	float ProjectileMeshToRadiusScale = 0.0f;
};

USTRUCT(NotBlueprintable, NotBlueprintType)
struct FWpEntityConfig final : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FString Name = TEXT("");
	UPROPERTY(EditAnywhere)
	FString Description = TEXT("");

	UPROPERTY(EditAnywhere)
	EWpEntityType Type = EWpEntityType::Undefined;
	UPROPERTY(EditAnywhere)
	FWpMeshConfig MeshConfig = {};
	UPROPERTY(EditAnywhere, meta = (EditCondition = "Type == EWpEntityType::Unit", EditConditionHides))
	FWpMovementConfig MovementConfig = {};
	UPROPERTY(EditAnywhere)
	FWpEnduranceConfig EnduranceConfig = {};
	UPROPERTY(EditAnywhere)
	TOptional<FWpOrderReceiverConfig> OrderReceiverConfig = {};
	UPROPERTY(EditAnywhere)
	TOptional<FWpIntelConfig> IntelConfig = {};
	UPROPERTY(EditAnywhere)
	TOptional<FWpProductionConfig> ProductionConfig = {};
	UPROPERTY(EditAnywhere)
	TArray<FWpWeaponConfig> WeaponConfigs = {};
};
