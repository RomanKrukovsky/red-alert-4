// Copyright (c) Red Alert 4 project.

#include "FogOfWarGrid.h"
#include <algorithm>

namespace RA4
{

FFogOfWarGrid::FFogOfWarGrid(int32_t InWidth, int32_t InHeight, int32_t InNumPlayers)
    : Width(InWidth)
    , Height(InHeight)
    , NumPlayers(InNumPlayers)
{
    VisibilityData.resize(NumPlayers);
    DirtyRegions.resize(NumPlayers);

    for (int32_t i = 0; i < NumPlayers; ++i)
    {
        VisibilityData[i].resize(Width * Height, VisibilityState::NeverSeen);
    }
}

void FFogOfWarGrid::RevealCircularArea(int32_t PlayerIndex, int32_t CenterX, int32_t CenterY, int32_t Radius)
{
    if (PlayerIndex < 0 || PlayerIndex >= NumPlayers) return;

    int32_t MinX = std::max(0, CenterX - Radius);
    int32_t MaxX = std::min(Width - 1, CenterX + Radius);
    int32_t MinY = std::max(0, CenterY - Radius);
    int32_t MaxY = std::min(Height - 1, CenterY + Radius);

    int32_t RadiusSq = Radius * Radius;
    bool bChanged = false;

    auto& Data = VisibilityData[PlayerIndex];

    for (int32_t Y = MinY; Y <= MaxY; ++Y)
    {
        for (int32_t X = MinX; X <= MaxX; ++X)
        {
            int32_t DistSq = (X - CenterX) * (X - CenterX) + (Y - CenterY) * (Y - CenterY);
            if (DistSq <= RadiusSq)
            {
                int32_t Idx = Y * Width + X;
                if (Data[Idx] != VisibilityState::CurrentlyVisible)
                {
                    Data[Idx] = VisibilityState::CurrentlyVisible;
                    bChanged = true;
                }
            }
        }
    }

    if (bChanged)
    {
        // Add to dirty regions (for simplicity, we add the bounding box of the vision circle)
        FIntRect DirtyRect(MinX, MinY, MaxX, MaxY);
        DirtyRegions[PlayerIndex].push_back(DirtyRect);
    }
}

void FFogOfWarGrid::ClearCurrentVisibility(int32_t PlayerIndex)
{
    if (PlayerIndex < 0 || PlayerIndex >= NumPlayers) return;

    auto& Data = VisibilityData[PlayerIndex];
    for (auto& Cell : Data)
    {
        if (Cell == VisibilityState::CurrentlyVisible)
        {
            Cell = VisibilityState::PreviouslySeen;
        }
    }
}

VisibilityState FFogOfWarGrid::GetVisibility(int32_t PlayerIndex, int32_t X, int32_t Y) const
{
    if (PlayerIndex < 0 || PlayerIndex >= NumPlayers || X < 0 || X >= Width || Y < 0 || Y >= Height)
    {
        return VisibilityState::NeverSeen;
    }
    return VisibilityData[PlayerIndex][Y * Width + X];
}

const std::vector<FIntRect>& FFogOfWarGrid::GetDirtyRegions(int32_t PlayerIndex) const
{
    static const std::vector<FIntRect> Empty;
    if (PlayerIndex < 0 || PlayerIndex >= NumPlayers) return Empty;
    return DirtyRegions[PlayerIndex];
}

void FFogOfWarGrid::ClearDirtyRegions(int32_t PlayerIndex)
{
    if (PlayerIndex >= 0 && PlayerIndex < NumPlayers)
    {
        DirtyRegions[PlayerIndex].clear();
    }
}

} // namespace RA4
