#include "WpWeaponComponent.h"

#include "Warpunk/Game/Endurance/WpEnduranceComponent.h"
#include "Warpunk/Game/Entity/WpCommander.h"
#include "Warpunk/Game/Entity/WpEntityConfig.h"
#include "Warpunk/Game/Entity/WpEntityInterface.h"
#include "Warpunk/Game/Entity/WpPooledEntityData.h"
#include "Warpunk/Game/Intel/WpFogOfWarManager.h"
#include "Warpunk/Game/Weapon/WpPooledWeaponData.h"

#include "Kismet/GameplayStatics.h"

const FName UWpWeaponComponent::MeshCollisionProfileName = TEXT("WpIntel");

UWpWeaponComponent::UWpWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

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

		UWpEnduranceComponent* const EnduranceComponent = GetOwner<IWpEntityInterface>()->GetEnduranceComponent();
		EnduranceComponent->OnEntityStateChanged().AddUObject(this, &UWpWeaponComponent::OnEntityStateChanged);
		OnEntityStateChanged(EnduranceComponent->GetEntityState());
	}
	else
	{
		SetComponentTickEnabled(false);
	}
}

void UWpWeaponComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* const ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GetOwner<IWpEntityInterface>()->GetEnduranceComponent()->GetEntityState() != EWpEntityState::Operational)
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

void UWpWeaponComponent::OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const FWpPooledObjectData& PooledObjectData)
{
	float LongestRange = 0.0f;

	if (bIsAcquired)
	{
		const FWpPooledEntityData& PooledEntityData = static_cast<const FWpPooledEntityData&>(PooledObjectData);
		const TArray<FWpWeaponConfig>& WeaponConfigs = PooledEntityData.EntityConfig->WeaponConfigs;

		for (uint32 Index = 0; Index < WP_ENTITY_MAX_WEAPON; ++Index)
		{
			if (WeaponConfigs.IsValidIndex(Index))
			{
				const FWpWeaponConfig& WeaponConfig = WeaponConfigs[Index];

				FWpWeapon* const Weapon = Weapons[Index].Set(WeaponConfig.Type);
				Weapon->Initialize(GetOwner<IWpEntityInterface>());

				FWpPooledWeaponData PooledWeaponData;
				PooledWeaponData.WeaponConfig = &WeaponConfig;
				Weapon->AcquirePooledObject(PooledWeaponData);

				const float Range = Weapon->GetRange();
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

		bManualControlOnly = GetOwner()->GetClass()->IsChildOf(AWpCommander::StaticClass());

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
	return Weapons[Index].Get();
}

const FWpWeapon* UWpWeaponComponent::GetWeapon(const uint32 Index) const
{
	return Weapons[Index].Get();
}

const TArray<IWpEntityInterface*>& UWpWeaponComponent::GetEntitiesInReach() const
{
	return EntitiesInReach;
}

void UWpWeaponComponent::OnBeginOverlap(UPrimitiveComponent* const OverlappedComponent, AActor* const OtherActor, UPrimitiveComponent* const OtherComp, const int32 OtherBodyIndex, const bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor == GetOwner())
	{
		return;
	}

	if (IWpEntityInterface* const OtherEntity = Cast<IWpEntityInterface>(OtherActor))
	{
		EntitiesInReach.Add(OtherEntity);
	}
}

void UWpWeaponComponent::OnEndOverlap(UPrimitiveComponent* const OverlappedComponent, AActor* const OtherActor, UPrimitiveComponent* const OtherComp, const int32 OtherBodyIndex)
{
	if (OtherActor == GetOwner())
	{
		return;
	}

	if (IWpEntityInterface* const OtherEntity = Cast<IWpEntityInterface>(OtherActor))
	{
		EntitiesInReach.RemoveSingle(OtherEntity);
	}
}

void UWpWeaponComponent::InitializeMesh()
{
	const AWpFogOfWarManager* const FogOfWarManager = Cast<AWpFogOfWarManager>(UGameplayStatics::GetActorOfClass(GetWorld(), AWpFogOfWarManager::StaticClass()));
	UStaticMesh* const IntelMesh = FogOfWarManager->GetIntelMesh();

	ensureAlways(IntelMesh);

	SetStaticMesh(IntelMesh);
}

void UWpWeaponComponent::OnEntityStateChanged(const EWpEntityState EntityState)
{
	const bool bIsOperational = (EntityState == EWpEntityState::Operational);
	SetGenerateOverlapEvents(bIsOperational);
	if (bIsOperational)
	{
		const FVector SmallDelta = FVector::UpVector / 1000.0f;
		GetOwner()->SetActorLocation(GetOwner()->GetActorLocation() + SmallDelta);
		GetOwner()->SetActorLocation(GetOwner()->GetActorLocation() - SmallDelta);
	}
}
