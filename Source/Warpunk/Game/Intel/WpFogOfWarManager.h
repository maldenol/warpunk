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

	UStaticMesh* GetIntelMesh() const;
	UMaterialInterface* GetIntelMaterial() const;

	static AWpFogOfWarManager* Instance(const UObject* const WorldContextObject);

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

	static const FName RootComponentName;
	static const FName SceneCaptureComponentName;
};
