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

class RA4INTEL_API PerceivedWorld
{
public:
    void Initialize(int32_t MapWidthTiles, int32_t MapHeightTiles, int32_t MaxTracks);
    void Reset();

    // --- Track slots (generational, recycled) --------------------------------
    TrackId AllocateTrack();
    void ReleaseTrack(TrackId Id);
    bool IsTrackAlive(TrackId Id) const;
    PerceivedTrack* GetTrack(TrackId Id);
    const PerceivedTrack* GetTrack(TrackId Id) const;
    uint32_t GetAliveTrackCount() const { return AliveCount; }
    uint32_t GetTrackCapacity() const { return uint32_t(Tracks.size()); }

    // --- Queries (UI/AI read surface, §4.6) -----------------------------------
    // Appends alive tracks intersecting the tile rectangle. Output order is slot
    // order, which is deterministic because allocation and recycling are.
    void GetTracksInRegion(int32_t MinTileX, int32_t MinTileY, int32_t MaxTileX, int32_t MaxTileY,
                           std::vector<const PerceivedTrack*>& Out) const;

    // Negative knowledge (§4.6): the tick a friendly source last observed a tile.
    // 0 means "never" -- an unscouted region reads differently from a scouted-empty
    // one, and the UI must be able to show that difference.
    TickIndex GetLastObservedTick(int32_t TileX, int32_t TileY) const;
    void SetLastObservedTick(int32_t TileX, int32_t TileY, TickIndex Tick);

    // --- Determinism plumbing --------------------------------------------------
    void Serialize(ByteWriter& W) const;
    bool Deserialize(ByteReader& R);
    void FeedChecksum(Hash64& H) const;

private:
    int32_t TileIndex(int32_t X, int32_t Y) const { return Y * MapWidth + X; }
    bool IsTileInside(int32_t X, int32_t Y) const
    {
        return X >= 0 && Y >= 0 && X < MapWidth && Y < MapHeight;
    }

    std::vector<PerceivedTrack> Tracks;      // dense slots; bAlive marks occupancy
    std::vector<uint32_t> FreeSlots;         // LIFO recycle, same scheme as SimWorld
    uint32_t HighWaterMark = 0;
    uint32_t AliveCount = 0;

    std::vector<TickIndex> LastObserved;     // per-tile negative knowledge
    int32_t MapWidth = 0;
    int32_t MapHeight = 0;
};

} // namespace Intel
} // namespace RA4
