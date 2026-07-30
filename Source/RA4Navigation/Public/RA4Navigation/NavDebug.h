// Copyright (c) Red Alert 4 project. Pure-data navigation debug snapshot.
//
// This is the ONLY debug surface the simulation exposes. The presentation bridge
// (a later roadmap stage) renders it; the headless tests serialize it. There is no
// DrawDebug* anywhere in RA4Navigation or RA4Simulation, because that would make the
// headless build link-fail and the deterministic core depend on the renderer.
#pragma once

#include <cstdint>
#include <vector>

#include "RA4Navigation/FlowField.h"
#include "RA4Navigation/MNavRouter.h"
#include "RA4Navigation/NavGrid.h"
#include "RA4Navigation/ReservationGrid.h"

namespace RA4
{
namespace Nav
{

struct NavDebugSnapshot
{
    uint32_t TopologyRevision = 0;
    std::vector<FlowDirection> FlowFieldSample;     // sparse: only dirty tiles
    std::vector<ReservationCell> ReservationSample; // copied cells (occupied only)
    std::vector<TileCoord> BlockedTiles;
    std::vector<MacroPath> ActiveMacroPaths;        // capped at 16 for debug
};

RA4NAVIGATION_API void SerializeNavDebugSnapshot(const NavDebugSnapshot& Snap, std::vector<uint8_t>& OutBytes);

} // namespace Nav
} // namespace RA4
