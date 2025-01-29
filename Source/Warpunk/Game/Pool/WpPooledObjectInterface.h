#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Pool/WpPooledObjectData.h"

#include "UObject/Interface.h"

#include "WpPooledObjectInterface.generated.h"

UINTERFACE(MinimalAPI, NotBlueprintable, NotBlueprintType)
class UWpPooledObjectInterface : public UInterface
{
	GENERATED_BODY()
};

class IWpPooledObjectInterface
{
	GENERATED_BODY()

protected:
	virtual void OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData) = 0;

	virtual void OnPooledObjectVisibilityChanged(const bool bIsVisible)
	{
	}

	virtual TArray<IWpPooledObjectInterface*> GetInnerPooledObjects()
	{
		return {};
	}

public:
	void AcquirePooledObject(const FWpPooledObjectData& PooledObjectData)
	{
		OnPooledObjectAcquiredOrReleased(true, PooledObjectData);

		for (IWpPooledObjectInterface* const InnerPooledObject : GetInnerPooledObjects())
		{
			InnerPooledObject->AcquirePooledObject(PooledObjectData);
		}
	}

	void ReleasePooledObject()
	{
		for (IWpPooledObjectInterface* const InnerPooledObject : GetInnerPooledObjects())
		{
			InnerPooledObject->ReleasePooledObject();
		}

		OnPooledObjectAcquiredOrReleased(false, {});
	}

	void SetPooledObjectVisibility(const bool bIsVisible)
	{
		OnPooledObjectVisibilityChanged(bIsVisible);

		for (IWpPooledObjectInterface* const InnerPooledObject : GetInnerPooledObjects())
		{
			InnerPooledObject->SetPooledObjectVisibility(bIsVisible);
		}
	}
};
