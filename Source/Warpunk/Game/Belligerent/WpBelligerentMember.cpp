#include "WpBelligerentMember.h"

#include "Warpunk/Game/EntityComponent/WpEntityComponent.h"

bool FWpBelligerentMember::BelongsToSameBelligerent(const uint32 OtherBelligerentId) const
{
	return BelligerentId == OtherBelligerentId;
}

bool FWpBelligerentMember::BelongsToSameBelligerent(const UWpEntityComponent* const OtherEntityComponent) const
{
	const uint32 OtherBelligerentId = WP_AS_BELLIGERENT_MEMBER_CONST(OtherEntityComponent).BelligerentId;
	return BelongsToSameBelligerent(OtherBelligerentId);
}

bool FWpBelligerentMember::BelongsToSameTeam(const uint32 OtherBelligerentId) const
{
	return BelligerentId == OtherBelligerentId;
}

bool FWpBelligerentMember::BelongsToSameTeam(const UWpEntityComponent* const OtherEntityComponent) const
{
	const uint32 OtherBelligerentId = WP_AS_BELLIGERENT_MEMBER_CONST(OtherEntityComponent).BelligerentId;
	return BelongsToSameTeam(OtherBelligerentId);
}

void FWpBelligerentMember::SetBelligerentId(const uint32 OtherBelligerentId)
{
	BelligerentId = OtherBelligerentId;
}

void FWpBelligerentMember::SetBelligerentId(const UWpEntityComponent* const OtherEntityComponent)
{
	const uint32 OtherBelligerentId = WP_AS_BELLIGERENT_MEMBER_CONST(OtherEntityComponent).BelligerentId;
	SetBelligerentId(OtherBelligerentId);
}

uint32 FWpBelligerentMember::GetBelligerentId() const
{
	return BelligerentId;
}
