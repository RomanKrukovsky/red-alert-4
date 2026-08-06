// Copyright (c) Red Alert 4 project.
#include "RA4Recon/PerceivedWorld.h"

#include <algorithm>

#include "RA4Core/ByteStream.h"
#include "RA4Core/Checksum.h"
#include "RA4Core/SimConfig.h"

namespace RA4
{
namespace Recon
{

namespace
{
// Serialization is versioned independently of the SimWorld save version so that
// intel format changes (frequent while the feature matures) do not force a bump
// of the outer save format every time.
constexpr uint32_t kPerceivedWorldVersion = 5; // v5: PhantomBornTick (M4) // v4: category/anonymous/node (M3 review B3) // v3: DecayCursor (I-B4); v2: phantom side table
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

    // Spatial index sized to the map, once. Cells are reserved rather than grown
    // per insert so a busy tick allocates nothing.
    SpatialCellsX = (MapWidth + kSpatialCellTiles - 1) / kSpatialCellTiles;
    SpatialCellsY = (MapHeight + kSpatialCellTiles - 1) / kSpatialCellTiles;
    SpatialCells.assign(size_t(SpatialCellsX) * size_t(SpatialCellsY), {});
}

void PerceivedWorld::Reset()
{
    Tracks.clear();
    PhantomFlags.clear();
    FreeSlots.clear();
    LastObserved.clear();
    SpatialCells.clear();
    SpatialCellsX = 0;
    SpatialCellsY = 0;
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
    SpatialInsert(Index);
    return T.Id;
}

void PerceivedWorld::ReleaseTrack(TrackId Id)
{
    PerceivedTrack* T = GetTrackMutable(Id);
    if (T == nullptr)
    {
        return;
    }
    // Drop from the spatial index while the position still means something.
    SpatialRemove(Id.Index);
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

int32_t PerceivedWorld::SpatialCellIndexOf(const Vec2& Position) const
{
    if (SpatialCellsX <= 0 || SpatialCellsY <= 0)
    {
        return -1;
    }
    const int64_t TileX = Position.X.ToIntFloor() / kTileSizeUnits;
    const int64_t TileY = Position.Y.ToIntFloor() / kTileSizeUnits;
    // Clamp rather than reject: a track just off the map edge (position error can
    // push one there) still belongs to the nearest cell, so a region query near the
    // border cannot silently lose it.
    const int32_t CellX = int32_t(TileX < 0 ? 0 : TileX / kSpatialCellTiles);
    const int32_t CellY = int32_t(TileY < 0 ? 0 : TileY / kSpatialCellTiles);
    const int32_t ClampedX = CellX >= SpatialCellsX ? SpatialCellsX - 1 : CellX;
    const int32_t ClampedY = CellY >= SpatialCellsY ? SpatialCellsY - 1 : CellY;
    return ClampedY * SpatialCellsX + ClampedX;
}

void PerceivedWorld::SpatialInsert(uint32_t Slot)
{
    const int32_t Cell = SpatialCellIndexOf(Tracks[Slot].BelievedPosition);
    if (Cell >= 0)
    {
        SpatialCells[size_t(Cell)].push_back(Slot);
    }
}

void PerceivedWorld::SpatialRemove(uint32_t Slot)
{
    // Swap-erase: cell order is not meaningful, and region queries sort by slot
    // afterwards, so the ORDER inside a cell cannot affect simulation results.
    const int32_t Cell = SpatialCellIndexOf(Tracks[Slot].BelievedPosition);
    if (Cell < 0)
    {
        return;
    }
    std::vector<uint32_t>& Bucket = SpatialCells[size_t(Cell)];
    for (size_t I = 0; I < Bucket.size(); ++I)
    {
        if (Bucket[I] == Slot)
        {
            Bucket[I] = Bucket.back();
            Bucket.pop_back();
            return;
        }
    }
}

void PerceivedWorld::RebuildSpatialIndex()
{
    for (std::vector<uint32_t>& Bucket : SpatialCells)
    {
        Bucket.clear();
    }
    for (uint32_t I = 0; I < HighWaterMark; ++I)
    {
        if (Tracks[I].bAlive)
        {
            SpatialInsert(I);
        }
    }
}

void PerceivedWorld::GetTracksInRegion(int32_t MinTileX, int32_t MinTileY, int32_t MaxTileX, int32_t MaxTileY,
                                       std::vector<const PerceivedTrack*>& Out) const
{
    // Grid-accelerated (M3-perf). Only the cells overlapping the query rectangle
    // are visited, so a HUD query for one screenful no longer walks every track on
    // the map. Output stays in ascending SLOT order, which is what callers and the
    // determinism tests rely on -- cell traversal order is an implementation detail
    // and must not leak into results.
    const Fixed MinX = Fixed::FromInt(MinTileX) * kTileSize;
    const Fixed MinY = Fixed::FromInt(MinTileY) * kTileSize;
    const Fixed MaxX = Fixed::FromInt(MaxTileX + 1) * kTileSize;
    const Fixed MaxY = Fixed::FromInt(MaxTileY + 1) * kTileSize;

    if (SpatialCellsX <= 0 || SpatialCellsY <= 0)
    {
        return; // uninitialised world: no tracks to find
    }

    const auto ClampCell = [](int32_t V, int32_t Count)
    {
        return V < 0 ? 0 : (V >= Count ? Count - 1 : V);
    };
    const int32_t CellMinX = ClampCell(MinTileX / kSpatialCellTiles, SpatialCellsX);
    const int32_t CellMaxX = ClampCell(MaxTileX / kSpatialCellTiles, SpatialCellsX);
    const int32_t CellMinY = ClampCell(MinTileY / kSpatialCellTiles, SpatialCellsY);
    const int32_t CellMaxY = ClampCell(MaxTileY / kSpatialCellTiles, SpatialCellsY);

    // Gather candidate slots, then walk them in slot order. Sorting the small
    // candidate set is cheaper than scanning every track, and it preserves the
    // documented output order exactly.
    RegionScratch.clear();
    for (int32_t CellY = CellMinY; CellY <= CellMaxY; ++CellY)
    {
        for (int32_t CellX = CellMinX; CellX <= CellMaxX; ++CellX)
        {
            const std::vector<uint32_t>& Bucket = SpatialCells[size_t(CellY * SpatialCellsX + CellX)];
            RegionScratch.insert(RegionScratch.end(), Bucket.begin(), Bucket.end());
        }
    }
    std::sort(RegionScratch.begin(), RegionScratch.end());

    for (uint32_t I : RegionScratch)
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

void PerceivedWorld::SetTrackPosition(TrackId Id, const Vec2& NewPosition)
{
    PerceivedTrack* T = GetTrackMutable(Id);
    if (T == nullptr)
    {
        return;
    }
    // Reindex only when the cell actually changes: most position updates are small
    // corrections inside one cell, and a bucket erase-and-push per update would cost
    // more than the query saves.
    const int32_t OldCell = SpatialCellIndexOf(T->BelievedPosition);
    const int32_t NewCell = SpatialCellIndexOf(NewPosition);
    if (OldCell != NewCell)
    {
        SpatialRemove(Id.Index);
        T->BelievedPosition = NewPosition;
        SpatialInsert(Id.Index);
    }
    else
    {
        T->BelievedPosition = NewPosition;
    }
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
        // All three gate FUTURE aggregation: the nearby-track merge search filters
        // on category and anonymity, and corroboration compares LastReportNodeId.
        // Restoring a track without them makes the merge match the wrong tracks and
        // makes every node look like a new independent source (M3 review B3).
        W.WriteUInt8(uint8_t(T.BelievedCategory));
        W.WriteBool(T.bAnonymous);
        W.WriteUInt32(T.LastReportNodeId);
        W.WriteUInt32(T.LastDecayTick);
        W.WriteInt32(T.LastClaimedCount);
        W.WriteUInt32(T.PhantomBornTick);
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
    // Size the spatial grid for the restored map before anything is inserted into
    // it. Reset() cleared it, and RebuildSpatialIndex() below writes into cells that
    // must already exist -- without this the index came back empty and every track
    // silently vanished from region queries while still existing.
    SpatialCellsX = (MapWidth + kSpatialCellTiles - 1) / kSpatialCellTiles;
    SpatialCellsY = (MapHeight + kSpatialCellTiles - 1) / kSpatialCellTiles;
    SpatialCells.assign(size_t(SpatialCellsX > 0 ? SpatialCellsX : 0) *
                            size_t(SpatialCellsY > 0 ? SpatialCellsY : 0),
                        {});
    HighWaterMark = R.ReadUInt32();
    DecayCursor = R.ReadUInt32();
    if (HighWaterMark > 0 && DecayCursor >= HighWaterMark)
    {
        // A corrupt or hand-edited save must not park the sweep out of range;
        // wrapping here matches the M2 sweep's own wrap rule, deterministically.
        DecayCursor %= HighWaterMark;
    }
    if (HighWaterMark == 0)
    {
        DecayCursor = 0;
    }
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
        T.BelievedCategory = ObservedCategory(R.ReadUInt8());
        T.bAnonymous = R.ReadBool();
        T.LastReportNodeId = uint16_t(R.ReadUInt32());
        T.LastDecayTick = R.ReadUInt32();
        T.LastClaimedCount = R.ReadInt32();
        T.PhantomBornTick = R.ReadUInt32();
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

    // The index is a pure function of the positions just restored, so it is rebuilt
    // rather than serialized: two peers then agree by construction instead of by
    // trusting a second copy of the same data.
    RebuildSpatialIndex();

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
        // Hashed alongside serialization, not instead of it: the M3 review found
        // fields hashed but not written, which turns a quiet divergence into a
        // guaranteed post-load desync. Both halves must list the same fields.
        H.FeedUInt8(uint8_t(T.BelievedCategory));
        H.FeedBool(T.bAnonymous);
        H.FeedUInt32(T.LastReportNodeId);
        H.FeedUInt32(T.LastDecayTick);
        H.FeedInt32(T.LastClaimedCount);
        H.FeedUInt32(T.PhantomBornTick);
    }
    // LastObserved is hashed coarsely (size only): per-tile hashing of a 256x256
    // map every checksum tick would dominate the hash cost for data that only
    // changes when tracks change anyway. Revisit at M5 with profiler numbers.
    H.FeedUInt32(uint32_t(LastObserved.size()));
}

} // namespace Recon
} // namespace RA4
