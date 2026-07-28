// Copyright (c) Red Alert 4 project.
#include "RA4Navigation/NavDebug.h"

namespace RA4
{
namespace Nav
{

void SerializeNavDebugSnapshot(const NavDebugSnapshot& Snap, std::vector<uint8_t>& OutBytes)
{
    OutBytes.clear();
    auto Append = [&](uint32_t V) {
        OutBytes.push_back(uint8_t(V & 0xFF));
        OutBytes.push_back(uint8_t((V >> 8) & 0xFF));
        OutBytes.push_back(uint8_t((V >> 16) & 0xFF));
        OutBytes.push_back(uint8_t((V >> 24) & 0xFF));
    };
    Append(Snap.TopologyRevision);
    Append(uint32_t(Snap.ReservationSample.size()));
    Append(uint32_t(Snap.ActiveMacroPaths.size()));
}

void ReservationGrid::Snapshot(NavDebugSnapshot& Out) const
{
    Out.ReservationSample.clear();
    for (const ReservationCell& C : Cells)
    {
        if (C.OccupantSlot != kInvalidReservationSlot)
        {
            Out.ReservationSample.push_back(C);
        }
    }
}

void MNavRouter::Snapshot(NavDebugSnapshot& Out) const
{
    Out.ActiveMacroPaths.clear();
    for (const CacheEntry& E : Cache)
    {
        MacroPath P;
        P.Waypoints = E.Waypoints;
        P.BuiltTopologyRevision = E.TopologyRevision;
        P.Query = E.Query;
        Out.ActiveMacroPaths.push_back(std::move(P));
        if (Out.ActiveMacroPaths.size() >= 16)
        {
            break;
        }
    }
}

void NavGrid::Snapshot(NavDebugSnapshot& Out) const
{
    Out.TopologyRevision = TopologyRevision;
    Out.BlockedTiles.clear();
    for (int32_t Y = 0; Y < Height; ++Y)
    {
        for (int32_t X = 0; X < Width; ++X)
        {
            const NavCell& C = Cells[size_t(Y * Width + X)];
            if (C.PassabilityMask == NavLayer_None)
            {
                Out.BlockedTiles.push_back(TileCoord(X, Y));
            }
        }
    }
}

} // namespace Nav
} // namespace RA4
