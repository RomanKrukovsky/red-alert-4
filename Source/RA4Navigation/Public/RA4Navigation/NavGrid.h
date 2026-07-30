// Copyright (c) Red Alert 4 project. Deterministic grid navigation topology.
#pragma once

#include <cstdint>
#include <vector>

#include "RA4Core/Vector.h"

#if __has_include("HAL/Platform.h")
#include "HAL/Platform.h"
#endif

#ifndef RA4NAVIGATION_API
#define RA4NAVIGATION_API
#endif

namespace RA4
{
namespace Nav
{

struct NavDebugSnapshot;   // defined in RA4Navigation/NavDebug.h

enum NavLayerMask : uint8_t
{
    NavLayer_None = 0,
    NavLayer_Infantry = 1 << 0,
    NavLayer_Wheeled = 1 << 1,
    NavLayer_Tracked = 1 << 2,
    NavLayer_Amphibious = 1 << 3,
    NavLayer_Naval = 1 << 4,
    NavLayer_Air = 1 << 5,
    NavLayer_All = NavLayer_Infantry | NavLayer_Wheeled | NavLayer_Tracked |
                   NavLayer_Amphibious | NavLayer_Naval | NavLayer_Air,
};

struct NavQuery
{
    uint8_t LayerMask = NavLayer_Infantry;
    uint8_t RequiredClearance = 1;
};

struct NavCell
{
    uint8_t PassabilityMask = NavLayer_All;
    uint8_t MovementCost = 1;
    uint8_t Clearance = 0;
};

struct NavSector
{
    uint16_t Id = 0;
    TileCoord Min;
    TileCoord Max;
};

struct NavPortal
{
    uint16_t Id = 0;
    uint16_t SectorA = 0;
    uint16_t SectorB = 0;
    TileCoord StartA;
    TileCoord EndA;
    TileCoord StartB;
    TileCoord EndB;
    uint8_t PassabilityMask = NavLayer_None;
};

class RA4NAVIGATION_API NavGrid
{
public:
    static constexpr int32_t kSectorSize = 16;

    NavGrid(int32_t InWidth, int32_t InHeight);

    int32_t GetWidth() const { return Width; }
    int32_t GetHeight() const { return Height; }
    uint32_t GetTopologyRevision() const { return TopologyRevision; }

    bool IsInBounds(const TileCoord& Tile) const;
    const NavCell& GetCell(const TileCoord& Tile) const;
    bool IsTraversable(const TileCoord& Tile, const NavQuery& Query) const;

    bool SetPassability(const TileCoord& Tile, uint8_t PassabilityMask);
    bool SetMovementCost(const TileCoord& Tile, uint8_t MovementCost);

    // Batch obstacle changes from a building placement or a destroyed bridge. The
    // expensive clearance and portal rebuild occurs once in EndTopologyUpdate.
    void BeginTopologyUpdate();
    bool EndTopologyUpdate();

    const std::vector<NavSector>& GetSectors() const { return Sectors; }
    const std::vector<NavPortal>& GetPortals() const { return Portals; }

    // Pure-data snapshot for the presentation bridge and the headless tests.
    // Captures the topology revision and the list of fully blocked tiles.
    void Snapshot(NavDebugSnapshot& Out) const;

private:
    int32_t ToIndex(const TileCoord& Tile) const;
    void MarkTopologyDirty();
    void RebuildDerivedData();
    void RebuildClearance();
    void RebuildSectorsAndPortals();
    void AppendBoundaryPortals(uint16_t SectorA, uint16_t SectorB, const TileCoord& StartA,
                               const TileCoord& Step, const TileCoord& OffsetToB, int32_t Length);

    int32_t Width = 0;
    int32_t Height = 0;
    int32_t UpdateDepth = 0;
    uint32_t TopologyRevision = 0;
    bool bTopologyDirty = false;
    std::vector<NavCell> Cells;
    std::vector<NavSector> Sectors;
    std::vector<NavPortal> Portals;
};

} // namespace Nav
} // namespace RA4
