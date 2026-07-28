// Copyright (c) Red Alert 4 project.

#pragma once

#include <vector>
#include <cstdint>
#include "NavGrid.h"

namespace RA4
{
namespace Nav
{

// A pre-calculated vector field guiding agents to a specific destination
class RA4NAVIGATION_API FFlowField
{
public:
    FFlowField(const FNavGrid* InGrid, const FGridCoord& TargetLocation);

    // Calculate the flow field vectors for the given region
    void CalculateField();

    // Returns a 2D direction vector [X, Y] normalized or [0,0] if impassable
    void GetDirection(int32_t X, int32_t Y, float& OutDirX, float& OutDirY) const;

private:
    const FNavGrid* Grid;
    FGridCoord Target;

    struct FFlowCell
    {
        uint16_t IntegrationCost = 0xFFFF; // Cost to reach target
        float DirX = 0.0f;
        float DirY = 0.0f;
    };

    std::vector<FFlowCell> Field;
};

} // namespace Nav
} // namespace RA4
