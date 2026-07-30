// Copyright (c) Red Alert 4 project. Soft tile reservations for group movement.
//
// A reservation lets one unit claim a tile for the next few ticks so a second unit
// heading into the same tile waits or diverts instead of stacking on top of it.
// Ties are broken by entity slot index (lower wins) so the outcome is identical on
// every machine and every run -- a precondition for replay and lockstep checksums.
#pragma once

#include <cstdint>
#include <vector>

#include "RA4Core/Ids.h"
#include "RA4Navigation/NavGrid.h"

#ifndef RA4NAVIGATION_API
#define RA4NAVIGATION_API
#endif

namespace RA4
{
namespace Nav
{

struct NavDebugSnapshot;   // defined in RA4Navigation/NavDebug.h

using ::RA4::TickIndex;

constexpr uint32_t kInvalidReservationSlot = 0xFFFFFFFFu;

struct ReservationCell
{
    uint32_t OccupantSlot = kInvalidReservationSlot;
    TickIndex ExpiryTick = 0;   // 0 = free
};

class RA4NAVIGATION_API ReservationGrid
{
public:
    explicit ReservationGrid(int32_t InWidth, int32_t InHeight);

    int32_t GetWidth() const { return Width; }
    int32_t GetHeight() const { return Height; }

    bool IsInBounds(const TileCoord& Tile) const;

    // A tile is free at Now if it was never reserved, or its reservation expired at
    // or before Now. ExpiryTick == 0 is the sentinel for "never reserved".
    bool IsFree(const TileCoord& Tile, TickIndex Now) const;

    // Reserve Tile for Slot for the next HoldTicks ticks. Returns false (and does
    // nothing) if a higher-priority occupant -- a strictly lower slot index --
    // already holds the tile past Now. Equal or higher slots are displaced.
    bool TryReserve(const TileCoord& Tile, uint32_t Slot, TickIndex Now, int32_t HoldTicks);

    // Clears every cell owned by Slot. Called when a unit arrives or dies.
    void Release(uint32_t Slot);

    // Single deterministic sweep at the start of a tick. Marks expired cells free.
    // O(tiles) and called once per tick, not per unit.
    void Expire(TickIndex Now);

    // Pure-data snapshot for the presentation bridge and the headless tests.
    // No dependency on DrawDebug*; the test that exercises this is the canary.
    void Snapshot(NavDebugSnapshot& Out) const;

private:
    int32_t ToIndex(const TileCoord& Tile) const;

    int32_t Width = 0;
    int32_t Height = 0;
    std::vector<ReservationCell> Cells;
};

} // namespace Nav
} // namespace RA4