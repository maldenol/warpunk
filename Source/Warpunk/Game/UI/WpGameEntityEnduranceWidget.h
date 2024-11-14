#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"

#include "WpGameEntityEnduranceWidget.generated.h"

class FWpEndurance;

class UProgressBar;

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class UWpGameEntityEnduranceWidget final : public UUserWidget
{
	GENERATED_BODY()

public:
	UWpGameEntityEnduranceWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void NativeConstruct() override;

	void SetEndurance(FWpEndurance* const Endurance);

private:
	void OnCurrentEnduranceChanged(const float CurrentEndurance, const float MaxEndurance);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar = nullptr;
};
