#pragma once

#include "CoreMinimal.h"

#include "GameFramework/PlayerController.h"

#include "WpGamePlayerController.generated.h"

class AWpCommander;
class AWpStrategicViewPawn;

class UInputAction;

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class AWpGamePlayerController final : public APlayerController
{
	GENERATED_BODY()

public:
	AWpGamePlayerController();

	virtual void SetupInputComponent() override;

	void SetStrategicViewPawn(AWpStrategicViewPawn* const InStrategicViewPawn);
	void SetCommander(AWpCommander* const InCommander);

private:
	void SetView(const bool bInStrategicView);
	void ToggleView();

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputAction> ToggleViewIa = nullptr;
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AWpCommander> CommanderClass = nullptr;

	bool bStrategicView = true;

	UPROPERTY(VisibleInstanceOnly)
	TObjectPtr<AWpStrategicViewPawn> StrategicViewPawn = nullptr;
	UPROPERTY(VisibleInstanceOnly)
	TObjectPtr<AWpCommander> Commander = nullptr;
};
