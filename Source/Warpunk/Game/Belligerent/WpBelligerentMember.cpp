#include "WpBelligerentMember.h"

#include "Warpunk/Game/EntityComponent/WpEntityComponent.h"

bool FWpBelligerentMember::BelongsToSameBelligerent(const uint32 OtherBelligerentID) const
{
	return BelligerentID == OtherBelligerentID;
}

bool FWpBelligerentMember::BelongsToSameBelligerent(const UWpEntityComponent* const OtherEntityComponent) const
{
	const uint32 OtherBelligerentID = WP_AS_BELLIGERENT_MEMBER_CONST(OtherEntityComponent).BelligerentID;
	return BelongsToSameBelligerent(OtherBelligerentID);
}

bool FWpBelligerentMember::BelongsToSameTeam(const uint32 OtherBelligerentID) const
{
	return BelligerentID == OtherBelligerentID;
}

bool FWpBelligerentMember::BelongsToSameTeam(const UWpEntityComponent* const OtherEntityComponent) const
{
	const uint32 OtherBelligerentID = WP_AS_BELLIGERENT_MEMBER_CONST(OtherEntityComponent).BelligerentID;
	return BelongsToSameTeam(OtherBelligerentID);
}

void FWpBelligerentMember::SetBelligerentID(const uint32 OtherBelligerentID)
{
	BelligerentID = OtherBelligerentID;
}

void FWpBelligerentMember::SetBelligerentID(const UWpEntityComponent* const OtherEntityComponent)
{
	const uint32 OtherBelligerentID = WP_AS_BELLIGERENT_MEMBER_CONST(OtherEntityComponent).BelligerentID;
	SetBelligerentID(OtherBelligerentID);
}

uint32 FWpBelligerentMember::GetBelligerentID() const
{
	return BelligerentID;
}
