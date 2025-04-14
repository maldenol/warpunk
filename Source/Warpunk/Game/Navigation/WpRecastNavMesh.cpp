#include "WpRecastNavMesh.h"

#include "Warpunk/Game/Navigation/WpNavMeshQuery.h"

#include "AI/NavigationSystemBase.h"
#include "NavigationSystem.h"
#include "NavMesh/PImplRecastNavMesh.h"

AWpRecastNavMesh::AWpRecastNavMesh()
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		FindPathImplementation = FindPathPImpl;
		FindHierarchicalPathImplementation = FindPathPImpl;
	}
}

FPathFindingResult AWpRecastNavMesh::FindPathPImpl(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query)
{
	const AWpRecastNavMesh* const Self = Cast<AWpRecastNavMesh>(Query.NavData.Get());
	ensureAlways(Self);

	FPathFindingResult Result{ENavigationQueryResult::Error};

	FNavigationPath* NavPath = Query.PathInstanceToFill.Get();
	FNavMeshPath* NavMeshPath = NavPath ? NavPath->CastPath<FNavMeshPath>() : nullptr;

	if (NavMeshPath)
	{
		Result.Path = Query.PathInstanceToFill;
		NavMeshPath->ResetForRepath();
	}
	else
	{
		Result.Path = Self->CreatePathInstance<FNavMeshPath>(Query);
		NavPath = Result.Path.Get();
		NavMeshPath = NavPath ? NavPath->CastPath<FNavMeshPath>() : nullptr;
	}

	const FNavigationQueryFilter* const NavFilter = Query.QueryFilter.Get();
	if (NavMeshPath && NavFilter)
	{
		NavMeshPath->ApplyFlags(Query.NavDataFlags);

		const FVector AdjustedEndLocation = NavFilter->GetAdjustedEndLocation(Query.EndLocation);
		if ((Query.StartLocation - AdjustedEndLocation).IsNearlyZero() == true)
		{
			Result.Path->GetPathPoints().Reset();
			Result.Path->GetPathPoints().Add(FNavPathPoint(AdjustedEndLocation));
			Result.Result = ENavigationQueryResult::Success;
		}
		else
		{
			Result.Result = Self->FindPath(Query.StartLocation, AdjustedEndLocation, Query.CostLimit, Query.bRequireNavigableEndLocation, *NavMeshPath, *NavFilter, Query.Owner.Get());

			const bool bPartialPath = Result.IsPartial();
			if (bPartialPath)
			{
				Result.Result = Query.bAllowPartialPaths ? ENavigationQueryResult::Success : ENavigationQueryResult::Fail;
			}
		}
	}

	return Result;
}

ENavigationQueryResult::Type AWpRecastNavMesh::FindPath(const FVector& StartLoc, const FVector& EndLoc,
                                                        const FVector::FReal& CostLimit, const bool bRequireNavigableEndLocation, FNavMeshPath& Path,
                                                        const FNavigationQueryFilter& InQueryFilter, const UObject* const InOwner) const
{
	const dtNavMesh* const DetourNavMesh = GetRecastNavMeshImpl()->GetRecastMesh();
	if (!DetourNavMesh)
	{
		return ENavigationQueryResult::Error;
	}

	const FRecastQueryFilter* const FilterImplementation = static_cast<const FRecastQueryFilter*>(InQueryFilter.GetImplementation());
	if (!FilterImplementation)
	{
		return ENavigationQueryResult::Error;
	}

	const dtQueryFilter* const QueryFilter = FilterImplementation->GetAsDetourQueryFilter();
	if (!QueryFilter)
	{
		return ENavigationQueryResult::Error;
	}

	static dtNavMeshQuery StaticDetourNavQuery = {};
	dtNavMeshQuery* DetourNavQuery = &StaticDetourNavQuery;

	TUniquePtr<dtNavMeshQuery> LocalDetourNavQuery = nullptr;
	if (!IsInGameThread())
	{
		LocalDetourNavQuery = TUniquePtr<dtNavMeshQuery>{new dtNavMeshQuery{}};
		DetourNavQuery = LocalDetourNavQuery.Get();
	}

	FRecastSpeciaLinkFilter LinkFilter{FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()), InOwner};
	DetourNavQuery->init(DetourNavMesh, InQueryFilter.GetMaxSearchNodes(), &LinkFilter);
	DetourNavQuery->setRequireNavigableEndLocation(bRequireNavigableEndLocation);

	FVector RecastStartPos, RecastEndPos;
	NavNodeRef StartPolyID, EndPolyID;
	const bool bCanSearch = GetRecastNavMeshImpl()->InitPathfinding(StartLoc, EndLoc,
	                                                                *DetourNavQuery, QueryFilter, RecastStartPos, StartPolyID, RecastEndPos, EndPolyID);
	if (!bCanSearch)
	{
		return ENavigationQueryResult::Error;
	}

	dtQueryResult PathResult;
	FWpNavMeshQuery NavQuery{*DetourNavQuery};
	const dtStatus FindPathStatus = NavQuery.FindPath(StartPolyID, EndPolyID,
	                                                  &RecastStartPos.X, &RecastEndPos.X, CostLimit, QueryFilter, PathResult);
	NavQuery.MoveToDetourNavMeshQuery(*DetourNavQuery);

	return PostProcessPath(FindPathStatus, Path, *DetourNavQuery, QueryFilter, StartPolyID, EndPolyID,
	                       RecastStartPos, RecastEndPos, PathResult);
}

ENavigationQueryResult::Type AWpRecastNavMesh::PostProcessPath(const dtStatus Status, FNavMeshPath& Path,
                                                               const dtNavMeshQuery& NavQuery, const dtQueryFilter* const QueryFilter,
                                                               const NavNodeRef StartPolyID, const NavNodeRef EndPolyID,
                                                               const FVector& RecastStartPos, const FVector& RecastEndPos,
                                                               dtQueryResult& PathResult) const
{
	if ((PathResult.size() == 1) && dtStatusDetail(Status, DT_PARTIAL_RESULT))
	{
		FVector RecastHandPlacedPathEnd;
		NavQuery.closestPointOnPolyBoundary(StartPolyID, &RecastEndPos.X, &RecastHandPlacedPathEnd.X);

		new(Path.GetPathPoints()) FNavPathPoint(Recast2UnrealPoint(&RecastStartPos.X), StartPolyID);
		new(Path.GetPathPoints()) FNavPathPoint(Recast2UnrealPoint(&RecastHandPlacedPathEnd.X), StartPolyID);

		Path.PathCorridor.Add(PathResult.getRef(0));
		Path.PathCorridorCost.Add(GetRecastNavMeshImpl()->CalcSegmentCostOnPoly(
			StartPolyID, QueryFilter, RecastHandPlacedPathEnd, RecastStartPos));
	}
	else
	{
		GetRecastNavMeshImpl()->PostProcessPath(Status, Path, NavQuery, QueryFilter, StartPolyID, EndPolyID,
		                                        Recast2UnrealPoint(&RecastStartPos.X), Recast2UnrealPoint(&RecastEndPos.X),
		                                        RecastStartPos, RecastEndPos, PathResult);
	}

	if (dtStatusDetail(Status, DT_PARTIAL_RESULT))
	{
		Path.SetIsPartial(true);
		Path.SetSearchReachedLimit(dtStatusDetail(Status, DT_OUT_OF_NODES));
	}

	Path.MarkReady();

	return dtStatusSucceed(Status)
		       ? (dtStatusDetail(Status, DT_PARTIAL_RESULT) ? ENavigationQueryResult::Fail : ENavigationQueryResult::Success)
		       : (dtStatusDetail(Status, DT_INVALID_PARAM) ? ENavigationQueryResult::Error : ENavigationQueryResult::Fail);
}
