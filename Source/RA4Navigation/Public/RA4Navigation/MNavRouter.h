// Copyright (c) Red Alert 4 project. Hierarchical macro router over sector portals.
//
// A* over the sector-portal graph produces a coarse corridor of sector-center
// sub-goals. Units then follow the shared flow field to each sub-goal, so 300
// units heading to one destination build one macro path and one flow field per
// sector, not 300 of each. Expansion order is fixed (ascending sector id) and the
// tie-break is (g+h, sector id), so the path is identical on every machine.
#pragma once

#include <cstdint>
#include <vector>

#include "RA4Navigation/NavGrid.h"

#ifndef RA4NAVIGATION_API
#define RA4NAVIGATION_API
#endif

namespace RA4
{
namespace Nav
{

struct MacroPath
{
    std::vector<TileCoord> Waypoints;     // sector-center sub-goals; final = destination tile
    uint32_t BuiltTopologyRevision = 0;   // stale paths are rejected, not rebuilt blindly
    NavQuery Query{};
};

class RA4NAVIGATION_API MNavRouter
{
public:
    explicit MNavRouter(const NavGrid& InGrid);

    // A* over sector portals from From -> To. Emits at most MaxWaypoints sub-goals.
    // Returns an empty MacroPath if no portal path exists. Deterministic.
    MacroPath Find(const TileCoord& From, const TileCoord& To,
                   const NavQuery& Query, int32_t MaxWaypoints);

    // Drop every cached path. Called when the grid's topology revision bumps.
    void InvalidateAll();

    // Cache diagnostics for the determinism/perf tests.
    uint32_t GetCacheHits() const { return CacheHits; }
    uint32_t GetCacheMisses() const { return CacheMisses; }
    void ResetCounters() { CacheHits = 0; CacheMisses = 0; }

private:
    struct CacheEntry
    {
        uint16_t FromSector = 0;
        uint16_t ToSector = 0;
        NavQuery Query{};
        uint32_t TopologyRevision = 0;
        std::vector<TileCoord> Waypoints;
        int32_t LastUsedSerial = 0;   // for LRU; bumped on each Find
    };

    TileCoord SectorCenter(uint16_t SectorId) const;
    const std::vector<CacheEntry>& GetCacheForTest() const { return Cache; }

    const NavGrid& Grid;
    std::vector<CacheEntry> Cache;
    int32_t AccessSerial = 0;
    uint32_t CacheHits = 0;
    uint32_t CacheMisses = 0;
    static constexpr size_t kCacheCap = 128;
};

} // namespace Nav
} // namespace RA4