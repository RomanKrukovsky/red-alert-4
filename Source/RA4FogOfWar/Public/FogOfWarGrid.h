// Copyright (c) Red Alert 4 project.

#pragma once

#include <cstdint>
#include <vector>
#include "CoreMinimal.h"

namespace RA4
{

enum class VisibilityState : uint8_t
{
    NeverSeen = 0,
    PreviouslySeen = 1,
    CurrentlyVisible = 2,
    RadarDetected = 3
};

class RA4FOGOFWAR_API FFogOfWarGrid
{
public:
    FFogOfWarGrid(int32_t InWidth, int32_t InHeight, int32_t InNumPlayers);

    // Updates visibility around a center point for a specific player
    void RevealCircularArea(int32_t PlayerIndex, int32_t CenterX, int32_t CenterY, int32_t Radius);

    // Resets current visibility (usually called at the start of a tick before applying all unit visions)
    void ClearCurrentVisibility(int32_t PlayerIndex);

    // Get visibility state for a specific cell and player
    VisibilityState GetVisibility(int32_t PlayerIndex, int32_t X, int32_t Y) const;

    // Retrieve dirty regions to update textures
    const std::vector<FIntRect>& GetDirtyRegions(int32_t PlayerIndex) const;
    void ClearDirtyRegions(int32_t PlayerIndex);

private:
    int32_t Width;
    int32_t Height;
    int32_t NumPlayers;

    // We store visibility for all players
    std::vector<std::vector<VisibilityState>> VisibilityData;

    // Track dirty regions for texture updates
    std::vector<std::vector<FIntRect>> DirtyRegions;
};

} // namespace RA4
