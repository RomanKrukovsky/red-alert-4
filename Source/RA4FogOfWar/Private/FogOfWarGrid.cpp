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
    const size_t PlayersCount = NumPlayers > 0 ? static_cast<size_t>(NumPlayers) : 0;
    VisibilityData.resize(PlayersCount);
    DirtyRegions.resize(PlayersCount);

    const size_t CellCount = (Width > 0 && Height > 0) ? static_cast<size_t>(Width) * static_cast<size_t>(Height) : 0;
    for (size_t i = 0; i < PlayersCount; ++i)
    {
        VisibilityData[i].resize(CellCount, VisibilityState::NeverSeen);
    }
}

void FFogOfWarGrid::RevealCircularArea(int32_t PlayerIndex, int32_t CenterX, int32_t CenterY, int32_t Radius)
{
    if (PlayerIndex < 0 || PlayerIndex >= NumPlayers) return;

    const size_t PIdx = static_cast<size_t>(PlayerIndex);
    const int32_t MinX = std::max(0, CenterX - Radius);
    const int32_t MaxX = std::min(Width - 1, CenterX + Radius);
    const int32_t MinY = std::max(0, CenterY - Radius);
    const int32_t MaxY = std::min(Height - 1, CenterY + Radius);

    const int32_t RadiusSq = Radius * Radius;
    bool bChanged = false;

    auto& Data = VisibilityData[PIdx];

    for (int32_t Y = MinY; Y <= MaxY; ++Y)
    {
        for (int32_t X = MinX; X <= MaxX; ++X)
        {
            const int32_t DistSq = (X - CenterX) * (X - CenterX) + (Y - CenterY) * (Y - CenterY);
            if (DistSq <= RadiusSq)
            {
                const size_t Idx = static_cast<size_t>(Y) * static_cast<size_t>(Width) + static_cast<size_t>(X);
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
        const FIntRect DirtyRect(MinX, MinY, MaxX, MaxY);
        DirtyRegions[PIdx].push_back(DirtyRect);
    }
}

void FFogOfWarGrid::RevealRadarArea(int32_t PlayerIndex, int32_t CenterX, int32_t CenterY, int32_t Radius)
{
    if (PlayerIndex < 0 || PlayerIndex >= NumPlayers) return;

    const size_t PIdx = static_cast<size_t>(PlayerIndex);
    const int32_t MinX = std::max(0, CenterX - Radius);
    const int32_t MaxX = std::min(Width - 1, CenterX + Radius);
    const int32_t MinY = std::max(0, CenterY - Radius);
    const int32_t MaxY = std::min(Height - 1, CenterY + Radius);

    const int32_t RadiusSq = Radius * Radius;
    bool bChanged = false;

    auto& Data = VisibilityData[PIdx];

    for (int32_t Y = MinY; Y <= MaxY; ++Y)
    {
        for (int32_t X = MinX; X <= MaxX; ++X)
        {
            const int32_t DistSq = (X - CenterX) * (X - CenterX) + (Y - CenterY) * (Y - CenterY);
            if (DistSq > RadiusSq)
            {
                continue;
            }
            const size_t Idx = static_cast<size_t>(Y) * static_cast<size_t>(Width) + static_cast<size_t>(X);
            // Eyes-on vision outranks a blip and must not be downgraded, whichever order
            // the two reveals happen in this tick.
            if (Data[Idx] == VisibilityState::CurrentlyVisible ||
                Data[Idx] == VisibilityState::RadarDetected)
            {
                continue;
            }
            Data[Idx] = VisibilityState::RadarDetected;
            bChanged = true;
        }
    }

    if (bChanged)
    {
        DirtyRegions[PIdx].push_back(FIntRect(MinX, MinY, MaxX, MaxY));
    }
}

void FFogOfWarGrid::ClearCurrentVisibility(int32_t PlayerIndex)
{
    if (PlayerIndex < 0 || PlayerIndex >= NumPlayers) return;

    const size_t PIdx = static_cast<size_t>(PlayerIndex);
    auto& Data = VisibilityData[PIdx];
    for (auto& Cell : Data)
    {
        // Both live states decay to "seen before". Radar coverage is recomputed from
        // scratch every tick exactly like vision is, so a radar that goes dark -- destroyed,
        // sold, or shut down by a power deficit -- stops contributing on the next tick
        // rather than leaving its blips painted on the map forever.
        if (Cell == VisibilityState::CurrentlyVisible || Cell == VisibilityState::RadarDetected)
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
    const size_t PIdx = static_cast<size_t>(PlayerIndex);
    const size_t Idx = static_cast<size_t>(Y) * static_cast<size_t>(Width) + static_cast<size_t>(X);
    return VisibilityData[PIdx][Idx];
}

const std::vector<FIntRect>& FFogOfWarGrid::GetDirtyRegions(int32_t PlayerIndex) const
{
    static const std::vector<FIntRect> Empty;
    if (PlayerIndex < 0 || PlayerIndex >= NumPlayers) return Empty;
    const size_t PIdx = static_cast<size_t>(PlayerIndex);
    return DirtyRegions[PIdx];
}

void FFogOfWarGrid::ClearDirtyRegions(int32_t PlayerIndex)
{
    if (PlayerIndex >= 0 && PlayerIndex < NumPlayers)
    {
        const size_t PIdx = static_cast<size_t>(PlayerIndex);
        DirtyRegions[PIdx].clear();
    }
}

} // namespace RA4
