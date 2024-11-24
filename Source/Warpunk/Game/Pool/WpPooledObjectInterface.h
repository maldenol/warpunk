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

public:
	virtual void OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const FWpPooledObjectData& PooledObjectData) = 0;

	virtual TArray<IWpPooledObjectInterface*> GetInnerPooledObjects()
	{
		return {};
	}

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
};
