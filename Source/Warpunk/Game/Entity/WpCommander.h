#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Entity/WpUnit.h"

#include "WpCommander.generated.h"

class AWpUnitAIController;

struct FInputActionValue;
class UInputAction;
class UInputMappingContext;

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class AWpCommander final : public AWpUnit
{
	GENERATED_BODY()

public:
	AWpCommander();

	virtual void PossessedBy(AController* const NewController) override;
	virtual void UnPossessed() override;

	virtual void SetupPlayerInputComponent(UInputComponent* const PlayerInputComponent) override;

private:
	UFUNCTION(Client, Reliable)
	void ClientPossessed();
	UFUNCTION(Client, Reliable)
	void ClientUnPossessed();

	void OnMoveTriggered(const FInputActionValue& ActionValue);
	void OnLookTriggered(const FInputActionValue& ActionValue);
	UFUNCTION(Server, Reliable)
	void ServerOnFireTriggered(const FInputActionValue& ActionValue);

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputMappingContext> TacticalViewImc = nullptr;
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputAction> MoveIa = nullptr;
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputAction> LookIa = nullptr;
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UInputAction> FireIa = nullptr;

	UPROPERTY(VisibleInstanceOnly)
	TObjectPtr<AWpUnitAIController> AIController = nullptr;
};
