#include "WpGameInstance.h"

#include "Warpunk/Game/Entity/WpEntityConfig.h"
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

FWpEntityConfig UWpGameInstance::GetEntityConfig(const uint32 EntityConfigId) const
{
	ensureAlways(EntityConfigs);

	const FName RowName = FName{FString::FromInt(EntityConfigId)};
	const FWpEntityConfig EntityConfig = *EntityConfigs->FindRow<FWpEntityConfig>(RowName, nullptr);
	return EntityConfig;
}

const TSubclassOf<UWpGameEntityEnduranceWidget>& UWpGameInstance::GetEnduranceWidgetClass() const
{
	return EnduranceWidgetClass;
}

const TSubclassOf<UUserWidget>& UWpGameInstance::GetSelectionWidgetClass() const
{
	return SelectionWidgetClass;
}

void UWpGameInstance::SetPlayerIds(const TArray<int32>& InPlayerIds)
{
	PlayerIds = InPlayerIds;
}

const TArray<int32>& UWpGameInstance::GetPlayerIds() const
{
	return PlayerIds;
}
