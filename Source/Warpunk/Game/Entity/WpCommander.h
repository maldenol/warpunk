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

	virtual void BeginPlay() override;

	virtual void SetupPlayerInputComponent(UInputComponent* const PlayerInputComponent) override;
	virtual void PossessedBy(AController* const NewController) override;
	virtual void UnPossessed() override;

private:
	void OnMoveTriggered(const FInputActionValue& ActionValue);
	void OnLookTriggered(const FInputActionValue& ActionValue);
	void OnFireTriggered(const FInputActionValue& ActionValue);

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
