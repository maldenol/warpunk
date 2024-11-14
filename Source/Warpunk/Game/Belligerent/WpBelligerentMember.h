#pragma once

#include "CoreMinimal.h"

class UWpEntityComponent;

class FWpBelligerentMember final
{
public:
	bool BelongsToSameBelligerent(const uint32 OtherBelligerentId) const;
	bool BelongsToSameBelligerent(const UWpEntityComponent* const OtherEntityComponent) const;
	bool BelongsToSameTeam(const uint32 OtherBelligerentId) const;
	bool BelongsToSameTeam(const UWpEntityComponent* const OtherEntityComponent) const;

	void SetBelligerentId(const uint32 OtherBelligerentId);
	void SetBelligerentId(const UWpEntityComponent* const OtherEntityComponent);
	uint32 GetBelligerentId() const;

private:
	uint32 BelligerentId = 0;
};
