#include "WpGameInstance.h"

void UWpGameInstance::SetPlayerIds(const TArray<int32>& InPlayerIds)
{
	PlayerIds = InPlayerIds;
}

const TArray<int32>& UWpGameInstance::GetPlayerIds() const
{
	return PlayerIds;
}
