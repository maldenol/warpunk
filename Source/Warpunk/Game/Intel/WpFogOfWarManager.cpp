#include "WpFogOfWarManager.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Kismet/GameplayStatics.h"

const FName AWpFogOfWarManager::RootComponentName = TEXT("RootComponent");
const FName AWpFogOfWarManager::SceneCaptureComponentName = TEXT("SceneCaptureComponent");

AWpFogOfWarManager::AWpFogOfWarManager()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = false;
	bNetLoadOnClient = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(RootComponentName);

	SceneCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(SceneCaptureComponentName);
	SceneCaptureComponent->AttachToComponent(RootComponent, FAttachmentTransformRules{EAttachmentRule::KeepRelative, false});
}

void AWpFogOfWarManager::BeginPlay()
{
	Super::BeginPlay();

	SceneCaptureComponent->SetRelativeLocation(FVector{0.0f, 0.0f, 1000.0f});
	SceneCaptureComponent->SetRelativeRotation(FRotator::MakeFromEuler(FVector{0.0f, 270.0f, 0.0f}));

	SceneCaptureComponent->ProjectionType = ECameraProjectionMode::Type::Orthographic;
	SceneCaptureComponent->OrthoWidth = 32768.0f;
	SceneCaptureComponent->CaptureSource = SCS_FinalColorLDR;

	ensureAlways(TextureRenderTarget);
	ensureAlways(FogOfWarMaterial);
	ensureAlways(IntelMesh);
	ensureAlways(IntelMaterial);

	SceneCaptureComponent->TextureTarget = TextureRenderTarget;
	SceneCaptureComponent->PostProcessSettings.AddBlendable(FogOfWarMaterial, 1.0f);
}

UStaticMesh* AWpFogOfWarManager::GetIntelMesh() const
{
	return IntelMesh;
}

UMaterialInterface* AWpFogOfWarManager::GetIntelMaterial() const
{
	return IntelMaterial;
}

AWpFogOfWarManager* AWpFogOfWarManager::Instance(const UObject* const WorldContextObject)
{
	return Cast<AWpFogOfWarManager>(UGameplayStatics::GetActorOfClass(WorldContextObject, StaticClass()));
}
