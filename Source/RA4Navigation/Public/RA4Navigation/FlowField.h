// Copyright (c) Red Alert 4 project. Deterministic shared flow fields.
#pragma once

#include <cstdint>
#include <vector>

#include "RA4Navigation/NavGrid.h"

namespace RA4
{
namespace Nav
{

struct FlowDirection
{
    int8_t X = 0;
    int8_t Y = 0;

    constexpr bool operator==(const FlowDirection& Other) const
    {
        return X == Other.X && Y == Other.Y;
    }
};

class RA4NAVIGATION_API FlowField
{
public:
    static constexpr uint32_t kUnreachableCost = 0xFFFFFFFFu;

    FlowField(const NavGrid& InGrid, const NavQuery& InQuery, const TileCoord& InTarget);

    void Rebuild();

    bool IsReachable(const TileCoord& Tile) const;
    uint32_t GetIntegrationCost(const TileCoord& Tile) const;
    FlowDirection GetDirection(const TileCoord& Tile) const;
    uint32_t GetBuiltTopologyRevision() const { return BuiltTopologyRevision; }

private:
    bool CanStep(const TileCoord& From, const TileCoord& To) const;
    int32_t ToIndex(const TileCoord& Tile) const;

    const NavGrid& Grid;
    NavQuery Query;
    TileCoord Target;
    uint32_t BuiltTopologyRevision = 0;
    std::vector<uint32_t> IntegrationCosts;
    std::vector<FlowDirection> Directions;
};

} // namespace Nav
} // namespace RA4
