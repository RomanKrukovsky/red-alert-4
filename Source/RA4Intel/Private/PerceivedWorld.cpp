// Copyright (c) Red Alert 4 project.
#include "RA4Intel/PerceivedWorld.h"

#include <algorithm>

#include "RA4Core/ByteStream.h"
#include "RA4Core/Checksum.h"
#include "RA4Core/SimConfig.h"

namespace RA4
{
namespace Intel
{

namespace
{
// Serialization is versioned independently of the SimWorld save version so that
// intel format changes (frequent while the feature matures) do not force a bump
// of the outer save format every time.
constexpr uint32_t kPerceivedWorldVersion = 3; // v3: DecayCursor (I-B4); v2: phantom side table
} // namespace

void PerceivedWorld::Initialize(int32_t MapWidthTiles, int32_t MapHeightTiles, int32_t MaxTracks)
{
    Reset();
    MapWidth = MapWidthTiles;
    MapHeight = MapHeightTiles;
    LastObserved.assign(size_t(MapWidth) * size_t(MapHeight), 0);
    // Reserve the hard cap up front: track allocation must never allocate in the
    // steady state (performance budget: zero hot-path allocations).
    Tracks.reserve(size_t(MaxTracks));
    PhantomFlags.reserve(size_t(MaxTracks));
    FreeSlots.reserve(size_t(MaxTracks));
}

void PerceivedWorld::Reset()
{
    Tracks.clear();
    PhantomFlags.clear();
    FreeSlots.clear();
    LastObserved.clear();
    HighWaterMark = 0;
    AliveCount = 0;
    DecayCursor = 0;
    MapWidth = 0;
    MapHeight = 0;
}

TrackId PerceivedWorld::AllocateTrack()
{
    uint32_t Index;
    if (!FreeSlots.empty())
    {
        Index = FreeSlots.back();
        FreeSlots.pop_back();
    }
    else
    {
        if (Tracks.size() >= Tracks.capacity() && Tracks.capacity() > 0)
        {
            // Hard cap reached. Refusing the allocation is the correct failure
            // mode: the aggregation phase must merge harder instead of growing
            // without bound (memory budget §6).
            return TrackId{};
        }
        Index = HighWaterMark;
        HighWaterMark += 1;
        Tracks.emplace_back();
        PhantomFlags.push_back(0);
    }

    PerceivedTrack& T = Tracks[Index];
    const uint32_t Generation = T.Id.Generation + 1; // survives slot reuse
    T = PerceivedTrack{};
    T.Id.Index = Index;
    T.Id.Generation = Generation;
    T.bAlive = true;
    PhantomFlags[Index] = 0;
    AliveCount += 1;
    return T.Id;
}

void PerceivedWorld::ReleaseTrack(TrackId Id)
{
    PerceivedTrack* T = GetTrackMutable(Id);
    if (T == nullptr)
    {
        return;
    }
    T->bAlive = false;
    FreeSlots.push_back(Id.Index);
    AliveCount -= 1;
}

bool PerceivedWorld::IsTrackAlive(TrackId Id) const
{
    return GetTrack(Id) != nullptr;
}

PerceivedTrack* PerceivedWorld::GetTrackMutable(TrackId Id)
{
    if (!Id.IsValid() || Id.Index >= Tracks.size())
    {
        return nullptr;
    }
    PerceivedTrack& T = Tracks[Id.Index];
    if (!T.bAlive || T.Id.Generation != Id.Generation)
    {
        return nullptr;
    }
    return &T;
}

const PerceivedTrack* PerceivedWorld::GetTrack(TrackId Id) const
{
    return const_cast<PerceivedWorld*>(this)->GetTrackMutable(Id);
}

bool PerceivedWorld::IsTrackPhantomInternal(TrackId Id) const
{
    const PerceivedTrack* T = GetTrack(Id);
    return T != nullptr && PhantomFlags[Id.Index] != 0;
}

void PerceivedWorld::SetTrackPhantomInternal(TrackId Id, bool bPhantom)
{
    if (GetTrackMutable(Id) != nullptr)
    {
        PhantomFlags[Id.Index] = bPhantom ? 1 : 0;
    }
}

void PerceivedWorld::GetTracksInRegion(int32_t MinTileX, int32_t MinTileY, int32_t MaxTileX, int32_t MaxTileY,
                                       std::vector<const PerceivedTrack*>& Out) const
{
    // Linear scan for M0/M1. The spatial grid arrives with the aggregation phase
    // (M3) where it is needed for merging anyway; adding it now would be
    // speculative structure with no measured need.
    const Fixed MinX = Fixed::FromInt(MinTileX) * kTileSize;
    const Fixed MinY = Fixed::FromInt(MinTileY) * kTileSize;
    const Fixed MaxX = Fixed::FromInt(MaxTileX + 1) * kTileSize;
    const Fixed MaxY = Fixed::FromInt(MaxTileY + 1) * kTileSize;

    for (uint32_t I = 0; I < HighWaterMark; ++I)
    {
        const PerceivedTrack& T = Tracks[I];
        if (!T.bAlive)
        {
            continue;
        }
        if (T.BelievedPosition.X < MinX || T.BelievedPosition.X >= MaxX ||
            T.BelievedPosition.Y < MinY || T.BelievedPosition.Y >= MaxY)
        {
            continue;
        }
        Out.push_back(&T);
    }
}

TickIndex PerceivedWorld::GetLastObservedTick(int32_t TileX, int32_t TileY) const
{
    if (!IsTileInside(TileX, TileY))
    {
        return 0;
    }
    return LastObserved[size_t(TileIndex(TileX, TileY))];
}

void PerceivedWorld::SetLastObservedTick(int32_t TileX, int32_t TileY, TickIndex Tick)
{
    if (!IsTileInside(TileX, TileY))
    {
        return;
    }
    LastObserved[size_t(TileIndex(TileX, TileY))] = Tick;
}

void PerceivedWorld::Serialize(ByteWriter& W) const
{
    W.WriteUInt32(kPerceivedWorldVersion);
    W.WriteInt32(MapWidth);
    W.WriteInt32(MapHeight);
    W.WriteUInt32(HighWaterMark);
    W.WriteUInt32(DecayCursor);
    W.WriteUInt32(uint32_t(Tracks.capacity()));

    for (uint32_t I = 0; I < HighWaterMark; ++I)
    {
        const PerceivedTrack& T = Tracks[I];
        W.WriteBool(T.bAlive);
        W.WriteUInt32(T.Id.Generation); // dead slots keep generation for safe reuse
        if (!T.bAlive)
        {
            continue;
        }
        W.WriteUInt32(T.BelievedClass.Value);
        W.WriteInt64(T.BelievedPosition.X.Raw);
        W.WriteInt64(T.BelievedPosition.Y.Raw);
        W.WriteInt64(T.PositionErrorRadius.Raw);
        W.WriteInt32(T.BelievedCountMin);
        W.WriteInt32(T.BelievedCountMax);
        W.WriteUInt32(T.LastUpdateTick);
        W.WriteInt64(T.Confidence.Raw);
        W.WriteUInt8(T.IndependentSourceCount);
        W.WriteBool(T.bStale);
        W.WriteBool(PhantomFlags[I] != 0); // side table, serialized in slot order
        W.WriteBool(T.bContested);
        for (uint32_t P = 0; P < kTrackProvenanceSize; ++P)
        {
            W.WriteUInt32(T.ProvenanceReportIds[P]);
        }
        W.WriteUInt8(T.ProvenanceCount);
    }

    W.WriteUInt32(uint32_t(FreeSlots.size()));
    for (uint32_t Slot : FreeSlots)
    {
        W.WriteUInt32(Slot);
    }

    W.WriteUInt32(uint32_t(LastObserved.size()));
    for (TickIndex Tick : LastObserved)
    {
        W.WriteUInt32(Tick);
    }
}

bool PerceivedWorld::Deserialize(ByteReader& R)
{
    if (R.ReadUInt32() != kPerceivedWorldVersion)
    {
        return false;
    }

    Reset();
    MapWidth = R.ReadInt32();
    MapHeight = R.ReadInt32();
    HighWaterMark = R.ReadUInt32();
    DecayCursor = R.ReadUInt32();
    const uint32_t Capacity = R.ReadUInt32();
    Tracks.reserve(Capacity);
    PhantomFlags.reserve(Capacity);
    FreeSlots.reserve(Capacity);
    Tracks.resize(HighWaterMark);
    PhantomFlags.assign(HighWaterMark, 0);

    AliveCount = 0;
    for (uint32_t I = 0; I < HighWaterMark; ++I)
    {
        PerceivedTrack& T = Tracks[I];
        T = PerceivedTrack{};
        T.Id.Index = I;
        T.bAlive = R.ReadBool();
        T.Id.Generation = R.ReadUInt32();
        if (!T.bAlive)
        {
            continue;
        }
        AliveCount += 1;
        T.BelievedClass = ContentId(R.ReadUInt32());
        T.BelievedPosition.X = Fixed::FromRaw(R.ReadInt64());
        T.BelievedPosition.Y = Fixed::FromRaw(R.ReadInt64());
        T.PositionErrorRadius = Fixed::FromRaw(R.ReadInt64());
        T.BelievedCountMin = R.ReadInt32();
        T.BelievedCountMax = R.ReadInt32();
        T.LastUpdateTick = R.ReadUInt32();
        T.Confidence = Fixed::FromRaw(R.ReadInt64());
        T.IndependentSourceCount = R.ReadUInt8();
        T.bStale = R.ReadBool();
        PhantomFlags[I] = R.ReadBool() ? 1 : 0;
        T.bContested = R.ReadBool();
        for (uint32_t P = 0; P < kTrackProvenanceSize; ++P)
        {
            T.ProvenanceReportIds[P] = R.ReadUInt32();
        }
        T.ProvenanceCount = R.ReadUInt8();
    }

    const uint32_t FreeCount = R.ReadUInt32();
    for (uint32_t I = 0; I < FreeCount; ++I)
    {
        FreeSlots.push_back(R.ReadUInt32());
    }

    const uint32_t ObservedCount = R.ReadUInt32();
    LastObserved.resize(ObservedCount);
    for (uint32_t I = 0; I < ObservedCount; ++I)
    {
        LastObserved[I] = R.ReadUInt32();
    }
    return true;
}

void PerceivedWorld::FeedChecksum(Hash64& H) const
{
    // Belief state feeds the match checksum because the AI's commands depend on
    // it (M6): a divergent belief on one peer is a real desync and must be caught
    // on the tick it happens, not when it eventually changes a unit position.
    H.FeedUInt32(HighWaterMark);
    H.FeedUInt32(DecayCursor);
    for (uint32_t I = 0; I < HighWaterMark; ++I)
    {
        const PerceivedTrack& T = Tracks[I];
        H.FeedBool(T.bAlive);
        if (!T.bAlive)
        {
            continue;
        }
        H.FeedUInt32(I);
        H.FeedUInt32(T.Id.Generation);
        H.FeedUInt32(T.BelievedClass.Value);
        H.FeedInt64(T.BelievedPosition.X.Raw);
        H.FeedInt64(T.BelievedPosition.Y.Raw);
        H.FeedInt64(T.PositionErrorRadius.Raw);
        H.FeedInt32(T.BelievedCountMin);
        H.FeedInt32(T.BelievedCountMax);
        H.FeedUInt32(T.LastUpdateTick);
        H.FeedInt64(T.Confidence.Raw);
        H.FeedUInt8(T.IndependentSourceCount);
        H.FeedBool(T.bStale);
        H.FeedBool(PhantomFlags[I] != 0);
        H.FeedBool(T.bContested);
    }
    // LastObserved is hashed coarsely (size only): per-tile hashing of a 256x256
    // map every checksum tick would dominate the hash cost for data that only
    // changes when tracks change anyway. Revisit at M5 with profiler numbers.
    H.FeedUInt32(uint32_t(LastObserved.size()));
}

} // namespace Intel
} // namespace RA4
