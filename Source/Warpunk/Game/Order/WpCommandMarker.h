#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Actor.h"

#include "WpCommandMarker.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;
class UTexture2D;

enum class EWpOrderType : uint8;

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class AWpCommandMarker final : public AActor
{
	GENERATED_BODY()

public:
	AWpCommandMarker();

	virtual void BeginPlay() override;
	virtual void Tick(const float DeltaSeconds) override;

	void Play(const FVector& Location, const float Radius, const FLinearColor& InColor);
	void Preview(const FVector& Location, const float Radius, const FLinearColor& InColor);
	void HidePreview();

	static UStaticMesh* MarkerMesh();
	static UMaterialInterface* MarkerMaterial();
	static UTexture2D* RingTexture();
	static FLinearColor ColorForOrder(const EWpOrderType OrderType);

private:
	void EnsureMaterial();
	void Apply(const float Radius, const FLinearColor& InColor, const float Opacity);

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Mesh = nullptr;
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> Material = nullptr;

	bool bPlaying = false;
	float Age = 0.0f;
	float Duration = 0.9f;
	float ShownRadius = 0.0f;
	FVector GroundLocation = FVector::ZeroVector;
	FLinearColor Color = FLinearColor::White;

	static constexpr float Lifetime = 0.9f;
};
