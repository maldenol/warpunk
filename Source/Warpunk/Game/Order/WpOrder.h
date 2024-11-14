#pragma once

#include "CoreMinimal.h"

#include "WpOrder.generated.h"

class UWpEntityComponent;

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

struct FWpOrder final
{
	EWpOrderType Type = EWpOrderType::Undefined;
	FVector TargetLocation = FVector::ZeroVector;
	float Radius = 0.0f;
	UWpEntityComponent* TargetEntityComponent = nullptr;
	uint32 BuildingToProduceConfigId = 0;
	TMap<uint32, uint32> UnitToProduceConfigIdNumbers = {};
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
