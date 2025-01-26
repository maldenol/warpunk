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
	DECLARE_EVENT(AWpGamePlayerController, FPlayerStateReplicatedEvent);

	AWpGamePlayerController();

	virtual void SetupInputComponent() override;

	virtual void OnRep_PlayerState() override;

	void SetStrategicViewPawn(AWpStrategicViewPawn* const InStrategicViewPawn);
	void SetCommander(AWpCommander* const InCommander);

	UFUNCTION(Client, Reliable)
	void ClientSetExpectedActorNum(const uint32 ExpectedActorNum);

	FPlayerStateReplicatedEvent& OnPlayerStateReplicated();

private:
	UFUNCTION(Server, Reliable)
	void ServerToggleView();

	void SetView(const bool bStrategicView);

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputAction> ToggleViewIa = nullptr;
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AWpCommander> CommanderClass = nullptr;

	UPROPERTY(VisibleInstanceOnly)
	TObjectPtr<AWpStrategicViewPawn> StrategicViewPawn = nullptr;
	UPROPERTY(VisibleInstanceOnly)
	TObjectPtr<AWpCommander> Commander = nullptr;

	FPlayerStateReplicatedEvent PlayerStateReplicatedEvent = {};
};
