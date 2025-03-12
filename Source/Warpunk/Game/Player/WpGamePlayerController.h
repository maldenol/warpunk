#pragma once

#include "CoreMinimal.h"

#include "GameFramework/PlayerController.h"

#include "WpGamePlayerController.generated.h"

class AWpCommander;
class AWpStrategicViewPawn;
class UWpOrderSenderComponent;

class UInputAction;

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class AWpGamePlayerController final : public APlayerController
{
	GENERATED_BODY()

public:
	AWpGamePlayerController();

	virtual void BeginPlay() override;

	virtual void OnRep_PlayerState() override;

	virtual void SetupInputComponent() override;
	virtual void SetControlRotation(const FRotator& NewRotation) override;

	void SetStrategicViewPawn(AWpStrategicViewPawn* const InStrategicViewPawn);
	void SetCommander(AWpCommander* const InCommander);

	UWpOrderSenderComponent* GetOrderSenderComponent();
	const UWpOrderSenderComponent* GetOrderSenderComponent() const;

	UFUNCTION(Client, Reliable)
	void ClientSetExpectedLoadedObjectNum(const uint32 ExpectedLoadedObjectNum);

	UFUNCTION(Server, Unreliable)
	void ServerAddCommanderMovementInput(const float Forward, const float Right);

	void NotifyCommanderDestroyed();

private:
	UFUNCTION(Server, Unreliable)
	void ServerSetControlRotation(const FRotator& NewRotation);

	UFUNCTION(Server, Reliable)
	void ServerToggleView();

	void SetView(const bool bStrategicView);

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputAction> ToggleViewIa = nullptr;

	UPROPERTY(VisibleInstanceOnly)
	TObjectPtr<AWpStrategicViewPawn> StrategicViewPawn = nullptr;
	UPROPERTY(VisibleInstanceOnly)
	TObjectPtr<AWpCommander> Commander = nullptr;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UWpOrderSenderComponent> OrderSenderComponent = nullptr;

	bool bHasNotifiedObjectLoaded = false;

public:
	static const FName OrderSenderComponentName;
};
