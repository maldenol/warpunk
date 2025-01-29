#include "WpGameInstance.h"

#include "Warpunk/Global/LoadingScreen/WpLoadingScreen.h"

const FName UWpGameInstance::LoadingScreenName = TEXT("LoadingScreen");

UWpLoadingScreen* UWpGameInstance::GetLoadingScreen()
{
	if (!LoadingScreen)
	{
		ensureAlways(LoadingScreenClass);
		LoadingScreen = NewObject<UWpLoadingScreen>(this, LoadingScreenClass, LoadingScreenName);
	}

	return LoadingScreen;
}

void UWpGameInstance::SetPlayerIds(const TArray<int32>& InPlayerIds)
{
	PlayerIds = InPlayerIds;
}

const TArray<int32>& UWpGameInstance::GetPlayerIds() const
{
	return PlayerIds;
}
