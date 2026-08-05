// Copyright (c) Red Alert 4 project. One player's HQ map: the world as they believe it.
//
// The perceived world is the ONLY read surface for enemy information: the HUD,
// the minimap and (from M6) the AI commander all query tracks here and never the
// ground-truth entity arrays. It is deterministic simulation state -- serialized
// and checksummed with the match -- because the AI's future commands depend on it,
// so a divergent belief is a desync, caught by the existing lockstep machinery.
#pragma once

#include <cstdint>
#include <vector>

#include "RA4Core/Ids.h"
#include "RA4Core/Vector.h"
#include "RA4Intel/IntelTypes.h"

#ifndef RA4INTEL_API
#define RA4INTEL_API
#endif

namespace RA4
{

class ByteWriter;
class ByteReader;
class Hash64;

namespace Intel
{

class IntelSystem;

// Test scaffolding needs the writer API without going through a full match.
// Declared here, defined only in the test binary; a friend declaration is the
// narrowest possible opening (INVARIANT 9 allows writes from RA4Intel and its
// deterministic tests, never from presentation/UI/AI).
struct PerceivedWorldTestAccess;

class RA4INTEL_API PerceivedWorld
{
public:
    // --- Read surface (the ONLY thing presentation/UI/AI may touch) ----------
    bool IsTrackAlive(TrackId Id) const;
    const PerceivedTrack* GetTrack(TrackId Id) const;
    uint32_t GetAliveTrackCount() const { return AliveCount; }
    uint32_t GetTrackCapacity() const { return uint32_t(Tracks.size()); }

    // Appends alive tracks intersecting the tile rectangle. Output order is slot
    // order, which is deterministic because allocation and recycling are.
    void GetTracksInRegion(int32_t MinTileX, int32_t MinTileY, int32_t MaxTileX, int32_t MaxTileY,
                           std::vector<const PerceivedTrack*>& Out) const;

    // Negative knowledge (§4.6): the tick a friendly source last observed a tile.
    // 0 means "never" -- an unscouted region reads differently from a scouted-empty
    // one, and the UI must be able to show that difference.
    TickIndex GetLastObservedTick(int32_t TileX, int32_t TileY) const;

    // --- Determinism plumbing (const half) -----------------------------------
    void Serialize(ByteWriter& W) const;
    void FeedChecksum(Hash64& H) const;

private:
    // --- Writer API: structural, not disciplinary (INVARIANT 9) --------------
    // Only the intel phases (IntelSystem) and the deterministic test harness may
    // mutate belief. Everything below being private is the fix for review
    // BLOCKER 2 -- a public mutable accessor was the violation, regardless of
    // how politely its comment asked callers not to use it.
    friend class IntelSystem;
    friend struct PerceivedWorldTestAccess;

    void Initialize(int32_t MapWidthTiles, int32_t MapHeightTiles, int32_t MaxTracks);
    void Reset();
    TrackId AllocateTrack();
    void ReleaseTrack(TrackId Id);
    PerceivedTrack* GetTrackMutable(TrackId Id);
    void SetLastObservedTick(int32_t TileX, int32_t TileY, TickIndex Tick);
    bool Deserialize(ByteReader& R);

    // Ground-truth phantom bookkeeping. Lives OUTSIDE PerceivedTrack on purpose:
    // the struct is the read surface, and a truth flag inside it leaks whether a
    // contact is real to any UI/AI caller (INVARIANT 10, review BLOCKER 1). The
    // refutation logic (M4) and the debug overlay are the only readers.
    bool IsTrackPhantomInternal(TrackId Id) const;
    void SetTrackPhantomInternal(TrackId Id, bool bPhantom);

    int32_t TileIndex(int32_t X, int32_t Y) const { return Y * MapWidth + X; }
    bool IsTileInside(int32_t X, int32_t Y) const
    {
        return X >= 0 && Y >= 0 && X < MapWidth && Y < MapHeight;
    }

    std::vector<PerceivedTrack> Tracks;      // dense slots; bAlive marks occupancy
    std::vector<uint8_t> PhantomFlags;       // parallel to Tracks; core-internal truth
    std::vector<uint32_t> FreeSlots;         // LIFO recycle, same scheme as SimWorld
    uint32_t HighWaterMark = 0;
    uint32_t AliveCount = 0;

    // Round-robin cursor for the amortized decay sweep (ADR-0021, I-B4).
    // PhaseTrackUpdate (M2) resumes here and advances by TrackTuning::
    // TracksPerTickBudget slots per tick, wrapping at HighWaterMark. Sim state:
    // serialized and checksummed, because sweep position determines *which tick*
    // a given track's confidence drops -- two peers with different cursors would
    // diverge the moment decay math lands.
    uint32_t DecayCursor = 0;

    std::vector<TickIndex> LastObserved;     // per-tile negative knowledge
    int32_t MapWidth = 0;
    int32_t MapHeight = 0;
};

} // namespace Intel
} // namespace RA4
