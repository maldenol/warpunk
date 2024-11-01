#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Belligerent/WpBelligerentMember.h"
#include "Warpunk/Game/Endurance/WpEndurance.h"
#include "Warpunk/Game/Intel/WpIntel.h"
#include "Warpunk/Game/Order/WpOrderReceiver.h"
#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"
#include "Warpunk/Game/Production/WpProduction.h"
#include "Warpunk/Game/Weapon/WpWeaponArray.h"

#include "Components/ActorComponent.h"

#include "WpEntityComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnDamageReceived, float);

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class UWpEntityComponent final : public UActorComponent, public IWpPooledObjectInterface
{
	GENERATED_BODY()

public:
	UWpEntityComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* const ThisTickFunction) override;

	virtual void OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const void* const CustomData) override;
	virtual TArray<IWpPooledObjectInterface*> GetInnerPooledObjects() override;

	operator FWpBelligerentMember&();
	operator const FWpBelligerentMember&() const;
	operator FWpEndurance&();
	operator const FWpEndurance&() const;
	operator FWpOrderReceiver&();
	operator const FWpOrderReceiver&() const;
	operator FWpIntel&();
	operator const FWpIntel&() const;
	operator FWpProduction&();
	operator const FWpProduction&() const;
	operator FWpWeaponArray&();
	operator const FWpWeaponArray&() const;

	uint32 GetEntityConfigID() const;

	UFUNCTION()
	void OnIntelVolumeBeginOverlap(UPrimitiveComponent* const OverlappedComponent, AActor* const OtherActor, UPrimitiveComponent* const OtherComp, const int32 OtherBodyIndex, const bool bFromSweep, const FHitResult& SweepResult);
	UFUNCTION()
	void OnIntelVolumeEndOverlap(UPrimitiveComponent* const OverlappedComponent, AActor* const OtherActor, UPrimitiveComponent* const OtherComp, const int32 OtherBodyIndex);

	FOnDamageReceived OnDamageReceived;

private:
	UFUNCTION()
	void OnTakeAnyDamage(AActor* const DamagedActor, const float Damage, const UDamageType* const DamageType, AController* const InstigatedBy, AActor* const DamageCauser);

	FWpBelligerentMember BelligerentMember = {};
	FWpEndurance Endurance = {};
	FWpOrderReceiver OrderReceiver = {};
	FWpIntel Intel = {};
	FWpProduction Production = {};
	FWpWeaponArray WeaponArray = {};

	uint32 EntityConfigID = 0;
};

#define WP_AS_BELLIGERENT_MEMBER(ec) static_cast<FWpBelligerentMember&>(*(ec))
#define WP_AS_BELLIGERENT_MEMBER_CONST(ec) static_cast<const FWpBelligerentMember&>(*(ec))
#define WP_AS_ENDURANCE(ec) static_cast<FWpEndurance&>(*(ec))
#define WP_AS_ENDURANCE_CONST(ec) static_cast<const FWpEndurance&>(*(ec))
#define WP_AS_ORDER_RECEIVER(ec) static_cast<FWpOrderReceiver&>(*(ec))
#define WP_AS_ORDER_RECEIVER_CONST(ec) static_cast<const FWpOrderReceiver&>(*(ec))
#define WP_AS_INTEL(ec) static_cast<FWpIntel&>(*(ec))
#define WP_AS_INTEL_CONST(ec) static_cast<const FWpIntel&>(*(ec))
#define WP_AS_PRODUCTION(ec) static_cast<FWpProduction&>(*(ec))
#define WP_AS_PRODUCTION_CONST(ec) static_cast<const FWpProduction&>(*(ec))
#define WP_AS_WEAPON_ARRAY(ec) static_cast<FWpWeaponArray&>(*(ec))
#define WP_AS_WEAPON_ARRAY_CONST(ec) static_cast<const FWpWeaponArray&>(*(ec))
