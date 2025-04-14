#pragma once

#include "CoreMinimal.h"

#include "Detour/DetourNavMeshQuery.h"
#include "NavMesh/RecastNavMesh.h"

#include "WpRecastNavMesh.generated.h"

UCLASS(MinimalAPI, NotBlueprintable, NotBlueprintType)
class AWpRecastNavMesh final : public ARecastNavMesh
{
	GENERATED_BODY()

public:
	AWpRecastNavMesh();

private:
	static FPathFindingResult FindPathPImpl(const FNavAgentProperties& AgentProperties, const FPathFindingQuery& Query);

	ENavigationQueryResult::Type FindPath(const FVector& StartLoc, const FVector& EndLoc,
	                                      const FVector::FReal& CostLimit, const bool bRequireNavigableEndLocation, FNavMeshPath& Path,
	                                      const FNavigationQueryFilter& InQueryFilter, const UObject* const InOwner) const;
	ENavigationQueryResult::Type PostProcessPath(const dtStatus Status, FNavMeshPath& Path,
	                                             const dtNavMeshQuery& NavQuery, const dtQueryFilter* const QueryFilter,
	                                             const NavNodeRef StartPolyID, const NavNodeRef EndPolyID,
	                                             const FVector& RecastStartPos, const FVector& RecastEndPos,
	                                             dtQueryResult& PathResult) const;
};
