#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Order/WpOrderSender.h"

#include "GameFramework/Pawn.h"

#include "WpStrategicViewPawn.generated.h"

struct FInputActionValue;
class UFloatingPawnMovement;
class UInputAction;
class UInputMappingContext;

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class AWpStrategicViewPawn final : public APawn, public FWpOrderSender
{
	GENERATED_BODY()

public:
	AWpStrategicViewPawn();

	virtual void Tick(const float DeltaTime) override;

	virtual void SetupPlayerInputComponent(UInputComponent* const PlayerInputComponent) override;
	virtual void PossessedBy(AController* const NewController) override;
	virtual void UnPossessed() override;

	static const FName MovementComponentName;

private:
	void OnMoveTriggered(const FInputActionValue& ActionValue);

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
};
