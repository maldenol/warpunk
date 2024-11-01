#include "WpOrder.h"

#include "Warpunk/Game/EntityComponent/WpEntityComponent.h"
#include "Warpunk/Game/EntityComponent/WpEntityInterface.h"

EWpOrderType FWpDefaultOrders::Get(const uint32 BelligerentID, const AActor* const Actor) const
{
	const IWpEntityInterface* const Entity = Cast<IWpEntityInterface>(Actor);
	if (!Entity)
	{
		return Location;
	}

	if (WP_AS_BELLIGERENT_MEMBER_CONST(Entity->GetEntityComponent()).BelongsToSameTeam(BelligerentID))
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
