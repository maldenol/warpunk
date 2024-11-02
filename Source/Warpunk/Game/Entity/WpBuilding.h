#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/EntityComponent/WpEntityInterface.h"
#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"

#include "GameFramework/Actor.h"

#include "WpBuilding.generated.h"

class UBoxComponent;

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class AWpBuilding final : public AActor, public IWpEntityInterface, public IWpPooledObjectInterface
{
	GENERATED_BODY()

public:
	AWpBuilding();

	virtual UWpEntityComponent* GetEntityComponent() override;
	virtual const UWpEntityComponent* GetEntityComponent() const override;

	virtual void OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const void* const CustomData) override;
	virtual TArray<IWpPooledObjectInterface*> GetInnerPooledObjects() override;

	static const FName BoxComponentName;
	static const FName MeshComponentName;
	static const FName EntityComponentName;
	static const FName BoxComponentCollisionProfileName;
	static const FName MeshComponentCollisionProfileName;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UBoxComponent> BoxComponent = nullptr;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USkeletalMeshComponent> MeshComponent = nullptr;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UWpEntityComponent> EntityComponent = nullptr;
};
