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
#include "RA4Recon/ReconTypes.h"

#ifndef RA4RECON_API
#define RA4RECON_API
#endif

namespace RA4
{

class ByteWriter;
class ByteReader;
class Hash64;

namespace Recon
{

class ReconSystem;

// Test scaffolding needs the writer API without going through a full match.
// Declared here, defined only in the test binary; a friend declaration is the
// narrowest possible opening (INVARIANT 9 allows writes from RA4Recon and its
// deterministic tests, never from presentation/UI/AI).
struct PerceivedWorldTestAccess;

class RA4RECON_API PerceivedWorld
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
    // Only the intel phases (ReconSystem) and the deterministic test harness may
    // mutate belief. Everything below being private is the fix for review
    // BLOCKER 2 -- a public mutable accessor was the violation, regardless of
    // how politely its comment asked callers not to use it.
    friend class ReconSystem;
    friend struct PerceivedWorldTestAccess;

    void Initialize(int32_t MapWidthTiles, int32_t MapHeightTiles, int32_t MaxTracks);
    void Reset();
    TrackId AllocateTrack();
    void ReleaseTrack(TrackId Id);
    PerceivedTrack* GetTrackMutable(TrackId Id);
    void SetLastObservedTick(int32_t TileX, int32_t TileY, TickIndex Tick);

    // The ONLY sanctioned way to move a track. Writing BelievedPosition directly
    // would silently desynchronise the spatial index, and the failure mode is
    // horrible: the track simply stops being found by region queries, so it
    // vanishes from the HUD while still existing. Callers use this instead.
    void SetTrackPosition(TrackId Id, const Vec2& NewPosition);
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

    // --- Spatial index (M3-perf) ----------------------------------------------
    // A coarse uniform grid over the map: cell -> slots whose believed position
    // falls in it. Repays the debt the linear GetTracksInRegion left behind, and it
    // is what the M3 review (finding M2) required before the perf milestone.
    //
    // Deliberately coarse: at kSpatialCellTiles the whole 256x256 map is a few
    // hundred cells, so the index is cheap to rebuild and cheap to hash. Cells hold
    // slot indices, not pointers, so nothing dangles when a slot is recycled.
    //
    // NOT serialized and NOT hashed: it is a pure function of the track positions
    // that already are. Rebuilt on load in Deserialize, so two peers agree by
    // construction rather than by agreement -- an index in the checksum would be a
    // second source of truth for the same data.
    static constexpr int32_t kSpatialCellTiles = 8;
    std::vector<std::vector<uint32_t>> SpatialCells;
    int32_t SpatialCellsX = 0;
    int32_t SpatialCellsY = 0;

    // Scratch for region queries; mutable because the query is logically const.
    // A member so repeated HUD queries allocate nothing.
    mutable std::vector<uint32_t> RegionScratch;

    void RebuildSpatialIndex();
    void SpatialInsert(uint32_t Slot);
    void SpatialRemove(uint32_t Slot);
    int32_t SpatialCellIndexOf(const Vec2& Position) const;

    // --- Reverse index: TrackId slot -> entity slots associated with it ---------
    // Lives in ReconSystem (it owns the association tables); declared here only as
    // a reminder that PerceivedWorld deliberately does NOT own it.
};

} // namespace Recon
} // namespace RA4
