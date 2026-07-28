// Copyright (c) Red Alert 4 project.
#include "RA4Navigation/MNavRouter.h"

#include <algorithm>
#include <queue>

namespace RA4
{
namespace Nav
{

namespace
{
struct AStarNode
{
    uint16_t SectorId = 0;
    int32_t G = 0;        // cost from start in sector hops
    int32_t H = 0;        // heuristic: sector-grid octile distance
    int32_t F = 0;        // G + H
    uint16_t CameFrom = 0;
};
} // namespace

MNavRouter::MNavRouter(const NavGrid& InGrid)
    : Grid(InGrid)
{
}

TileCoord MNavRouter::SectorCenter(uint16_t SectorId) const
{
    const std::vector<NavSector>& Sectors = Grid.GetSectors();
    if (static_cast<size_t>(SectorId) >= Sectors.size())
    {
        return TileCoord();
    }
    const NavSector& S = Sectors[SectorId];
    return TileCoord((S.Min.X + S.Max.X) / 2, (S.Min.Y + S.Max.Y) / 2);
}

MacroPath MNavRouter::Find(const TileCoord& From, const TileCoord& To,
                           const NavQuery& Query, int32_t MaxWaypoints)
{
    MacroPath Out;
    Out.Query = Query;
    Out.BuiltTopologyRevision = Grid.GetTopologyRevision();

    if (!Grid.IsInBounds(From) || !Grid.IsInBounds(To))
    {
        return Out;
    }

    // Sector id from a tile: floor(tile / kSectorSize) plus row-major index.
    const auto SectorOf = [&](const TileCoord& T) -> uint16_t {
        const int32_t Sx = T.X / NavGrid::kSectorSize;
        const int32_t Sy = T.Y / NavGrid::kSectorSize;
        const int32_t W = (Grid.GetWidth() + NavGrid::kSectorSize - 1) / NavGrid::kSectorSize;
        return uint16_t(Sy * W + Sx);
    };

    const uint16_t StartSector = SectorOf(From);
    const uint16_t GoalSector = SectorOf(To);

    // Cache lookup. Hit = same from/to sectors, same query, same topology revision.
    for (CacheEntry& E : Cache)
    {
        if (E.FromSector == StartSector && E.ToSector == GoalSector &&
            E.Query.LayerMask == Query.LayerMask && E.Query.RequiredClearance == Query.RequiredClearance &&
            E.TopologyRevision == Out.BuiltTopologyRevision)
        {
            ++CacheHits;
            E.LastUsedSerial = ++AccessSerial;
            Out.Waypoints = E.Waypoints;
            // LRU: move to back so the front is always the coldest entry.
            // Deviation from brief: std::find requires CacheEntry::operator==, which
            // the brief never defined. Recover the iterator from the reference's
            // address instead -- identity comparison, no new operator==.
            const size_t HitIndex = static_cast<size_t>(&E - Cache.data());
            std::rotate(Cache.begin(), Cache.begin() + ptrdiff_t(HitIndex), Cache.begin() + 1);
            return Out;
        }
    }
    ++CacheMisses;

    // Same-sector shortcut: the flow field alone handles it.
    if (StartSector == GoalSector)
    {
        Out.Waypoints.push_back(To);
        return Out;
    }

    // A* over portals. Neighbor expansion order is ascending destination sector id;
    // the priority queue tie-breaks on (F, SectorId) so it is fully deterministic.
    const std::vector<NavPortal>& Portals = Grid.GetPortals();
    const std::vector<NavSector>& Sectors = Grid.GetSectors();
    const size_t SectorCount = Sectors.size();

    std::vector<AStarNode> Nodes(SectorCount);
    std::vector<bool> Closed(SectorCount, false);
    for (size_t I = 0; I < SectorCount; ++I)
    {
        const uint16_t Idx = uint16_t(I);
        Nodes[I].SectorId = Idx;
        Nodes[I].CameFrom = Idx;
    }

    auto Heuristic = [&](uint16_t A, uint16_t B) -> int32_t {
        const NavSector& SA = Sectors[A];
        const NavSector& SB = Sectors[B];
        const int32_t Dx = (SA.Min.X + SA.Max.X) / 2 - (SB.Min.X + SB.Max.X) / 2;
        const int32_t Dy = (SA.Min.Y + SA.Max.Y) / 2 - (SB.Min.Y + SB.Max.Y) / 2;
        const int32_t Adx = Dx < 0 ? -Dx : Dx;
        const int32_t Ady = Dy < 0 ? -Dy : Dy;
        return 10 * (Adx < Ady ? Adx : Ady) + 14 * (Adx > Ady ? Adx - Ady : Ady - Adx);
    };

    auto Greater = [](const AStarNode& A, const AStarNode& B) {
        return A.F != B.F ? A.F > B.F : A.SectorId > B.SectorId;
    };
    std::priority_queue<AStarNode, std::vector<AStarNode>, decltype(Greater)> Open(Greater);

    Nodes[StartSector].G = 0;
    Nodes[StartSector].H = Heuristic(StartSector, GoalSector);
    Nodes[StartSector].F = Nodes[StartSector].H;
    Open.push(Nodes[StartSector]);

    bool bFound = false;
    while (!Open.empty())
    {
        AStarNode Cur = Open.top();
        Open.pop();
        if (Closed[Cur.SectorId])
        {
            continue;
        }
        Closed[Cur.SectorId] = true;
        if (Cur.SectorId == GoalSector)
        {
            bFound = true;
            break;
        }
        // Expand neighbors in ascending destination-sector-id order for determinism.
        // Portals are pre-sorted by SectorA then SectorB at grid build time; filter.
        for (const NavPortal& P : Portals)
        {
            uint16_t Neighbor = 0;
            if (P.SectorA == Cur.SectorId) { Neighbor = P.SectorB; }
            else if (P.SectorB == Cur.SectorId) { Neighbor = P.SectorA; }
            else { continue; }
            if (Closed[Neighbor])
            {
                continue;
            }
            // Layer/clearance check against the portal's passability mask.
            if ((P.PassabilityMask & Query.LayerMask) == 0)
            {
                continue;
            }
            // Note: per-portal clearance is not stored today; the flow field handles
            // cell-level clearance. The router only guarantees sector reachability.
            const int32_t TentativeG = Cur.G + 1;
            if (Nodes[Neighbor].G == 0 && Neighbor != StartSector)
            {
                Nodes[Neighbor].G = TentativeG;
                Nodes[Neighbor].H = Heuristic(Neighbor, GoalSector);
                Nodes[Neighbor].F = TentativeG + Nodes[Neighbor].H;
                Nodes[Neighbor].CameFrom = Cur.SectorId;
                Open.push(Nodes[Neighbor]);
            }
            else if (TentativeG < Nodes[Neighbor].G)
            {
                Nodes[Neighbor].G = TentativeG;
                Nodes[Neighbor].F = TentativeG + Nodes[Neighbor].H;
                Nodes[Neighbor].CameFrom = Cur.SectorId;
                Open.push(Nodes[Neighbor]);
            }
        }
    }

    if (!bFound)
    {
        // Unreachable. Cache the empty path so we don't A* again next tick.
        if (Cache.size() >= kCacheCap)
        {
            Cache.erase(Cache.begin());
        }
        Cache.push_back({StartSector, GoalSector, Query, Out.BuiltTopologyRevision, {}, ++AccessSerial});
        return Out;
    }

    // Reconstruct sector chain, then emit sector centers as sub-goals, final = To.
    std::vector<uint16_t> Chain;
    uint16_t Cur = GoalSector;
    while (Cur != StartSector && Nodes[Cur].CameFrom != Cur)
    {
        Chain.push_back(Cur);
        Cur = Nodes[Cur].CameFrom;
    }
    std::reverse(Chain.begin(), Chain.end());

    Out.Waypoints.reserve(Chain.size() + 1);
    for (uint16_t S : Chain)
    {
        Out.Waypoints.push_back(SectorCenter(S));
        if (static_cast<int32_t>(Out.Waypoints.size()) >= MaxWaypoints - 1)
        {
            break;
        }
    }
    Out.Waypoints.push_back(To);

    if (Cache.size() >= kCacheCap)
    {
        Cache.erase(Cache.begin());
    }
    Cache.push_back({StartSector, GoalSector, Query, Out.BuiltTopologyRevision, Out.Waypoints, ++AccessSerial});
    return Out;
}

void MNavRouter::InvalidateAll()
{
    Cache.clear();
}

} // namespace Nav
} // namespace RA4