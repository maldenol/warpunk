#include "WpWeaponComponent.h"

#include "Warpunk/Game/Endurance/WpEnduranceComponent.h"
#include "Warpunk/Game/Entity/WpCommander.h"
#include "Warpunk/Game/Entity/WpEntity.h"
#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Entity/WpPooledEntityData.h"
#include "Warpunk/Game/Intel/WpFogOfWarManager.h"
#include "Warpunk/Game/Order/WpOrderReceiverComponent.h"
#include "Warpunk/Game/Weapon/WpPooledWeaponData.h"

const FName UWpWeaponComponent::MeshCollisionProfileName = TEXT("WpIntel");

UWpWeaponComponent::UWpWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.SetTickFunctionEnable(false);

	SetIsReplicatedByDefault(false);

	UWpWeaponComponent::SetCollisionProfileName(MeshCollisionProfileName);
	UWpWeaponComponent::SetNotifyRigidBodyCollision(false);
	SetGenerateOverlapEvents(false);

	SetVisibility(false);
}

void UWpWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner()->HasAuthority())
	{
		OnComponentBeginOverlap.AddDynamic(this, &UWpWeaponComponent::OnBeginOverlap);
		OnComponentEndOverlap.AddDynamic(this, &UWpWeaponComponent::OnEndOverlap);

		UWpEnduranceComponent* const EnduranceComponent = GetOwner<AWpEntity>()->GetEnduranceComponent();
		EnduranceComponent->OnEntityStateChanged().AddUObject(this, &UWpWeaponComponent::OnEntityStateChanged);
		OnEntityStateChanged(GetOwner<AWpEntity>(), EnduranceComponent->GetEntityState());

		UWpOrderReceiverComponent* const OrderReceiverComponent = GetOwner<AWpEntity>()->GetOrderReceiverComponent();
		OrderReceiverComponent->OnOrderReceived().AddUObject(this, &UWpWeaponComponent::OnOrderReceived);
	}
}

void UWpWeaponComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* const ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	if (GetOwner<AWpEntity>()->GetEnduranceComponent()->GetEntityState() != EWpEntityState::Operational)
	{
		return;
	}

	if (bManualControlOnly)
	{
		return;
	}

	for (uint32 Index = 0; Index < WP_ENTITY_MAX_WEAPON; ++Index)
	{
		if (FWpWeapon* const Weapon = Weapons[Index].Get())
		{
			Weapon->Tick(DeltaTime);
		}
		else
		{
			break;
		}
	}
}

void UWpWeaponComponent::OnRep_IsActive()
{
	// Override to not enable tick
}

void UWpWeaponComponent::OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
{
	ensureAlways(GetOwner()->HasAuthority());

	float LongestRange = 0.0f;

	if (bIsAcquired)
	{
		const FWpPooledEntityData& PooledEntityData = static_cast<const FWpPooledEntityData&>(PooledObjectData);
		const TArray<FWpWeaponConfig>& WeaponConfigs = PooledEntityData.EntityConfig->WeaponConfigs;

		ensureAlways(WeaponConfigs.Num() <= WP_ENTITY_MAX_WEAPON);

		for (uint32 Index = 0; Index < WP_ENTITY_MAX_WEAPON; ++Index)
		{
			if (WeaponConfigs.IsValidIndex(Index))
			{
				const FWpWeaponConfig& WeaponConfig = WeaponConfigs[Index];

				FWpWeapon* const Weapon = Weapons[Index].Set(WeaponConfig.Type);
				Weapon->Initialize(GetOwner<AWpEntity>());

				FWpPooledWeaponData PooledWeaponData;
				PooledWeaponData.WeaponConfig = &WeaponConfig;
				PooledWeaponData.ProjectileConfig = FWpProjectileConfig{PooledEntityData.EntityConfigId, Index};
				Weapon->AcquirePooledObject(PooledWeaponData);

				const float Range = Weapon->GetRange() * ((WeaponConfig.Type == EWpWeaponType::Projectile) ? 3.0f : 1.0f);
				if (Range > LongestRange)
				{
					LongestRange = Range;
				}
			}
			else
			{
				Weapons[Index].Reset();
			}
		}

		bManualControlOnly = GetOwner()->GetClass()->IsChildOf<AWpCommander>();

		bIsAcquired = !WeaponConfigs.IsEmpty();
	}

	if (!bIsAcquired)
	{
		for (uint32 Index = 0; Index < WP_ENTITY_MAX_WEAPON; ++Index)
		{
			if (FWpWeapon* const Weapon = Weapons[Index].Get())
			{
				Weapon->ReleasePooledObject();
				Weapons[Index].Reset();
			}
			else
			{
				break;
			}
		}
	}
	else
	{
		InitializeMesh();

		const float Range = LongestRange;

		static constexpr float HalfSizeZ = 2500.0f;
		const FVector MeshExtent = GetStaticMesh()->GetBounds().BoxExtent;
		const float ScaleX = Range / MeshExtent.X;
		const float ScaleY = Range / MeshExtent.Y;
		const float ScaleZ = HalfSizeZ / MeshExtent.Z;

		SetRelativeLocation(FVector{0.0f, 0.0f, -HalfSizeZ});
		SetRelativeScale3D(FVector{ScaleX, ScaleY, ScaleZ});
	}

	SetActive(bIsAcquired);
	SetComponentTickEnabled(bIsAcquired);
}

FWpWeapon* UWpWeaponComponent::GetWeapon(const uint32 Index)
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	return Weapons[Index].Get();
}

const FWpWeapon* UWpWeaponComponent::GetWeapon(const uint32 Index) const
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	return Weapons[Index].Get();
}

const TSet<AWpEntity*>& UWpWeaponComponent::GetEntitiesInReach() const
{
	ensureAlways(IsActive());
	ensureAlways(GetOwner()->HasAuthority());

	return EntitiesInReach;
}

void UWpWeaponComponent::OnBeginOverlap(UPrimitiveComponent* const OverlappedComponent, AActor* const OtherActor, UPrimitiveComponent* const OtherComp, const int32 OtherBodyIndex, const bool bFromSweep, const FHitResult& SweepResult)
{
	ensureAlways(GetOwner()->HasAuthority());

	if (OtherActor == GetOwner())
	{
		return;
	}

	if (AWpEntity* const OtherEntity = Cast<AWpEntity>(OtherActor))
	{
		ensureAlways(!EntitiesInReach.Contains(OtherEntity));

		EntitiesInReach.Add(OtherEntity);
	}
}

void UWpWeaponComponent::OnEndOverlap(UPrimitiveComponent* const OverlappedComponent, AActor* const OtherActor, UPrimitiveComponent* const OtherComp, const int32 OtherBodyIndex)
{
	ensureAlways(GetOwner()->HasAuthority());

	if (OtherActor == GetOwner())
	{
		return;
	}

	if (AWpEntity* const OtherEntity = Cast<AWpEntity>(OtherActor))
	{
		ensureAlways(EntitiesInReach.Contains(OtherEntity));

		EntitiesInReach.Remove(OtherEntity);
	}
}

void UWpWeaponComponent::InitializeMesh()
{
	ensureAlways(GetOwner()->HasAuthority());

	const AWpFogOfWarManager* const FogOfWarManager = AWpFogOfWarManager::Instance(this);
	UStaticMesh* const IntelMesh = FogOfWarManager->GetIntelMesh();

	ensureAlways(IntelMesh);

	SetStaticMesh(IntelMesh);
}

void UWpWeaponComponent::OnEntityStateChanged(AWpEntity* const Entity, const EWpEntityState EntityState)
{
	ensureAlways(GetOwner()->HasAuthority());

	const bool bIsOperational = EntityState == EWpEntityState::Operational;
	SetGenerateOverlapEvents(bIsOperational);
	if (bIsOperational)
	{
		const FVector SmallDelta = FVector::UpVector / 1000.0f;
		GetOwner()->SetActorLocation(GetOwner()->GetActorLocation() + SmallDelta);
		GetOwner()->SetActorLocation(GetOwner()->GetActorLocation() - SmallDelta);
	}
}

void UWpWeaponComponent::OnOrderReceived(const FWpOrder& InOrder)
{
	if (InOrder.Type == EWpOrderType::Stop)
	{
		for (uint32 Index = 0; Index < WP_ENTITY_MAX_WEAPON; ++Index)
		{
			if (FWpWeapon* const Weapon = Weapons[Index].Get())
			{
				Weapon->ClearCurrentTargetEntity();
				Weapon->SetPriorityTargetEntity(nullptr);
			}
			else
			{
				break;
			}
		}
	}
}
