#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Pawn.h"

#include "WpStrategicViewPawn.generated.h"

class UWpOrderSenderComponent;

struct FInputActionValue;
class UFloatingPawnMovement;
class UInputAction;
class UInputMappingContext;

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class AWpStrategicViewPawn final : public APawn
{
	GENERATED_BODY()

public:
	AWpStrategicViewPawn();

	virtual void PossessedBy(AController* const NewController) override;
	virtual void UnPossessed() override;

	virtual void SetupPlayerInputComponent(UInputComponent* const PlayerInputComponent) override;

	UWpOrderSenderComponent* GetOrderSenderComponent();
	const UWpOrderSenderComponent* GetOrderSenderComponent() const;

private:
	UFUNCTION(Client, Reliable)
	void ClientPossessed();
	UFUNCTION(Client, Reliable)
	void ClientUnPossessed();

	UFUNCTION(Server, Unreliable)
	void ServerUpdateLocation(const FVector& Location);

	void OnMoveTriggered(const FInputActionValue& ActionValue);
	void OnLMBStarted();
	void OnLMBTriggered();
	void OnLMBCompleted();
	void OnRMBStarted();
	void OnRMBTriggered();
	void OnRMBCompleted();

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputMappingContext> StrategicViewImc = nullptr;
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputAction> MoveIa = nullptr;
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputAction> LmbIa = nullptr;
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputAction> RmbIa = nullptr;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UFloatingPawnMovement> MovementComponent = nullptr;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UWpOrderSenderComponent> OrderSenderComponent = nullptr;

	static const FName MovementComponentName;
	static const FName OrderSenderComponentName;
};
