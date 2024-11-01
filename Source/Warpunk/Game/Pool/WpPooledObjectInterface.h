#pragma once

#include "CoreMinimal.h"

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
	virtual void OnPooledObjectAcquiredOrReleased(const bool bIsAcquired, const void* const CustomData) = 0;

	virtual TArray<IWpPooledObjectInterface*> GetInnerPooledObjects()
	{
		return {};
	}

	void AcquirePooledObject(const void* const CustomData)
	{
		OnPooledObjectAcquiredOrReleased(true, CustomData);

		for (IWpPooledObjectInterface* const InnerPooledObject : GetInnerPooledObjects())
		{
			InnerPooledObject->AcquirePooledObject(CustomData);
		}
	}

	void ReleasePooledObject()
	{
		for (IWpPooledObjectInterface* const InnerPooledObject : GetInnerPooledObjects())
		{
			InnerPooledObject->ReleasePooledObject();
		}

		OnPooledObjectAcquiredOrReleased(false, nullptr);
	}
};
