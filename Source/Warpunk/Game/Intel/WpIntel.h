#pragma once

#include "CoreMinimal.h"

#include "Warpunk/Game/Pool/WpPooledObjectInterface.h"

class UWpEntityComponent;

class FWpIntel final : public IWpPooledObjectInterface
{
public:
	virtual void OnPooledObjectAcquiredOrReleased(bool bIsAcquired, const void* const CustomData) override;

	void Initialize(UWpEntityComponent* const InOwnerEntityComponent);

	void AddIntel(UWpEntityComponent* const EntityComponent);
	void RemoveIntel(UWpEntityComponent* const EntityComponent);

	const TArray<UWpEntityComponent*>& GetIntel() const;

	static const FName IntelVolumeName;
	static const FName IntelVolumeCollisionProfileName;

private:
	UStaticMeshComponent* IntelVolume = nullptr;

	TArray<UWpEntityComponent*> Intel = {};
};
