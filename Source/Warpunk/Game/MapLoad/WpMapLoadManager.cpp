#include "WpMapLoadManager.h"

#include "Warpunk/Game/UI/WpGameMapLoadWidget.h"

#include "Kismet/GameplayStatics.h"

AWpMapLoadManager::AWpMapLoadManager()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = false;
	bNetLoadOnClient = true;
}

void AWpMapLoadManager::BeginPlay()
{
	Super::BeginPlay();

	ensureAlways(MapLoadWidgetClass);

	UWpGameMapLoadWidget* const MapLoadWidget = CreateWidget<UWpGameMapLoadWidget>(GetWorld(), MapLoadWidgetClass);
	MapLoadWidget->AddToViewport(100);
	MapLoadWidget->SetMapLoadManager(this);
}

void AWpMapLoadManager::NotifyActorLoaded()
{
	if (ExpectedActorNum && (LoadedActorNum == ExpectedActorNum))
	{
		return;
	}

	++LoadedActorNum;

	UpdateProgress();
	CheckMapLoaded();
}

void AWpMapLoadManager::SetExpectedActorNum(const uint32 InExpectedActorNum)
{
	ensureAlways(InExpectedActorNum);

	ExpectedActorNum = InExpectedActorNum;

	UpdateProgress();
	CheckMapLoaded();
}

bool AWpMapLoadManager::IsMapLoaded() const
{
	return bMapIsLoaded;
}

float AWpMapLoadManager::GetProgress() const
{
	return Progress;
}

AWpMapLoadManager::FMapLoadedEvent& AWpMapLoadManager::OnMapLoaded()
{
	return MapLoadedEvent;
}

AWpMapLoadManager::FProgressChangedEvent& AWpMapLoadManager::OnProgressChanged()
{
	return ProgressChangedEvent;
}

AWpMapLoadManager* AWpMapLoadManager::Instance(const UObject* const WorldContextObject)
{
	return Cast<AWpMapLoadManager>(UGameplayStatics::GetActorOfClass(WorldContextObject, StaticClass()));
}

void AWpMapLoadManager::CheckMapLoaded()
{
	if (bMapIsLoaded || !ExpectedActorNum)
	{
		return;
	}

	ensureAlways(LoadedActorNum <= ExpectedActorNum);

	if (LoadedActorNum == ExpectedActorNum)
	{
		bMapIsLoaded = true;
		MapLoadedEvent.Broadcast();
	}
}

void AWpMapLoadManager::UpdateProgress()
{
	Progress = ExpectedActorNum ? (static_cast<float>(LoadedActorNum) / static_cast<float>(ExpectedActorNum)) : 0.0f;
	ProgressChangedEvent.Broadcast(Progress);
}
