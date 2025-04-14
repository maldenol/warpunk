#include "WpNavMeshQuery.h"

#include "Detour/DetourNode.h"

FWpNavMeshQuery::FWpNavMeshQuery(const dtNavMeshQuery& DetourNavMeshQuery)
{
	static_assert(sizeof(FWpNavMeshQuery) == sizeof(dtNavMeshQuery));
	memcpy(this, &DetourNavMeshQuery, sizeof(dtNavMeshQuery));
}

dtStatus FWpNavMeshQuery::FindPath(const dtPolyRef StartRef, const dtPolyRef EndRef,
                                   const dtReal* const StartPos, const dtReal* const EndPos,
                                   const dtReal CostLimit, const dtQueryFilter* const QueryFilter, dtQueryResult& OutQueryResult) const
{
	dtAssert(NavMesh);
	dtAssert(NodePool);
	dtAssert(OpenNodeList);

	QueriedNodeNum = 0;

	if (!StartRef || (QueryData.bRequireNavigableEndLocation && !EndRef))
	{
		return DT_FAILURE | DT_INVALID_PARAM;
	}

	if (!NavMesh->isValidPolyRef(StartRef) || (!NavMesh->isValidPolyRef(EndRef) && (EndRef || QueryData.bRequireNavigableEndLocation)))
	{
		return DT_FAILURE | DT_INVALID_PARAM;
	}

	FWpNavQueryResult NavQueryResult = {};

	if (StartRef == EndRef)
	{
		NavQueryResult.AddItem(StartRef, 0.0f, nullptr, 0);
		NavQueryResult.MoveToDetourQueryResult(OutQueryResult);
		return DT_SUCCESS;
	}

	const dtReal HeuristicCostScale = QueryFilter->getModifiedHeuristicScale();
	const bool bShouldIgnoreClosedNodes = QueryFilter->getShouldIgnoreClosedNodes();

	NodePool->clear();
	OpenNodeList->clear();

	dtNode* const StartNode = NodePool->getNode(StartRef);
	dtVcopy(StartNode->pos, StartPos);
	StartNode->pidx = 0;
	StartNode->cost = 0.0f;
	StartNode->total = dtVdist(StartPos, EndPos) * HeuristicCostScale;
	StartNode->id = StartRef;
	StartNode->flags = DT_NODE_OPEN;
	OpenNodeList->push(StartNode);
	++QueriedNodeNum;

	dtNode* LastBestNode = StartNode;
	dtReal LastBestNodeCost = StartNode->total;

	dtStatus Status = DT_SUCCESS;

	int IterationIndex = 0;
	const int MaxIterationNum = NodePool->getMaxRuntimeNodes() + 1;

	while (!OpenNodeList->empty())
	{
		dtNode* const BestNode = OpenNodeList->pop();
		BestNode->flags &= ~DT_NODE_OPEN;
		BestNode->flags |= DT_NODE_CLOSED;

		if (BestNode->id == EndRef)
		{
			LastBestNode = BestNode;
			break;
		}

		++IterationIndex;
		if (IterationIndex >= MaxIterationNum * 4)
		{
			break;
		}

		const dtPolyRef BestPolyRef = BestNode->id;
		const dtMeshTile* BestMeshTile = nullptr;
		const dtPoly* BestPoly = nullptr;
		NavMesh->getTileAndPolyByRefUnsafe(BestPolyRef, &BestMeshTile, &BestPoly);

		dtPolyRef ParentPolyRef = 0;
		const dtMeshTile* ParentMeshTile = nullptr;
		const dtPoly* ParentPoly = nullptr;
		if (BestNode->pidx)
		{
			ParentPolyRef = NodePool->getNodeAtIdx(BestNode->pidx)->id;
		}
		if (ParentPolyRef)
		{
			NavMesh->getTileAndPolyByRefUnsafe(ParentPolyRef, &ParentMeshTile, &ParentPoly);
		}

		unsigned int i = BestPoly->firstLink;
		while (i != DT_NULL_LINK)
		{
			const dtLink& Link = NavMesh->getLink(BestMeshTile, i);
			i = Link.next;

			const dtPolyRef NeighbourPolyRef = Link.ref;

			if (!NeighbourPolyRef || (NeighbourPolyRef == ParentPolyRef) || !QueryFilter->isValidLinkSide(Link.side))
			{
				continue;
			}

			const dtMeshTile* NeighbourMeshTile = nullptr;
			const dtPoly* NeighbourPoly = nullptr;
			NavMesh->getTileAndPolyByRefUnsafe(NeighbourPolyRef, &NeighbourMeshTile, &NeighbourPoly);

			if (!QueryFilter->passFilter(NeighbourPolyRef, NeighbourMeshTile, NeighbourPoly)
				|| !PassLinkFilterByRef(NeighbourMeshTile, NeighbourPolyRef))
			{
				continue;
			}

			dtNode* const NeighbourNode = NodePool->getNode(NeighbourPolyRef);
			if (!NeighbourNode)
			{
				Status |= DT_OUT_OF_NODES;
				continue;
			}
			if (bShouldIgnoreClosedNodes && (NeighbourNode->flags & DT_NODE_CLOSED))
			{
				continue;
			}

			dtReal NeighbourPos[3] = {0.0f, 0.0f, 0.0f};
			if ((HeuristicCostScale <= 1.0f) || !NeighbourNode->flags)
			{
				GetEdgeMidPoint(BestPolyRef, BestPoly, BestMeshTile,
				                NeighbourPolyRef, NeighbourPoly, NeighbourMeshTile,
				                NeighbourPos);
			}
			else
			{
				dtVcopy(NeighbourPos, NeighbourNode->pos);
			}

			dtReal CostSoFar, HeuristicCost, DeltaCost;
			if (NeighbourPolyRef != EndRef)
			{
				DeltaCost = QueryFilter->getCost(BestNode->pos, NeighbourPos, ParentPolyRef, ParentMeshTile, ParentPoly,
				                                 BestPolyRef, BestMeshTile, BestPoly, NeighbourPolyRef, NeighbourMeshTile, NeighbourPoly);
				CostSoFar = BestNode->cost + DeltaCost;
				HeuristicCost = dtVdist(NeighbourPos, EndPos) * HeuristicCostScale;
			}
			else
			{
				const dtReal EndCost = QueryFilter->getCost(NeighbourPos, EndPos, BestPolyRef, BestMeshTile, BestPoly,
				                                            NeighbourPolyRef, NeighbourMeshTile, NeighbourPoly, 0, nullptr, nullptr);
				DeltaCost = QueryFilter->getCost(BestNode->pos, NeighbourPos, ParentPolyRef, ParentMeshTile, ParentPoly,
				                                 BestPolyRef, BestMeshTile, BestPoly, NeighbourPolyRef, NeighbourMeshTile, NeighbourPoly);
				CostSoFar = BestNode->cost + DeltaCost + EndCost;
				HeuristicCost = 0.0f;
			}

			const dtReal TotalCost = CostSoFar + HeuristicCost;

			if ((NeighbourNode->flags & DT_NODE_OPEN) && (TotalCost >= NeighbourNode->total))
			{
				continue;
			}
			if ((NeighbourNode->flags & DT_NODE_CLOSED) && (TotalCost >= NeighbourNode->total))
			{
				continue;
			}

			if (DeltaCost == DT_UNWALKABLE_POLY_COST)
			{
				continue;
			}
			if (TotalCost > CostLimit)
			{
				continue;
			}

			NeighbourNode->pidx = NodePool->getNodeIdx(BestNode);
			NeighbourNode->id = NeighbourPolyRef;
			NeighbourNode->flags &= ~DT_NODE_CLOSED;
			NeighbourNode->cost = CostSoFar;
			NeighbourNode->total = TotalCost;
			dtVcopy(NeighbourNode->pos, NeighbourPos);

			if (NeighbourNode->flags & DT_NODE_OPEN)
			{
				OpenNodeList->modify(NeighbourNode);
			}
			else
			{
				NeighbourNode->flags |= DT_NODE_OPEN;
				OpenNodeList->push(NeighbourNode);
				++QueriedNodeNum;
			}

			if (HeuristicCost < LastBestNodeCost)
			{
				LastBestNodeCost = HeuristicCost;
				LastBestNode = NeighbourNode;
			}
		}
	}

	if (LastBestNode->id != EndRef)
	{
		Status |= DT_PARTIAL_RESULT;
	}

	dtNode* PrevNode = nullptr;
	dtNode* CurrNode = LastBestNode;
	int n = 1;
	do
	{
		dtNode* const NextNode = NodePool->getNodeAtIdx(CurrNode->pidx);
		CurrNode->pidx = NodePool->getNodeIdx(PrevNode);
		PrevNode = CurrNode;
		CurrNode = NextNode;
	}
	while (CurrNode && (++n < MaxIterationNum));

	if (n >= MaxIterationNum)
	{
		return DT_FAILURE | DT_INVALID_CYCLE_PATH;
	}

	NavQueryResult.Reserve(n);
	dtReal PrevNodeCost = 0.0f;
	CurrNode = PrevNode;
	ensureAlways(CurrNode);
	do
	{
		NavQueryResult.AddItem(CurrNode->id, CurrNode->cost - PrevNodeCost, nullptr, 0);
		PrevNodeCost = CurrNode->cost;

		CurrNode = NodePool->getNodeAtIdx(CurrNode->pidx);
	}
	while (CurrNode && (--n > 0));
	NavQueryResult.MoveToDetourQueryResult(OutQueryResult);
	return Status;
}

bool FWpNavMeshQuery::PassLinkFilterByRef(const dtMeshTile* const MeshTile, const dtPolyRef PolyRef) const
{
	const int PolyId = NavMesh->decodePolyIdPoly(PolyRef);
	const int LinkId = PolyId - MeshTile->header->offMeshBase;

	return !(LinkFilter
		&& (PolyId >= MeshTile->header->offMeshBase)
		&& (LinkId < MeshTile->header->offMeshConCount)
		&& MeshTile->offMeshCons[LinkId].userId
		&& !LinkFilter->isLinkAllowed(MeshTile->offMeshCons[LinkId].userId));
}

dtStatus FWpNavMeshQuery::GetEdgeMidPoint(const dtPolyRef FromPolyRef, const dtPoly* const FromPoly, const dtMeshTile* const FromMeshTile,
                                          const dtPolyRef ToPolyRef, const dtPoly* const ToPoly, const dtMeshTile* const ToMeshTile,
                                          dtReal* const OutMidPoint) const
{
	dtReal LeftPoint[3], RightPoint[3];
	if (dtStatusFailed(GetPortalPoints(FromPolyRef, FromPoly, FromMeshTile, ToPolyRef, ToPoly, ToMeshTile, LeftPoint, RightPoint)))
	{
		return DT_FAILURE | DT_INVALID_PARAM;
	}

	OutMidPoint[0] = (LeftPoint[0] + RightPoint[0]) * 0.5f;
	OutMidPoint[1] = (LeftPoint[1] + RightPoint[1]) * 0.5f;
	OutMidPoint[2] = (LeftPoint[2] + RightPoint[2]) * 0.5f;
	return DT_SUCCESS;
}

dtStatus FWpNavMeshQuery::GetPortalPoints(const dtPolyRef FromPolyRef, const dtPoly* const FromPoly, const dtMeshTile* const FromMeshTile,
                                          const dtPolyRef ToPolyRef, const dtPoly* const ToPoly, const dtMeshTile* const ToMeshTile,
                                          dtReal* const OutLeftPoint, dtReal* const OutRightPoint) const
{
	const dtLink* link = nullptr;
	unsigned int linkIndex = FromPoly->firstLink;
	while (linkIndex != DT_NULL_LINK)
	{
		const dtLink& testLink = NavMesh->getLink(FromMeshTile, linkIndex);
		linkIndex = testLink.next;

		if (testLink.ref == ToPolyRef)
		{
			link = &testLink;
			break;
		}
	}
	if (!link)
	{
		return DT_FAILURE | DT_INVALID_PARAM;
	}

	if (FromPoly->getType() == DT_POLYTYPE_OFFMESH_POINT)
	{
		unsigned int i = FromPoly->firstLink;
		while (i != DT_NULL_LINK)
		{
			const dtLink& testLink = NavMesh->getLink(FromMeshTile, i);
			i = testLink.next;

			if (testLink.ref == ToPolyRef)
			{
				const int v = testLink.edge;
				dtVcopy(OutLeftPoint, &FromMeshTile->verts[FromPoly->verts[v] * 3]);
				dtVcopy(OutRightPoint, &FromMeshTile->verts[FromPoly->verts[v] * 3]);
				return DT_SUCCESS;
			}
		}

		return DT_FAILURE | DT_INVALID_PARAM;
	}

#if WITH_NAVMESH_SEGMENT_LINKS
	if (FromPoly->getType() == DT_POLYTYPE_OFFMESH_SEGMENT)
	{
		unsigned int i = FromPoly->firstLink;
		while (i != DT_NULL_LINK)
		{
			const dtLink& testLink = NavMesh->getLink(FromMeshTile, i);
			i = testLink.next;

			if (testLink.ref == ToPolyRef)
			{
				const int v = testLink.edge * 2;
				dtVcopy(OutLeftPoint, &FromMeshTile->verts[FromPoly->verts[v + 0] * 3]);
				dtVcopy(OutRightPoint, &FromMeshTile->verts[FromPoly->verts[v + 1] * 3]);

				return DT_SUCCESS;
			}
		}

		return DT_FAILURE | DT_INVALID_PARAM;
	}
#endif // WITH_NAVMESH_SEGMENT_LINKS

	if (ToPoly->getType() == DT_POLYTYPE_OFFMESH_POINT)
	{
		unsigned int i = ToPoly->firstLink;
		while (i != DT_NULL_LINK)
		{
			const dtLink& testLink = NavMesh->getLink(ToMeshTile, i);
			i = testLink.next;

			if (testLink.ref == FromPolyRef)
			{
				const int v = testLink.edge;
				dtVcopy(OutLeftPoint, &ToMeshTile->verts[ToPoly->verts[v] * 3]);
				dtVcopy(OutRightPoint, &ToMeshTile->verts[ToPoly->verts[v] * 3]);

				return DT_SUCCESS;
			}
		}

		return DT_FAILURE | DT_INVALID_PARAM;
	}

#if WITH_NAVMESH_SEGMENT_LINKS
	if (ToPoly->getType() == DT_POLYTYPE_OFFMESH_SEGMENT)
	{
		unsigned int i = ToPoly->firstLink;
		while (i != DT_NULL_LINK)
		{
			const dtLink& testLink = NavMesh->getLink(ToMeshTile, i);
			i = testLink.next;

			if (testLink.ref == FromPolyRef)
			{
				const int v = testLink.edge * 2;
				dtVcopy(OutLeftPoint, &ToMeshTile->verts[ToPoly->verts[v + 0] * 3]);
				dtVcopy(OutRightPoint, &ToMeshTile->verts[ToPoly->verts[v + 1] * 3]);

				return DT_SUCCESS;
			}
		}

		return DT_FAILURE | DT_INVALID_PARAM;
	}
#endif // WITH_NAVMESH_SEGMENT_LINKS

	const int v0 = FromPoly->verts[link->edge];
	const int v1 = FromPoly->verts[(link->edge + 1) % static_cast<int>(FromPoly->vertCount)];
	dtVcopy(OutLeftPoint, &FromMeshTile->verts[v0 * 3]);
	dtVcopy(OutRightPoint, &FromMeshTile->verts[v1 * 3]);

	if (!(link->side & DT_CONNECTION_INTERNAL))
	{
		if ((link->bmin != 0) || (link->bmax != 255))
		{
			constexpr dtReal s = static_cast<dtReal>(1.0f) / 255.0f;
			const dtReal tmin = link->bmin * s;
			const dtReal tmax = link->bmax * s;
			dtVlerp(OutLeftPoint, &FromMeshTile->verts[v0 * 3], &FromMeshTile->verts[v1 * 3], tmin);
			dtVlerp(OutRightPoint, &FromMeshTile->verts[v0 * 3], &FromMeshTile->verts[v1 * 3], tmax);
		}
	}

	return DT_SUCCESS;
}

void FWpNavMeshQuery::MoveToDetourNavMeshQuery(dtNavMeshQuery& DetourNavMeshQuery)
{
	static_assert(sizeof(FWpNavMeshQuery) == sizeof(dtNavMeshQuery));
	memcpy(&DetourNavMeshQuery, this, sizeof(dtNavMeshQuery));
	memset(this, 0, sizeof(FWpNavMeshQuery));
}

void FWpNavQueryResult::Reserve(const int Size)
{
	Data.resize(Size);
	Data.resize(0);
}

void FWpNavQueryResult::AddItem(const dtPolyRef Ref, const dtReal Cost, const dtReal* const Pos, const unsigned int Flag)
{
	Data.push(dtQueryResultPack{Ref, Cost, Pos, Flag});
}

void FWpNavQueryResult::MoveToDetourQueryResult(dtQueryResult& DetourQueryResult)
{
	static_assert(sizeof(FWpNavQueryResult) == sizeof(dtQueryResult));
	memcpy(&DetourQueryResult, this, sizeof(dtQueryResult));
	memset(this, 0, sizeof(FWpNavQueryResult));
}

void dtNodePool::clear()
{
	memset(m_first, 0xff, sizeof(dtNodeIndex) * m_hashSize);
	m_nodeCount = 0;
}

inline unsigned int dtHashRef(dtPolyRef a)
{
	a += ~(a << 31);
	a ^= (a >> 20);
	a += (a << 6);
	a ^= (a >> 12);
	a += ~(a << 22);
	a ^= (a >> 32);
	return static_cast<unsigned int>(a);
}

dtNode* dtNodePool::getNode(const dtPolyRef id)
{
	const unsigned int bucket = dtHashRef(id) & (m_hashSize - 1);
	dtNodeIndex i = m_first[bucket];
	while (i != DT_NULL_IDX)
	{
		if (m_nodes[i].id == id)
		{
			return &m_nodes[i];
		}

		i = m_next[i];
	}

	if (m_nodeCount >= getMaxRuntimeNodes())
	{
		return nullptr;
	}

	i = static_cast<dtNodeIndex>(m_nodeCount);
	m_nodeCount++;

	dtNode* const node = &m_nodes[i];
	node->pidx = 0;
	node->cost = 0;
	node->total = 0;
	node->id = id;
	node->flags = 0;

	m_next[i] = m_first[bucket];
	m_first[bucket] = i;

	return node;
}

void dtNodeQueue::bubbleUp(int i, dtNode* const node)
{
	int parent = (i - 1) / 2;
	while ((i > 0) && (m_heap[parent]->total > node->total))
	{
		m_heap[i] = m_heap[parent];
		i = parent;
		parent = (i - 1) / 2;
	}
	m_heap[i] = node;
}

void dtNodeQueue::trickleDown(int i, dtNode* const node)
{
	int child = (i * 2) + 1;
	while (child < m_size)
	{
		if (((child + 1) < m_size) && (m_heap[child]->total > m_heap[child + 1]->total))
		{
			child++;
		}
		m_heap[i] = m_heap[child];
		i = child;
		child = (i * 2) + 1;
	}
	bubbleUp(i, node);
}

dtQueryResultPack::dtQueryResultPack(const dtPolyRef inRef, const dtReal inCost, const dtReal* const inPos, const unsigned int inFlag)
	: ref(inRef), cost(inCost), flag(inFlag)
{
	if (inPos)
	{
		dtVcopy(pos, inPos);
	}
}
