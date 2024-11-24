#pragma once

#include "CoreMinimal.h"

#include "UObject/Interface.h"

#include "WpEntityInterface.generated.h"

class UWpBelligerentComponent;
class UWpEnduranceComponent;
class UWpIntelComponent;
class UWpOrderReceiverComponent;
class UWpProductionComponent;
class UWpWeaponComponent;

UINTERFACE(MinimalAPI, NotBlueprintable, NotBlueprintType)
class UWpEntityInterface : public UInterface
{
	GENERATED_BODY()
};

class IWpEntityInterface
{
	GENERATED_BODY()

public:
	virtual UWpBelligerentComponent* GetBelligerentComponent() = 0;
	virtual const UWpBelligerentComponent* GetBelligerentComponent() const = 0;

	virtual UWpEnduranceComponent* GetEnduranceComponent() = 0;
	virtual const UWpEnduranceComponent* GetEnduranceComponent() const = 0;

	virtual UWpIntelComponent* GetIntelComponent() = 0;
	virtual const UWpIntelComponent* GetIntelComponent() const = 0;

	virtual UWpOrderReceiverComponent* GetOrderReceiverComponent() = 0;
	virtual const UWpOrderReceiverComponent* GetOrderReceiverComponent() const = 0;

	virtual UWpProductionComponent* GetProductionComponent() = 0;
	virtual const UWpProductionComponent* GetProductionComponent() const = 0;

	virtual UWpWeaponComponent* GetWeaponComponent() = 0;
	virtual const UWpWeaponComponent* GetWeaponComponent() const = 0;

	virtual uint32 GetConfigId() const = 0;
};
