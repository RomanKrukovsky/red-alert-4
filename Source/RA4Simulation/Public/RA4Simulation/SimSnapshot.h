// Copyright (c) Red Alert 4 project. Deterministic state snapshots and rollback buffer.
#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <memory>

#include "RA4Core/Checksum.h"
#include "RA4Core/Ids.h"

namespace RA4
{

// A complete snapshot of the simulation world state at a specific tick.
// Used for rollback networking, replay fast-forward/scrubbing, and desync diagnosis.
struct SimSnapshot
{
    TickIndex Tick = 0;
    uint64_t Checksum = 0;
    StateHashBreakdown Breakdown;
    std::vector<uint8_t> StateBuffer;
    bool bValid = false;

    void Clear()
    {
        Tick = 0;
        Checksum = 0;
        Breakdown = StateHashBreakdown{};
        StateBuffer.clear();
        bValid = false;
    }
};

// Fixed-capacity circular buffer storing consecutive tick snapshots.
// Keeps the last N ticks (e.g., 128 ticks = 6.4s @ 20Hz) available for instant rollback.
class SnapshotRingBuffer
{
public:
    explicit SnapshotRingBuffer(size_t InCapacity = 128)
        : Capacity(InCapacity > 0 ? InCapacity : 128)
    {
        Ring.resize(Capacity);
    }

    void Push(const SimSnapshot& Snapshot)
    {
        Ring[Head] = Snapshot;
        Head = (Head + 1) % Capacity;
        if (Count < Capacity)
        {
            Count++;
        }
    }

    void Push(SimSnapshot&& Snapshot)
    {
        Ring[Head] = std::move(Snapshot);
        Head = (Head + 1) % Capacity;
        if (Count < Capacity)
        {
            Count++;
        }
    }

    bool GetSnapshot(TickIndex Tick, SimSnapshot& OutSnapshot) const
    {
        if (Count == 0)
        {
            return false;
        }

        // Iterate backwards from newest to oldest
        for (size_t I = 0; I < Count; ++I)
        {
            const size_t Index = (Head + Capacity - 1 - I) % Capacity;
            if (Ring[Index].bValid && Ring[Index].Tick == Tick)
            {
                OutSnapshot = Ring[Index];
                return true;
            }
        }
        return false;
    }

    const SimSnapshot* GetLatest() const
    {
        if (Count == 0)
        {
            return nullptr;
        }
        const size_t Index = (Head + Capacity - 1) % Capacity;
        return Ring[Index].bValid ? &Ring[Index] : nullptr;
    }

    void Clear()
    {
        for (auto& S : Ring)
        {
            S.Clear();
        }
        Head = 0;
        Count = 0;
    }

    size_t NumSnapshots() const { return Count; }
    size_t GetCapacity() const { return Capacity; }

    TickIndex GetOldestTick() const
    {
        if (Count == 0) return 0;
        const size_t Index = (Head + Capacity - Count) % Capacity;
        return Ring[Index].Tick;
    }

    TickIndex GetNewestTick() const
    {
        if (Count == 0) return 0;
        const size_t Index = (Head + Capacity - 1) % Capacity;
        return Ring[Index].Tick;
    }

private:
    std::vector<SimSnapshot> Ring;
    size_t Capacity = 128;
    size_t Head = 0;
    size_t Count = 0;
};

} // namespace RA4
