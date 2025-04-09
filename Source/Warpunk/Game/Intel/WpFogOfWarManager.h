#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Info.h"

#include "WpFogOfWarManager.generated.h"

UCLASS(MinimalAPI, Abstract, Blueprintable, NotBlueprintType)
class AWpFogOfWarManager final : public AInfo
{
	GENERATED_BODY()

public:
	AWpFogOfWarManager();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UStaticMesh* GetIntelMesh() const;
	UMaterialInterface* GetIntelMaterial() const;

	static AWpFogOfWarManager* Instance(const UWorld* const World);

private:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UTextureRenderTarget2D> TextureRenderTarget = nullptr;
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UMaterialInterface> FogOfWarMaterial = nullptr;
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMesh> IntelMesh = nullptr;
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UMaterialInterface> IntelMaterial = nullptr;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneCaptureComponent2D> SceneCaptureComponent = nullptr;

public:
	static const FName RootComponentName;
	static const FName SceneCaptureComponentName;

private:
	using FWpInstanceKey = const UWorld*;

	static TMap<FWpInstanceKey, AWpFogOfWarManager*> Instances;
};
