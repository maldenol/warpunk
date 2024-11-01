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

	virtual void BeginPlay() override;

	virtual void SetupInputComponent() override;

private:
	void SetView(const bool bInStrategicView);
	void ToggleView();

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputAction> ToggleViewIa = nullptr;
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AWpCommander> CommanderClass = nullptr;

	bool bStrategicView = true;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<AWpStrategicViewPawn> StrategicViewPawn = nullptr;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<AWpCommander> Commander = nullptr;
};
