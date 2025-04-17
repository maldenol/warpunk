#pragma once

#include "CoreMinimal.h"

#include "WpOrder.generated.h"

class AWpEntity;

UENUM(NotBlueprintable, NotBlueprintType)
enum class EWpOrderType : uint8
{
	Undefined UMETA(Hidden),
	Stop UMETA(Hidden),
	Move,
	Patrol,
	Assist,
	Attack,
	Strike,
	ProduceBuilding,
	ProduceUnit,
	ProductionProxy UMETA(Hidden),
	InfiniteUnitProductionOff,
	InfiniteUnitProductionOn,
	Repair,
	Reclaim,
	SelfDestruct UMETA(Hidden),
};

USTRUCT(NotBlueprintable, NotBlueprintType)
struct FWpConfigIdNumber final
{
	GENERATED_BODY()

	UPROPERTY(VisibleInstanceOnly)
	uint32 ConfigId = 0;
	UPROPERTY(VisibleInstanceOnly)
	uint32 Number = 0;

	bool operator==(const FWpConfigIdNumber& Other) const;
	bool operator==(const uint32 OtherConfigId) const;

	static TArray<FWpConfigIdNumber> FromMap(const TMap<uint32, uint32>& Map);
	static TMap<uint32, uint32> ToMap(const TArray<FWpConfigIdNumber>& Array);
};

USTRUCT(NotBlueprintable, NotBlueprintType)
struct FWpOrder final
{
	GENERATED_BODY()

	UPROPERTY(VisibleInstanceOnly)
	EWpOrderType Type = EWpOrderType::Undefined;
	UPROPERTY(VisibleInstanceOnly)
	FVector TargetLocation = FVector::ZeroVector;
	UPROPERTY(VisibleInstanceOnly)
	float Radius = 0.0f;
	UPROPERTY(VisibleInstanceOnly)
	AWpEntity* TargetEntity = nullptr;
	UPROPERTY(VisibleInstanceOnly)
	uint32 BuildingToProduceConfigId = 0;
	UPROPERTY(VisibleInstanceOnly)
	TArray<FWpConfigIdNumber> UnitToProduceConfigIdNumbers = {};
	UPROPERTY(VisibleInstanceOnly)
	bool bOverride = false;
};

USTRUCT(NotBlueprintable, NotBlueprintType)
struct FWpDefaultOrders final
{
	GENERATED_BODY()

	EWpOrderType Get(const uint32 BelligerentId, const AActor* const Actor) const;

	bool operator==(const FWpDefaultOrders& Other) const;
	friend uint32 GetTypeHash(const FWpDefaultOrders& Other);

	UPROPERTY(EditDefaultsOnly)
	EWpOrderType Location = EWpOrderType::Undefined;
	UPROPERTY(EditDefaultsOnly)
	EWpOrderType Ally = EWpOrderType::Undefined;
	UPROPERTY(EditDefaultsOnly)
	EWpOrderType Enemy = EWpOrderType::Undefined;
};
