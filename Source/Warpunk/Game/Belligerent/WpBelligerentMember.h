#pragma once

#include "CoreMinimal.h"

class UWpEntityComponent;

class FWpBelligerentMember final
{
public:
	bool BelongsToSameBelligerent(const uint32 OtherBelligerentID) const;
	bool BelongsToSameBelligerent(const UWpEntityComponent* const OtherEntityComponent) const;
	bool BelongsToSameTeam(const uint32 OtherBelligerentID) const;
	bool BelongsToSameTeam(const UWpEntityComponent* const OtherEntityComponent) const;

	void SetBelligerentID(const uint32 OtherBelligerentID);
	void SetBelligerentID(const UWpEntityComponent* const OtherEntityComponent);
	uint32 GetBelligerentID() const;

private:
	uint32 BelligerentID = 0;
};
