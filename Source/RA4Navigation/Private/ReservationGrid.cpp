// Copyright (c) Red Alert 4 project.
#include "RA4Navigation/ReservationGrid.h"

#include <algorithm>

namespace RA4
{
namespace Nav
{

ReservationGrid::ReservationGrid(int32_t InWidth, int32_t InHeight)
    : Width(std::max(0, InWidth))
    , Height(std::max(0, InHeight))
    , Cells(size_t(Width) * size_t(Height))
{
}

int32_t ReservationGrid::ToIndex(const TileCoord& Tile) const
{
    return Tile.Y * Width + Tile.X;
}

bool ReservationGrid::IsInBounds(const TileCoord& Tile) const
{
    return Tile.X >= 0 && Tile.Y >= 0 && Tile.X < Width && Tile.Y < Height;
}

bool ReservationGrid::IsFree(const TileCoord& Tile, TickIndex Now) const
{
    if (!IsInBounds(Tile))
    {
        return false;
    }
    const ReservationCell& C = Cells[size_t(ToIndex(Tile))];
    return C.OccupantSlot == kInvalidReservationSlot || C.ExpiryTick <= Now;
}

bool ReservationGrid::TryReserve(const TileCoord& Tile, uint32_t Slot, TickIndex Now, int32_t HoldTicks)
{
    if (!IsInBounds(Tile))
    {
        return false;
    }
    const int32_t Idx = ToIndex(Tile);
    ReservationCell& C = Cells[size_t(Idx)];
    // Displace only if the existing holder has expired OR the new slot is strictly
    // lower (the documented tie-break). Equal slots do not displace -- a unit never
    // needs to take its own tile twice.
    const bool bExpired = C.OccupantSlot == kInvalidReservationSlot || C.ExpiryTick <= Now;
    const bool bLowerWins = Slot < C.OccupantSlot;
    if (!bExpired && !bLowerWins)
    {
        return false;
    }
    C.OccupantSlot = Slot;
    C.ExpiryTick = Now + static_cast<TickIndex>(HoldTicks) + TickIndex{1};
    return true;
}

void ReservationGrid::Release(uint32_t Slot)
{
    for (ReservationCell& C : Cells)
    {
        if (C.OccupantSlot == Slot)
        {
            C.OccupantSlot = kInvalidReservationSlot;
            C.ExpiryTick = 0;
        }
    }
}

void ReservationGrid::Expire(TickIndex Now)
{
    for (ReservationCell& C : Cells)
    {
        if (C.OccupantSlot != kInvalidReservationSlot && C.ExpiryTick <= Now)
        {
            C.OccupantSlot = kInvalidReservationSlot;
            C.ExpiryTick = 0;
        }
    }
}

} // namespace Nav
} // namespace RA4