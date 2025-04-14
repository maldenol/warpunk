#pragma once

#include "CoreMinimal.h"

#include "Detour/DetourNavMeshQuery.h"

class FWpNavMeshQuery final
{
public:
	FWpNavMeshQuery(const dtNavMeshQuery& DetourNavMeshQuery);

	dtStatus FindPath(const dtPolyRef StartRef, const dtPolyRef EndRef,
	                  const dtReal* const StartPos, const dtReal* const EndPos,
	                  const dtReal CostLimit, const dtQueryFilter* const QueryFilter, dtQueryResult& OutQueryResult) const;
	bool PassLinkFilterByRef(const dtMeshTile* const MeshTile, const dtPolyRef PolyRef) const;
	dtStatus GetEdgeMidPoint(const dtPolyRef FromPolyRef, const dtPoly* const FromPoly, const dtMeshTile* const FromMeshTile,
	                         const dtPolyRef ToPolyRef, const dtPoly* const ToPoly, const dtMeshTile* const ToMeshTile,
	                         dtReal* const OutMidPoint) const;
	dtStatus GetPortalPoints(const dtPolyRef FromPolyRef, const dtPoly* const FromPoly, const dtMeshTile* const FromMeshTile,
	                         const dtPolyRef ToPolyRef, const dtPoly* const ToPoly, const dtMeshTile* const ToMeshTile,
	                         dtReal* const OutLeftPoint, dtReal* const OutRightPoint) const;

	void MoveToDetourNavMeshQuery(dtNavMeshQuery& DetourNavMeshQuery);

private:
	const dtNavMesh* NavMesh = nullptr;
	dtQuerySpecialLinkFilter* LinkFilter = nullptr;

	struct FWpQueryData
	{
		dtStatus Status = DT_FAILURE;
		dtNode* LastBestNode = nullptr;
		dtReal LastBestNodeCost = 0.0f;
		dtPolyRef StartRef = 0;
		dtPolyRef EndRef = 0;
		dtReal StartPos[3] = {};
		dtReal EndPos[3] = {};
		dtReal CostLimit = 0.0f;
		const dtQueryFilter* Filter = nullptr;
		unsigned char bRequireNavigableEndLocation : 1 = 0;
	};

	FWpQueryData QueryData = {};

	dtNodePool* TinyNodePool = nullptr;
	dtNodePool* NodePool = nullptr;
	dtNodeQueue* OpenNodeList = nullptr;

	mutable int QueriedNodeNum = 0;
};

class FWpNavQueryResult
{
public:
	void Reserve(const int Size);
	void AddItem(const dtPolyRef Ref, const dtReal Cost, const dtReal* const Pos, const unsigned int Flag);

	void MoveToDetourQueryResult(dtQueryResult& DetourQueryResult);

private:
	dtChunkArray<dtQueryResultPack> Data = {};
};
