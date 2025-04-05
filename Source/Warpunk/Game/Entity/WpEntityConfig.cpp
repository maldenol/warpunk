#include "WpEntityConfig.h"

#include "Warpunk/Game/Weapon/WpWeaponComponent.h"

float FWpEntityConfig::DefaultGravityScale = 980.0f;

void FWpEntityConfig::OnDataTableChanged(const UDataTable* const InDataTable, const FName InRowName)
{
	Super::OnDataTableChanged(InDataTable, InRowName);

	FWpEntityConfig& EntityConfig = *InDataTable->FindRow<FWpEntityConfig>(InRowName, nullptr);
	EntityConfig.OnConfigChanged();
}

void FWpEntityConfig::OnConfigChanged()
{
	CheckCorrectness();

	if (bIsCorrect)
	{
		UpdateDerivedValues();
	}
}

void FWpEntityConfig::CheckCorrectness()
{
	bIsCorrect = true;

	bIsCorrect &= !Name.IsEmpty();
	bIsCorrect &= !Description.IsEmpty();

	bIsCorrect &= Type != EWpEntityType::Undefined;

	{
		bIsCorrect &= MeshConfig.Transform.GetScale3D().GetMin() > 0.0;
		if (MeshConfig.bUseSkeletalMesh)
		{
			bIsCorrect &= MeshConfig.SkeletalMesh != nullptr;
			bIsCorrect &= MeshConfig.AnimInstanceClass != nullptr;
		}
		else
		{
			bIsCorrect &= MeshConfig.StaticMesh != nullptr;
		}
		bIsCorrect &= MeshConfig.Material != nullptr;
	}

	if (Type == EWpEntityType::Unit)
	{
		bIsCorrect &= MovementConfig.Type != EWpEntityMovementType::Undefined;
		bIsCorrect &= MovementConfig.MaxMoveSpeed > 0.0f;
		bIsCorrect &= MovementConfig.MaxMoveAccelerationTime > 0.0f;
		bIsCorrect &= (MovementConfig.MaxMoveInclineAngleDeg > 0.0f) && (MovementConfig.MaxMoveInclineAngleDeg < 90.0f);
		bIsCorrect &= MovementConfig.MaxMoveBrakingTime > 0.0f;
		bIsCorrect &= MovementConfig.MaxMoveInertiaTime > 0.0f;
		bIsCorrect &= MovementConfig.MaxRotationSpeedDeg > 0.0f;
		if ((MovementConfig.Type == EWpEntityMovementType::Legs) || (MovementConfig.Type == EWpEntityMovementType::Tracks))
		{
			bIsCorrect &= MovementConfig.MaxRotationAccelerationTime > 0.0f;
			bIsCorrect &= MovementConfig.MaxRotationBrakingTime > 0.0f;
		}
		if (MovementConfig.Type == EWpEntityMovementType::Wheels)
		{
			bIsCorrect &= MovementConfig.MinTurningRadius > 0.0f;
			bIsCorrect &= MovementConfig.MaxWheelEdgeRotationTime > 0.0f;
			bIsCorrect &= MovementConfig.MaxWheelCenterRotationTime > 0.0f;
		}
		bIsCorrect &= MovementConfig.Mass > 0.0f;
		bIsCorrect &= (MovementConfig.MaxInclineAngleDeg > 0.0f) && (MovementConfig.MaxInclineAngleDeg < 90.0f);
		bIsCorrect &= MovementConfig.MaxStepUpHeight > 0.0f;
	}

	{
		bIsCorrect &= EnduranceConfig.MatterCost > 0.0f;
		bIsCorrect &= EnduranceConfig.MaxEndurance > 0.0f;
	}

	{
		bIsCorrect &= !OrderReceiverConfig->SupportedOrders.IsEmpty();
	}

	if (const FWpIntelConfig* const IntelConfigPtr = IntelConfig.GetPtrOrNull())
	{
		bIsCorrect &= IntelConfigPtr->Range > 0.0f;
	}

	if (const FWpProductionConfig* const ProductionConfigPtr = ProductionConfig.GetPtrOrNull())
	{
		bIsCorrect &= ProductionConfigPtr->Range > 0.0f;
		bIsCorrect &= ProductionConfigPtr->MatterConsumption > 0.0f;
		bIsCorrect &= ProductionConfigPtr->EnergyPerMatterRatio > 0.0f;
	}

	bIsCorrect &= WeaponConfigs.Num() <= WP_ENTITY_MAX_WEAPON;
	for (const FWpWeaponConfig& WeaponConfig : WeaponConfigs)
	{
		bIsCorrect &= WeaponConfig.Type != EWpWeaponType::Undefined;
		bIsCorrect &= WeaponConfig.Range > 0.0f;
		bIsCorrect &= WeaponConfig.MinRange >= 0.0f;
		bIsCorrect &= WeaponConfig.Damage > 0.0f;
		bIsCorrect &= WeaponConfig.FireRate > 0.0f;
		bIsCorrect &= !WeaponConfig.FireLocation.IsNearlyZero();
		if (WeaponConfig.Type == EWpWeaponType::Projectile)
		{
			bIsCorrect &= WeaponConfig.ProjectileRadius > 0.0f;
			bIsCorrect &= WeaponConfig.ProjectileMesh != nullptr;
			bIsCorrect &= WeaponConfig.ProjectileMeshToRadiusScale > 0.0f;
		}
	}
}

void FWpEntityConfig::UpdateDerivedValues()
{
	{
		const FBoxSphereBounds MeshBounds = MeshConfig.bUseSkeletalMesh
			                                    ? MeshConfig.SkeletalMesh->GetBounds()
			                                    : MeshConfig.StaticMesh->GetBounds();

		MeshConfig.RelativeTransform = MeshConfig.Transform;

		const FVector MeshOrigin = MeshBounds.Origin * MeshConfig.Transform.GetScale3D();
		MeshConfig.RelativeTransform.AddToTranslation(-MeshOrigin);

		const FVector MeshExtent = MeshBounds.BoxExtent * MeshConfig.Transform.GetScale3D();
		MeshConfig.CollisionRadius = FMath::Max(MeshExtent.X, MeshExtent.Y);
		MeshConfig.CollisionHalfHeight = FMath::Max(MeshExtent.Z, MeshConfig.CollisionRadius);
		MeshConfig.CollisionHeight = 2.0f * MeshConfig.CollisionHalfHeight;
	}

	if (Type == EWpEntityType::Unit)
	{
		float MoveAccelerationTorqueCoefficient = 1.0f;
		constexpr uint32 MaxIterationNum = 1000;
		for (uint32 Index = 0; Index < MaxIterationNum; ++Index)
		{
			const float CurrentMaxMoveInclineAngle = FMath::Asin(
				MovementConfig.MaxMoveSpeed * MoveAccelerationTorqueCoefficient
				/ DefaultGravityScale / MovementConfig.MaxMoveAccelerationTime
				/ (1.0f - FMath::Exp(-MoveAccelerationTorqueCoefficient)));
			const float DesiredMaxMoveInclineAngle = FMath::DegreesToRadians(MovementConfig.MaxMoveInclineAngleDeg);
			const float DeltaMaxMoveInclineAngle = DesiredMaxMoveInclineAngle - CurrentMaxMoveInclineAngle;
			if (!FMath::IsNearlyZero(DeltaMaxMoveInclineAngle, FMath::DegreesToRadians(0.5f)))
			{
				MoveAccelerationTorqueCoefficient += DeltaMaxMoveInclineAngle / 10.0f;

				if (Index == MaxIterationNum - 1)
				{
					bIsCorrect = false;
					return;
				}
			}
			else
			{
				break;
			}
		}

		MovementConfig.MoveAccelerationCoefficient1 = MovementConfig.MaxMoveSpeed * MoveAccelerationTorqueCoefficient
			/ MovementConfig.MaxMoveAccelerationTime
			/ (1.0f - FMath::Exp(-MoveAccelerationTorqueCoefficient));
		MovementConfig.MoveAccelerationCoefficient2 = -MoveAccelerationTorqueCoefficient / MovementConfig.MaxMoveAccelerationTime;

		MovementConfig.MaxMoveBrakingDeceleration = MovementConfig.MaxMoveSpeed / MovementConfig.MaxMoveBrakingTime;
		MovementConfig.MaxMoveInertiaDeceleration = MovementConfig.MaxMoveSpeed / MovementConfig.MaxMoveInertiaTime;
		MovementConfig.MaxGroundStaticFriction = DefaultGravityScale * FMath::Tan(FMath::DegreesToRadians(MovementConfig.MaxInclineAngleDeg));
		MovementConfig.MaxRotationSpeed = FMath::DegreesToRadians(MovementConfig.MaxRotationSpeedDeg);
		MovementConfig.MaxRotationAcceleration = MovementConfig.MaxRotationSpeed / MovementConfig.MaxRotationAccelerationTime;
		MovementConfig.MaxRotationBrakingDeceleration = MovementConfig.MaxRotationSpeed / MovementConfig.MaxRotationBrakingTime;
		MovementConfig.MaxWheelRotationAngle = FMath::Asin(1.0f / MovementConfig.MinTurningRadius);
		MovementConfig.MaxWheelEdgeRotationSpeed = MovementConfig.MaxWheelRotationAngle / MovementConfig.MaxWheelEdgeRotationTime;
		MovementConfig.MaxWheelCenterRotationSpeed = MovementConfig.MaxWheelRotationAngle / MovementConfig.MaxWheelCenterRotationTime;
		MovementConfig.MaxInclineAngleCos = FMath::Cos(FMath::DegreesToRadians(MovementConfig.MaxInclineAngleDeg));
	}

	for (FWpWeaponConfig& WeaponConfig : WeaponConfigs)
	{
		WeaponConfig.Cooldown = 1.0f / WeaponConfig.FireRate;

		if (WeaponConfig.Type == EWpWeaponType::Projectile)
		{
			WeaponConfig.ProjectileSpeed2 = WeaponConfig.Range * DefaultGravityScale;
			WeaponConfig.ProjectileSpeed4 = FMath::Square(WeaponConfig.ProjectileSpeed2);
			WeaponConfig.ProjectileSpeed = FMath::Sqrt(WeaponConfig.ProjectileSpeed2);
			WeaponConfig.ProjectileSpeedMinRange2 = WeaponConfig.MinRange * DefaultGravityScale;
			WeaponConfig.ProjectileSpeedMinRange4 = FMath::Square(WeaponConfig.ProjectileSpeedMinRange2);

			const FBoxSphereBounds& MeshBounds = WeaponConfig.ProjectileMesh->GetBounds();
			const float MeshExtentMax = MeshBounds.BoxExtent.GetMax();
			const float MeshScale = WeaponConfig.ProjectileMeshToRadiusScale * WeaponConfig.ProjectileRadius / MeshExtentMax;
			const FVector MeshOrigin = WeaponConfig.ProjectileMeshRotation.RotateVector(MeshBounds.Origin * MeshScale);
			WeaponConfig.ProjectileMeshRelativeTransform =
				FTransform{
					WeaponConfig.ProjectileMeshRotation,
					-MeshOrigin,
					FVector{MeshScale}
				};
		}
	}
}
