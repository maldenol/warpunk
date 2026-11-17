#include "WpCommandMarker.h"

#include "Warpunk/Game/Order/WpOrder.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"

AWpCommandMarker::AWpCommandMarker()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ring"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetVisibility(false);

	SetActorEnableCollision(false);
	SetCanBeDamaged(false);
}

void AWpCommandMarker::BeginPlay()
{
	Super::BeginPlay();

	EnsureMaterial();
}

void AWpCommandMarker::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bPlaying)
	{
		return;
	}

	Age += DeltaSeconds;
	const float Time = FMath::Clamp(Age / Duration, 0.0f, 1.0f);
	const float Opacity = 1.0f - Time;
	const float Radius = ShownRadius * FMath::Lerp(0.86f, 1.12f, Time);
	Apply(Radius, Color, Opacity);

	if (Age >= Duration)
	{
		Destroy();
	}
}

void AWpCommandMarker::Play(const FVector& Location, const float Radius, const FLinearColor& InColor)
{
	EnsureMaterial();

	bPlaying = true;
	Age = 0.0f;
	Duration = Lifetime;
	GroundLocation = Location;
	ShownRadius = FMath::Max(Radius, 110.0f);
	Color = InColor;
	SetActorTickEnabled(true);
	SetActorHiddenInGame(false);
	Apply(ShownRadius, Color, 1.0f);
}

void AWpCommandMarker::Preview(const FVector& Location, const float Radius, const FLinearColor& InColor)
{
	EnsureMaterial();

	bPlaying = false;
	GroundLocation = Location;
	ShownRadius = FMath::Max(Radius, 110.0f);
	Color = InColor;
	SetActorTickEnabled(false);
	SetActorHiddenInGame(false);
	Apply(ShownRadius, Color, 0.72f);
}

void AWpCommandMarker::HidePreview()
{
	bPlaying = false;
	SetActorTickEnabled(false);
	SetActorHiddenInGame(true);
	if (Mesh)
	{
		Mesh->SetVisibility(false);
	}
}

void AWpCommandMarker::EnsureMaterial()
{
	if (Material)
	{
		return;
	}

	UStaticMesh* const Plane = MarkerMesh();
	UMaterialInterface* const MaterialInterface = MarkerMaterial();
	ensureAlways(Plane);
	ensureAlways(MaterialInterface);
	if (!Plane || !MaterialInterface)
	{
		return;
	}

	Mesh->SetStaticMesh(Plane);
	Material = UMaterialInstanceDynamic::Create(MaterialInterface, this);
	Material->SetTextureParameterValue(TEXT("Texture"), RingTexture());
	Mesh->SetMaterial(0, Material);
}

void AWpCommandMarker::Apply(const float Radius, const FLinearColor& InColor, const float Opacity)
{
	const float Diameter = FMath::Max(Radius, 40.0f) * 2.0f;
	const float Scale = Diameter / 100.0f;
	SetActorLocation(GroundLocation + FVector{0.0f, 0.0f, 8.0f});
	Mesh->SetRelativeScale3D(FVector{Scale, Scale, 1.0f});
	Mesh->SetVisibility(true);
	if (Material)
	{
		Material->SetVectorParameterValue(TEXT("Color"), InColor * Opacity);
		Material->SetScalarParameterValue(TEXT("Opacity"), Opacity);
	}
}

UStaticMesh* AWpCommandMarker::MarkerMesh()
{
	static TObjectPtr<UStaticMesh> MeshAsset = nullptr;
	if (!MeshAsset)
	{
		MeshAsset = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
		if (MeshAsset)
		{
			MeshAsset->AddToRoot();
		}
	}
	return MeshAsset;
}

UMaterialInterface* AWpCommandMarker::MarkerMaterial()
{
	static TObjectPtr<UMaterialInterface> MaterialAsset = nullptr;
	if (!MaterialAsset)
	{
		MaterialAsset = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/EmissiveMeshMaterial.EmissiveMeshMaterial"));
		if (MaterialAsset)
		{
			MaterialAsset->AddToRoot();
		}
	}
	return MaterialAsset;
}

UTexture2D* AWpCommandMarker::RingTexture()
{
	static TObjectPtr<UTexture2D> Texture = nullptr;
	if (Texture)
	{
		return Texture;
	}

	constexpr int32 Size = 256;
	TArray<uint8> Data;
	Data.SetNumZeroed(Size * Size * 4);
	const FVector2D Center{127.5f, 127.5f};
	for (int32 Y = 0; Y < Size; ++Y)
	{
		for (int32 X = 0; X < Size; ++X)
		{
			const FVector2D Point{static_cast<float>(X) + 0.5f, static_cast<float>(Y) + 0.5f};
			const float Distance = (Point - Center).Size();
			const float Ring = 1.0f - FMath::SmoothStep(0.0f, 2.5f, FMath::Abs(Distance - 108.0f) - 8.0f);
			const uint8 Value = static_cast<uint8>(FMath::Clamp(Ring, 0.0f, 1.0f) * 255.0f);
			const int32 Index = (Y * Size + X) * 4;
			Data[Index + 0] = Value;
			Data[Index + 1] = Value;
			Data[Index + 2] = Value;
			Data[Index + 3] = Value;
		}
	}

	Texture = UTexture2D::CreateTransient(Size, Size, PF_B8G8R8A8);
	Texture->SRGB = false;
	Texture->NeverStream = true;
	Texture->Filter = TF_Bilinear;
	Texture->AddressX = TA_Clamp;
	Texture->AddressY = TA_Clamp;
	FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
	void* const Dest = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Dest, Data.GetData(), Data.Num());
	Mip.BulkData.Unlock();
	Texture->UpdateResource();
	Texture->AddToRoot();
	return Texture;
}

FLinearColor AWpCommandMarker::ColorForOrder(const EWpOrderType OrderType)
{
	switch (OrderType)
	{
	case EWpOrderType::Move:
		return FLinearColor{0.04f, 0.34f, 0.32f, 1.0f};
	case EWpOrderType::Patrol:
		return FLinearColor{0.06f, 0.36f, 0.14f, 1.0f};
	case EWpOrderType::Assist:
		return FLinearColor{0.08f, 0.22f, 0.52f, 1.0f};
	case EWpOrderType::Attack:
		return FLinearColor{0.48f, 0.08f, 0.08f, 1.0f};
	case EWpOrderType::Strike:
		return FLinearColor{0.52f, 0.24f, 0.04f, 1.0f};
	case EWpOrderType::Repair:
		return FLinearColor{0.46f, 0.34f, 0.05f, 1.0f};
	case EWpOrderType::Reclaim:
		return FLinearColor{0.32f, 0.10f, 0.46f, 1.0f};
	case EWpOrderType::InfiniteUnitProductionOn:
		return FLinearColor{0.04f, 0.30f, 0.44f, 1.0f};
	case EWpOrderType::InfiniteUnitProductionOff:
		return FLinearColor{0.14f, 0.18f, 0.38f, 1.0f};
	case EWpOrderType::SelfDestruct:
		return FLinearColor{0.46f, 0.06f, 0.24f, 1.0f};
	case EWpOrderType::Stop:
		return FLinearColor{0.28f, 0.12f, 0.16f, 1.0f};
	default:
		return FLinearColor{0.16f, 0.20f, 0.24f, 1.0f};
	}
}
