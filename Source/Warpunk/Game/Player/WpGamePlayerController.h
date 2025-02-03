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

	virtual void OnRep_PlayerState() override;

	void SetStrategicViewPawn(AWpStrategicViewPawn* const InStrategicViewPawn);
	void SetCommander(AWpCommander* const InCommander);

	UFUNCTION(Client, Reliable)
	void ClientSetExpectedLoadedObjectNum(const uint32 ExpectedLoadedObjectNum);

	UFUNCTION(Server, Unreliable)
	void ServerAddCommanderMovementInput(const float Forward, const float Right);

private:
	UFUNCTION(Server, Reliable)
	void ServerToggleView();

	void SetView(const bool bStrategicView);

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputAction> ToggleViewIa = nullptr;

	UPROPERTY(VisibleInstanceOnly)
	TObjectPtr<AWpStrategicViewPawn> StrategicViewPawn = nullptr;
	UPROPERTY(VisibleInstanceOnly)
	TObjectPtr<AWpCommander> Commander = nullptr;

	bool bHasNotifiedObjectLoaded = false;
};
