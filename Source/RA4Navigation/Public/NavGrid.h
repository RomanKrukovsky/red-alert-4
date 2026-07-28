// Copyright (c) Red Alert 4 project.

#pragma once

#include <cstdint>
#include <vector>
#include "RA4Core/Fixed4816.h" // Assuming Fixed4816.h exists in RA4Core

namespace RA4
{
namespace Nav
{

// Each grid cell has a clearance value and movement costs for different unit types
struct FNavCell
{
    uint8_t Clearance = 0; // Distance to nearest obstacle
    uint8_t CostMultiplier = 1; // Base cost (1 = normal, 255 = impassable)
    uint8_t LayerMask = 0xFF; // Bitmask of which layers (Ground, Amphibious, Hover, etc.) can enter
    uint16_t SectorId = 0xFFFF; // ID of the sector this cell belongs to
};

// Represents a 2D coordinate on the navigation grid
struct FGridCoord
{
    int32_t X = 0;
    int32_t Y = 0;

    constexpr bool operator==(const FGridCoord& Other) const { return X == Other.X && Y == Other.Y; }
    constexpr bool operator!=(const FGridCoord& Other) const { return !(*this == Other); }
};

// A contiguous block of cells, used for hierarchical pathfinding (HPA*)
struct FSector
{
    uint16_t Id;
    FGridCoord MinBounds;
    FGridCoord MaxBounds;
    std::vector<uint16_t> ConnectedPortals;
};

// A transition between two adjacent sectors
struct FPortal
{
    uint16_t Id;
    uint16_t SectorA;
    uint16_t SectorB;
    
    // The cells that make up this portal boundary
    std::vector<FGridCoord> Cells;
    FGridCoord Center;
};

class RA4NAVIGATION_API FNavGrid
{
public:
    FNavGrid(int32_t Width, int32_t Height);

    void Initialize(const std::vector<uint8_t>& InImpassableMask);

    const FNavCell& GetCell(int32_t X, int32_t Y) const;
    FNavCell& GetCellMutable(int32_t X, int32_t Y);
    bool IsValid(int32_t X, int32_t Y) const;

    int32_t GetWidth() const { return Width; }
    int32_t GetHeight() const { return Height; }

private:
    void BuildSectors();
    void BuildPortals();

    int32_t Width;
    int32_t Height;
    std::vector<FNavCell> Cells;
    std::vector<FSector> Sectors;
    std::vector<FPortal> Portals;

    static constexpr int32_t kSectorSize = 16; // 16x16 cells per sector
};

} // namespace Nav
} // namespace RA4
