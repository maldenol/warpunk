#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Info.h"

#include "WpMapLoadManager.generated.h"

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class AWpMapLoadManager final : public AInfo
{
	GENERATED_BODY()

public:
	DECLARE_EVENT(AWpMapLoadManager, FMapLoadedEvent);

	DECLARE_EVENT_OneParam(AWpMapLoadManager, FProgressChangedEvent, float);

	AWpMapLoadManager();

	virtual void BeginPlay() override;

	void NotifyActorLoaded();
	void SetExpectedActorNum(const uint32 InExpectedActorNum);

	bool IsMapLoaded() const;
	float GetProgress() const;

	FMapLoadedEvent& OnMapLoaded();
	FProgressChangedEvent& OnProgressChanged();

	static AWpMapLoadManager* Instance(const UObject* const WorldContextObject);

private:
	void CheckMapLoaded();
	void UpdateProgress();

	uint32 ExpectedActorNum = 0;
	uint32 LoadedActorNum = 0;

	bool bMapIsLoaded = false;
	float Progress = 0.0f;

	FMapLoadedEvent MapLoadedEvent = {};
	FProgressChangedEvent ProgressChangedEvent = {};

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> MapLoadWidgetClass = nullptr;
};
