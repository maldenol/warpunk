#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Pawn.h"

#include "WpStrategicViewPawn.generated.h"

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

	virtual bool IsNetRelevantFor(const AActor* const RealViewer, const AActor* const ViewTarget, const FVector& SrcLocation) const override;

	virtual void PossessedBy(AController* const NewController) override;
	virtual void UnPossessed() override;

	virtual void SetupPlayerInputComponent(UInputComponent* const PlayerInputComponent) override;

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
	const AActor* OwningPlayerController = nullptr;

	bool bHasNotifiedObjectLoaded = false;

public:
	static const FName MovementComponentName;
};
