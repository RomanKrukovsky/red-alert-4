// Copyright (c) Red Alert 4 project. Fog-of-war visibility as texture data.
//
// ADR-0030: fog is drawn in the 3D world by sampling a texture built from the
// simulation's visibility grid. This header is the whole encoding rule, kept
// free of Unreal types so the mapping and the upload buffer can be unit-tested
// headlessly -- the part that gets the encoding wrong is the part that silently
// renders explored ground as unexplored, and that must fail a test, not a
// playtest.
#pragma once

#include <cstdint>
#include <vector>

#include "FogOfWarGrid.h"

namespace RA4
{

// One byte per tile. The four states are evenly spaced across the range rather
// than packed at 0..3 for two reasons: the material samples this with bilinear
// filtering and interprets it as a 0..1 ramp (even spacing makes the midpoints
// meaningful), and a fifth state can be added later without changing the format
// or the material's threshold arithmetic.
constexpr uint8_t kFogTexelNeverSeen = 0;
constexpr uint8_t kFogTexelPreviouslySeen = 85;
constexpr uint8_t kFogTexelRadarDetected = 170;
constexpr uint8_t kFogTexelCurrentlyVisible = 255;

// Radar sits between "remembered" and "seen" deliberately: the player is told
// something is out there, so the ground is less dim than pure memory, but per
// the V-A MAJOR-2 decision a radar contact never renders a unit -- that gate
// lives in the entity visibility check, not here.
inline uint8_t FogStateToTexel(VisibilityState State)
{
    switch (State)
    {
    case VisibilityState::NeverSeen:
        return kFogTexelNeverSeen;
    case VisibilityState::PreviouslySeen:
        return kFogTexelPreviouslySeen;
    case VisibilityState::RadarDetected:
        return kFogTexelRadarDetected;
    case VisibilityState::CurrentlyVisible:
        return kFogTexelCurrentlyVisible;
    }
    // An unmapped state must read as unexplored rather than as visible: the safe
    // failure for a fog bug is showing the player too little, never too much.
    return kFogTexelNeverSeen;
}

// Fills Out with Width*Height texels in row-major order for one player.
// Returns false and leaves Out untouched when the player index is out of range,
// so a caller with a bad seat renders nothing new rather than another player's
// vision.
inline bool BuildFogTexelBuffer(const FFogOfWarGrid& Grid, int32_t PlayerIndex,
                                std::vector<uint8_t>& Out)
{
    if (PlayerIndex < 0 || PlayerIndex >= Grid.GetNumPlayers())
    {
        return false;
    }
    const int32_t W = Grid.GetWidth();
    const int32_t H = Grid.GetHeight();
    Out.resize(size_t(W) * size_t(H));
    for (int32_t Y = 0; Y < H; ++Y)
    {
        for (int32_t X = 0; X < W; ++X)
        {
            Out[size_t(Y) * size_t(W) + size_t(X)] =
                FogStateToTexel(Grid.GetVisibility(PlayerIndex, X, Y));
        }
    }
    return true;
}

// Copies one rectangle of the grid into an existing full-size buffer. The dirty
// path and the full path must agree exactly -- a partial upload that drifts from
// the full rebuild shows stale fog, which looks like a vision bug and is
// untraceable in a playtest. Rect coordinates are inclusive of Min, exclusive of
// Max, and are clamped to the grid.
inline bool BlitFogTexelRegion(const FFogOfWarGrid& Grid, int32_t PlayerIndex,
                               int32_t MinX, int32_t MinY, int32_t MaxX, int32_t MaxY,
                               std::vector<uint8_t>& InOut)
{
    if (PlayerIndex < 0 || PlayerIndex >= Grid.GetNumPlayers())
    {
        return false;
    }
    const int32_t W = Grid.GetWidth();
    const int32_t H = Grid.GetHeight();
    if (InOut.size() != size_t(W) * size_t(H))
    {
        return false;   // caller must size the buffer with BuildFogTexelBuffer first
    }
    const int32_t X0 = MinX < 0 ? 0 : MinX;
    const int32_t Y0 = MinY < 0 ? 0 : MinY;
    const int32_t X1 = MaxX > W ? W : MaxX;
    const int32_t Y1 = MaxY > H ? H : MaxY;
    for (int32_t Y = Y0; Y < Y1; ++Y)
    {
        for (int32_t X = X0; X < X1; ++X)
        {
            InOut[size_t(Y) * size_t(W) + size_t(X)] =
                FogStateToTexel(Grid.GetVisibility(PlayerIndex, X, Y));
        }
    }
    return true;
}

} // namespace RA4
