#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/EntityComponent/WpEntityInterface.h"
#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"

#include "GameFramework/Character.h"

#include "WpUnit.generated.h"

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class AWpUnit : public ACharacter, public IWpEntityInterface, public IWpPooledObjectInterface
{
	GENERATED_BODY()

public:
	AWpUnit();

	virtual UWpEntityComponent* GetEntityComponent() override;
	virtual const UWpEntityComponent* GetEntityComponent() const override;

	virtual void OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const void* const CustomData) override;
	virtual TArray<IWpPooledObjectInterface*> GetInnerPooledObjects() override;

	static const FName EntityComponentName;
	static const FName CapsuleComponentCollisionProfileName;
	static const FName MeshComponentCollisionProfileName;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UWpEntityComponent> EntityComponent = nullptr;
};
