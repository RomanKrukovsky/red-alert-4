// Copyright (c) Red Alert 4 project.

#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

#if defined(RA4_HEADLESS)
struct FIntRect
{
    int32_t MinX = 0;
    int32_t MinY = 0;
    int32_t MaxX = 0;
    int32_t MaxY = 0;
    FIntRect() = default;
    FIntRect(int32_t InMinX, int32_t InMinY, int32_t InMaxX, int32_t InMaxY)
        : MinX(InMinX), MinY(InMinY), MaxX(InMaxX), MaxY(InMaxY) {}
};
#else
#include "CoreMinimal.h"
#endif

#ifndef RA4FOGOFWAR_API
#define RA4FOGOFWAR_API
#endif

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

    int32_t GetWidth() const { return Width; }
    int32_t GetHeight() const { return Height; }
    int32_t GetNumPlayers() const { return NumPlayers; }

    // Updates visibility around a center point for a specific player
    void RevealCircularArea(int32_t PlayerIndex, int32_t CenterX, int32_t CenterY, int32_t Radius);

    // Radar coverage: marks cells as RadarDetected, which reads as "something is there"
    // without granting the detail that eyes-on vision does. Deliberately never downgrades
    // a cell -- a tile that is CurrentlyVisible stays so, because real vision is strictly
    // better information than a blip, and order of calls within a tick must not decide
    // which one wins.
    //
    // VisibilityState::RadarDetected existed and was tested for in two places, but
    // nothing ever set it, so radar contributed nothing to the minimap or the AI view.
    void RevealRadarArea(int32_t PlayerIndex, int32_t CenterX, int32_t CenterY, int32_t Radius);

    // Resets current visibility (called at the start of a tick before applying unit visions)
    void ClearCurrentVisibility(int32_t PlayerIndex);

    // Get visibility state for a specific cell and player
    VisibilityState GetVisibility(int32_t PlayerIndex, int32_t X, int32_t Y) const;

    // Retrieve dirty regions to update textures
    const std::vector<FIntRect>& GetDirtyRegions(int32_t PlayerIndex) const;
    void ClearDirtyRegions(int32_t PlayerIndex);

private:
    int32_t Width = 0;
    int32_t Height = 0;
    int32_t NumPlayers = 0;

    // We store visibility for all players
    std::vector<std::vector<VisibilityState>> VisibilityData;

    // Track dirty regions for texture updates
    std::vector<std::vector<FIntRect>> DirtyRegions;
};

} // namespace RA4
