#include "WpOrder.h"

#include "Warpunk/Game/Belligerent/WpBelligerentComponent.h"
#include "Warpunk/Game/Entity/WpEntityInterface.h"

bool FWpConfigIdNumber::operator==(const FWpConfigIdNumber& Other) const
{
	return ConfigId == Other.ConfigId;
}

bool FWpConfigIdNumber::operator==(const uint32 OtherConfigId) const
{
	return ConfigId == OtherConfigId;
}

TArray<FWpConfigIdNumber> FWpConfigIdNumber::FromMap(const TMap<uint32, uint32>& Map)
{
	TArray<FWpConfigIdNumber> Array;
	Array.Reserve(Map.Num());
	for (const TPair<uint32, uint32>& Pair : Map)
	{
		FWpConfigIdNumber ConfigIdNumber;
		ConfigIdNumber.ConfigId = Pair.Key;
		ConfigIdNumber.Number = Pair.Value;
		Array.Add(ConfigIdNumber);
	}
	return Array;
}

TMap<uint32, uint32> FWpConfigIdNumber::ToMap(const TArray<FWpConfigIdNumber>& Array)
{
	TMap<uint32, uint32> Map;
	Map.Reserve(Array.Num());
	for (const FWpConfigIdNumber& ConfigIdNumber : Array)
	{
		Map.Add(ConfigIdNumber.ConfigId, ConfigIdNumber.Number);
	}
	return Map;
}

IWpEntityInterface* FWpOrder::GetTargetEntity() const
{
	return Cast<IWpEntityInterface>(TargetEntityActor);
}

EWpOrderType FWpDefaultOrders::Get(const uint32 BelligerentId, const AActor* const Actor) const
{
	const IWpEntityInterface* const Entity = Cast<IWpEntityInterface>(Actor);
	if (!Entity)
	{
		return Location;
	}

	if (Entity->GetBelligerentComponent()->BelongsToSameTeam(BelligerentId))
	{
		return Ally;
	}

	return Enemy;
}

bool FWpDefaultOrders::operator==(const FWpDefaultOrders& Other) const
{
	return (Location == Other.Location) && (Ally == Other.Ally) && (Enemy == Other.Enemy);
}

uint32 GetTypeHash(const FWpDefaultOrders& Other)
{
	uint32 Hash = GetTypeHash(Other.Location);
	Hash = HashCombine(Hash, GetTypeHash(Other.Ally));
	Hash = HashCombine(Hash, GetTypeHash(Other.Enemy));
	return Hash;
}
