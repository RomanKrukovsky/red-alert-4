// Copyright (c) Red Alert 4 project.
#include "RA4Simulation/MapFile.h"

#include "RA4Content/ContentDatabase.h"
#include "RA4Core/SimConfig.h"
#include "RA4Simulation/SimWorld.h"

#include <cstdio>

namespace RA4
{

// The map format's dimension ceiling is the simulation's, not a second opinion.
// SimConfig.h owns the number; asserting the relationship here means a future change
// to kMaxMapTiles cannot leave a map file able to describe a grid the simulation
// refuses to run.
static_assert(kMinMapTilesPerSide <= kMaxMapTiles, "map size floor must not exceed the simulation ceiling");
static_assert(kMinStartPositions <= int32_t(kMaxPlayers), "start position floor must fit in the player slots");

// MapDescription keeps a local copy of the tile size so its header does not force
// SimConfig on every consumer; SimWorld.cpp asserts they agree, and so does this
// file, because StartPositionToTile divides by the local one.
static_assert(MapDescription::kTileSizeUnitsLocal == kTileSizeUnits,
              "MapDescription::kTileSizeUnitsLocal has drifted from kTileSizeUnits");

namespace
{

MapValidationResult MakeFailure(MapValidationReason Reason)
{
    MapValidationResult R;
    R.Reason = Reason;
    return R;
}

MapValidationResult MakeFailureAt(MapValidationReason Reason, const TileCoord& Tile, int32_t Index)
{
    MapValidationResult R;
    R.Reason = Reason;
    R.Tile = Tile;
    R.Index = Index;
    return R;
}

MapValidationResult MakeFailureValue(MapValidationReason Reason, int64_t Value)
{
    MapValidationResult R;
    R.Reason = Reason;
    R.Value = Value;
    return R;
}

// ByteWriter::WriteString writes a uint16 length, so a longer name would be
// truncated on write and silently produce a different map on read. Refused rather
// than truncated: a name is how a map is identified in a lobby and a replay header.
constexpr size_t kMaxNameLength = 0xFFFFu;

} // namespace

// ---------------------------------------------------------------------------
// Reason reporting
// ---------------------------------------------------------------------------

const char* ToString(MapValidationReason Reason)
{
    switch (Reason)
    {
        case MapValidationReason::Valid: return "Valid";
        case MapValidationReason::BadMagic: return "BadMagic";
        case MapValidationReason::UnsupportedVersion: return "UnsupportedVersion";
        case MapValidationReason::Truncated: return "Truncated";
        case MapValidationReason::TrailingBytes: return "TrailingBytes";
        case MapValidationReason::NameTooLong: return "NameTooLong";
        case MapValidationReason::WidthOutOfRange: return "WidthOutOfRange";
        case MapValidationReason::HeightOutOfRange: return "HeightOutOfRange";
        case MapValidationReason::TileCountMismatch: return "TileCountMismatch";
        case MapValidationReason::UnknownTileFlagBit: return "UnknownTileFlagBit";
        case MapValidationReason::RuntimeOnlyTileFlagAuthored: return "RuntimeOnlyTileFlagAuthored";
        case MapValidationReason::TooFewStartPositions: return "TooFewStartPositions";
        case MapValidationReason::TooManyStartPositions: return "TooManyStartPositions";
        case MapValidationReason::StartPositionOutOfBounds: return "StartPositionOutOfBounds";
        case MapValidationReason::StartPositionNotPassable: return "StartPositionNotPassable";
        case MapValidationReason::DuplicateStartPosition: return "DuplicateStartPosition";
        case MapValidationReason::ResourceNodeOutOfBounds: return "ResourceNodeOutOfBounds";
        case MapValidationReason::ResourceNodeAmountNotPositive: return "ResourceNodeAmountNotPositive";
        case MapValidationReason::ResourceNodeInvalidContent: return "ResourceNodeInvalidContent";
        case MapValidationReason::ResourceNodeNotOnPassableTile: return "ResourceNodeNotOnPassableTile";
        case MapValidationReason::DuplicateResourceNode: return "DuplicateResourceNode";
    }
    return "Unknown";
}

std::string DescribeMapValidation(const MapValidationResult& Result)
{
    std::string Out = ToString(Result.Reason);
    if (Result.Reason == MapValidationReason::Valid)
    {
        return Out;
    }
    if (Result.Index >= 0)
    {
        Out += " at index " + std::to_string(Result.Index);
    }
    if (Result.Tile.X >= 0 && Result.Tile.Y >= 0)
    {
        Out += " on tile (" + std::to_string(Result.Tile.X) + "," + std::to_string(Result.Tile.Y) + ")";
    }
    if (Result.Value != 0)
    {
        Out += " value " + std::to_string(Result.Value);
    }
    return Out;
}

// ---------------------------------------------------------------------------
// Coordinate conversion
// ---------------------------------------------------------------------------

bool StartPositionToTile(const Vec2& Position, TileCoord& OutTile)
{
    const int64_t RawX = Position.X.ToIntFloor();
    const int64_t RawY = Position.Y.ToIntFloor();
    if (RawX < 0 || RawY < 0)
    {
        return false;
    }
    const int64_t TileX = RawX / MapDescription::kTileSizeUnitsLocal;
    const int64_t TileY = RawY / MapDescription::kTileSizeUnitsLocal;
    // A position beyond int32 range cannot index the grid; report it as a failed
    // conversion so the caller emits StartPositionOutOfBounds rather than wrapping.
    if (TileX > int64_t(kMaxMapTiles) || TileY > int64_t(kMaxMapTiles))
    {
        return false;
    }
    OutTile = TileCoord(static_cast<int32_t>(TileX), static_cast<int32_t>(TileY));
    return true;
}

Vec2 TileCenterPosition(const TileCoord& Tile)
{
    // Routed through MapDescription so the authored point and the simulation's own
    // idea of a tile centre cannot drift. The instance is only a coordinate helper --
    // TileCenterToWorld reads no member state.
    const MapDescription Conv;
    return Conv.TileCenterToWorld(Tile);
}

// ---------------------------------------------------------------------------
// Using a map
// ---------------------------------------------------------------------------

int32_t ApplyMapToMatchSetup(const MapFile& File, MatchSetup& Setup)
{
    // Whole-struct assignment: MapDescription is exactly what SimWorld::Initialize
    // copies into SimWorld::Map, so assigning it wholesale means a field added to
    // MapDescription is carried automatically instead of being silently dropped by a
    // field-by-field copy that nobody remembered to extend.
    Setup.Map = File.Map;
    return static_cast<int32_t>(Setup.Map.StartPositions.size());
}

int32_t SpawnMapResourceNodes(const MapFile& File, SimWorld& World)
{
    // Index order, not iterator order over any associative container: two runs must
    // allocate the same EntityId to the same ore field or the match desyncs.
    int32_t Spawned = 0;
    const size_t Count = File.ResourceNodes.size();
    for (size_t I = 0; I < Count; ++I)
    {
        const MapResourceNode& Node = File.ResourceNodes[I];
        // SpawnResourceNode sets Tile_Resource itself and returns Invalid() when the
        // def is unknown or the pool is full. Counting successes rather than asserting
        // lets a caller report "3 of 18 ore fields failed" instead of dying, which is
        // the difference between a diagnosable content mismatch and a crash.
        if (World.SpawnResourceNode(Node.Def, Node.Tile, Node.Amount).IsValid())
        {
            ++Spawned;
        }
    }
    return Spawned;
}

// ---------------------------------------------------------------------------
// Authored maps
// ---------------------------------------------------------------------------

MapFile MakeSymmetricDuelMap(int32_t Size, ContentId OreDef, int32_t OreAmountPerNode)
{
    MapFile File;
    File.Version = kMapFileVersion;
    File.Map.Name = "duel.symmetric";

    // A size that is not a multiple of 8 cannot place both bases at exact reflections
    // of each other, and a silently rounded map would be asymmetric in a way no test
    // asserts. Returned unresized so the caller's ValidateMap reports
    // WidthOutOfRange rather than this function inventing a size.
    if (Size < kMinMapTilesPerSide || Size > kMaxMapTiles || (Size % 8) != 0)
    {
        return File;
    }

    // Open ground everywhere, then features are cut in symmetric pairs below. Note
    // the fill is Tile_GroundPassable alone: no Tile_Occupied and no Tile_Resource,
    // because those are runtime-owned (kRuntimeOwnedTileFlags) and authoring either
    // one is a validation failure, not a shortcut.
    File.Map.Resize(Size, Size, static_cast<uint8_t>(Tile_GroundPassable));

    const int32_t Base = Size / 8;              // 12 on 96x96
    const int32_t Mirror = Size - Base;         // 84 on 96x96

    // Point reflection through the centre: (x,y) -> (W-x, H-y). This is the exact
    // convention AICommander::NextScoutWaypoint assumes, so the reflected base lands
    // on the tile the AI scouts first.
    const auto Reflect = [Size](const TileCoord& T) {
        return TileCoord(Size - T.X, Size - T.Y);
    };

    // Cliffs along the anti-diagonal shoulders, in reflected pairs, so the direct
    // route between bases is a corridor rather than open field -- and so the map
    // exercises a non-passable flag instead of being a featureless plain that would
    // let an impassability bug pass validation unnoticed.
    //
    // Each cliff is placed only if BOTH it and its reflection are safely clear of
    // either base, so terrain can never wall a spawn in. Reflection is applied to
    // every feature, never assumed to be safe.
    const int32_t Quarter = Size / 4;
    for (int32_t Step = 0; Step < Quarter; ++Step)
    {
        const TileCoord Cliff(Quarter + Step, Size - Quarter - Step);
        const TileCoord Pair = Reflect(Cliff);
        if (!File.Map.IsInBounds(Cliff.X, Cliff.Y) || !File.Map.IsInBounds(Pair.X, Pair.Y))
        {
            continue;
        }
        // Keep a wide berth around both bases: a cliff adjacent to a spawn is legal by
        // the letter of validation but produces a base that cannot expand.
        const int32_t KeepClear = 6;
        const auto TooCloseToBase = [&](const TileCoord& T) {
            const int32_t D0X = T.X - Base,   D0Y = T.Y - Base;
            const int32_t D1X = T.X - Mirror, D1Y = T.Y - Mirror;
            return (D0X * D0X + D0Y * D0Y) < (KeepClear * KeepClear) ||
                   (D1X * D1X + D1Y * D1Y) < (KeepClear * KeepClear);
        };
        if (TooCloseToBase(Cliff) || TooCloseToBase(Pair))
        {
            continue;
        }
        File.Map.SetTileFlag(Cliff.X, Cliff.Y, static_cast<uint8_t>(Tile_Cliff), true);
        File.Map.SetTileFlag(Cliff.X, Cliff.Y, static_cast<uint8_t>(Tile_GroundPassable), false);
        File.Map.SetTileFlag(Pair.X, Pair.Y, static_cast<uint8_t>(Tile_Cliff), true);
        File.Map.SetTileFlag(Pair.X, Pair.Y, static_cast<uint8_t>(Tile_GroundPassable), false);
    }

    // Start positions, in slot order: index 0 is the near base, index 1 its
    // reflection. MatchSetup::PlayerSlot::StartPositionIndex indexes this vector.
    File.Map.StartPositions.clear();
    File.Map.StartPositions.push_back(TileCenterPosition(TileCoord(Base, Base)));
    File.Map.StartPositions.push_back(TileCenterPosition(TileCoord(Mirror, Mirror)));

    // Ore: a 3x3 field of nine nodes per side, its centre offset from the base along
    // the diagonal toward the map centre. The second field is the point reflection of
    // the first, which is what makes the two haul distances equal by construction --
    // base 0 to field 0 and base 1 to field 1 are the same vector, mirrored.
    const int32_t OreOffset = 5;
    const TileCoord FieldCentre(Base + OreOffset, Base + OreOffset);
    File.ResourceNodes.clear();
    for (int32_t DY = -1; DY <= 1; ++DY)
    {
        for (int32_t DX = -1; DX <= 1; ++DX)
        {
            const TileCoord Near(FieldCentre.X + DX, FieldCentre.Y + DY);
            const TileCoord Far = Reflect(Near);

            // Ore must sit on a tile a harvester can drive onto, which is the same
            // rule ValidateMap enforces. Skipping a blocked cell rather than forcing
            // it passable keeps terrain authority with the grid above; skipping in
            // pairs keeps the two sides equal even when one cell is skipped.
            const bool NearOk = File.Map.IsInBounds(Near.X, Near.Y) &&
                                IsTilePassable(File.Map.GetTile(Near.X, Near.Y));
            const bool FarOk = File.Map.IsInBounds(Far.X, Far.Y) &&
                               IsTilePassable(File.Map.GetTile(Far.X, Far.Y));
            if (!NearOk || !FarOk)
            {
                continue;
            }

            MapResourceNode A;
            A.Tile = Near;
            A.Def = OreDef;
            A.Amount = OreAmountPerNode;
            File.ResourceNodes.push_back(A);

            MapResourceNode B;
            B.Tile = Far;
            B.Def = OreDef;
            B.Amount = OreAmountPerNode;
            File.ResourceNodes.push_back(B);
        }
    }

    return File;
}

// ---------------------------------------------------------------------------
// Validation
// ---------------------------------------------------------------------------

MapValidationResult ValidateMap(const MapDescription& Map, const std::vector<MapResourceNode>& ResourceNodes,
                                const ContentDatabase* Content)
{
    // --- Rule 1: dimensions within sane bounds -----------------------------
    // Ceiling authority: SimConfig.h kMaxMapTiles. Floor: kMinMapTilesPerSide, so
    // that a 1x1 grid cannot satisfy "two spawns on passable tiles" by accident.
    if (Map.Width < kMinMapTilesPerSide || Map.Width > kMaxMapTiles)
    {
        return MakeFailureValue(MapValidationReason::WidthOutOfRange, Map.Width);
    }
    if (Map.Height < kMinMapTilesPerSide || Map.Height > kMaxMapTiles)
    {
        return MakeFailureValue(MapValidationReason::HeightOutOfRange, Map.Height);
    }

    // --- Rule 2: the grid is the size it claims ----------------------------
    // MapDescription::TileIndex indexes Tiles unchecked, so a short array is an
    // out-of-bounds read on the first pathfinding query rather than a bad map.
    const size_t Expected = size_t(Map.Width) * size_t(Map.Height);
    if (Map.Tiles.size() != Expected)
    {
        return MakeFailureValue(MapValidationReason::TileCountMismatch, int64_t(Map.Tiles.size()));
    }

    if (Map.Name.size() > kMaxNameLength)
    {
        return MakeFailureValue(MapValidationReason::NameTooLong, int64_t(Map.Name.size()));
    }

    // --- Rule 3: tile flags are well-formed --------------------------------
    // Checked against the real TileFlags set in SimTypes.h, via kAllTileFlags. An
    // unknown bit means a corrupt file or a newer format, and accepting it would let
    // it reach the flag tests in SimWorld::CanPlaceBuilding as a silent no-op.
    //
    // Row-major index order: deterministic, and it makes the reported coordinate the
    // first offending tile in reading order rather than whichever the compiler
    // happened to reach first.
    for (int32_t Y = 0; Y < Map.Height; ++Y)
    {
        for (int32_t X = 0; X < Map.Width; ++X)
        {
            const uint8_t T = Map.Tiles[Map.TileIndex(X, Y)];
            if ((T & static_cast<uint8_t>(~kAllTileFlags)) != 0)
            {
                return MakeFailureAt(MapValidationReason::UnknownTileFlagBit, TileCoord(X, Y), -1);
            }
            if ((T & kRuntimeOwnedTileFlags) != 0)
            {
                return MakeFailureAt(MapValidationReason::RuntimeOnlyTileFlagAuthored, TileCoord(X, Y), -1);
            }
        }
    }

    // --- Rule 4: at least two distinct start positions ---------------------
    // Authority: the documented contract of URA4MapBaker::ValidateLevelSetup --
    // "at least 2 spawn locations, valid Ore nodes" (RA4MapBaker.h:19-21). Note that
    // the implementation of that function is a stub which assigns
    // "Map validation passed!" and returns true unconditionally, so the header
    // comment is the only surviving statement of the rule; this is the first place it
    // is actually enforced. Ceiling authority: Ids.h kMaxPlayers.
    const int32_t StartCount = int32_t(Map.StartPositions.size());
    if (StartCount < kMinStartPositions)
    {
        return MakeFailureValue(MapValidationReason::TooFewStartPositions, StartCount);
    }
    if (StartCount > int32_t(kMaxPlayers))
    {
        return MakeFailureValue(MapValidationReason::TooManyStartPositions, StartCount);
    }

    // --- Rule 5: start positions are in bounds and standable ---------------
    // Passability predicate authority: SimWorld.cpp's spawn-ring search, which
    // accepts a tile on (Tile_GroundPassable set && Tile_Occupied clear). A spawn in
    // a lake or against a cliff face produces a base that cannot be built from.
    for (int32_t I = 0; I < StartCount; ++I)
    {
        // Defaults to the "no coordinate" sentinel rather than (0,0): a spawn at a
        // negative world position fails the conversion outright, and reporting it as
        // tile (0,0) would point the author at a corner that is probably fine.
        TileCoord Tile(-1, -1);
        if (!StartPositionToTile(Map.StartPositions[size_t(I)], Tile) || !Map.IsInBounds(Tile.X, Tile.Y))
        {
            return MakeFailureAt(MapValidationReason::StartPositionOutOfBounds, Tile, I);
        }
        if (!IsTilePassable(Map.Tiles[Map.TileIndex(Tile.X, Tile.Y)]))
        {
            return MakeFailureAt(MapValidationReason::StartPositionNotPassable, Tile, I);
        }

        // Distinctness is required, not merely desirable: two players spawning on one
        // tile means two construction yards on one footprint, and the second
        // SpawnBuilding either fails or overlaps. Compared by tile rather than by
        // exact Vec2 so that two positions inside the same tile still collide.
        //
        // O(n^2) over at most kMaxPlayers entries -- eight. A set would add a
        // container whose iteration order is not defined for no measurable gain.
        for (int32_t J = 0; J < I; ++J)
        {
            TileCoord Other;
            if (StartPositionToTile(Map.StartPositions[size_t(J)], Other) && Other == Tile)
            {
                return MakeFailureAt(MapValidationReason::DuplicateStartPosition, Tile, I);
            }
        }
    }

    // --- Rule 6: resource nodes are in bounds and harvestable --------------
    // "valid Ore nodes" from the RA4MapBaker.h contract, made specific. A node needs
    // to be reachable and to hold something, and SpawnResourceNode must be able to
    // resolve its Def -- it returns EntityId::Invalid() and silently spawns nothing
    // when FindResourceNode misses, which on a map file would read as an ore field
    // that simply is not there.
    const int32_t NodeCount = int32_t(ResourceNodes.size());
    for (int32_t I = 0; I < NodeCount; ++I)
    {
        const MapResourceNode& Node = ResourceNodes[size_t(I)];
        if (!Map.IsInBounds(Node.Tile.X, Node.Tile.Y))
        {
            return MakeFailureAt(MapValidationReason::ResourceNodeOutOfBounds, Node.Tile, I);
        }
        if (Node.Amount <= 0)
        {
            // SpawnResourceNode falls back to the content default on Amount <= 0, so
            // a zero here would load as "whatever the def says" -- a map that means
            // one thing and plays as another. Authored amounts must be explicit.
            return MakeFailureAt(MapValidationReason::ResourceNodeAmountNotPositive, Node.Tile, I);
        }
        if (!Node.Def.IsValid() || (Content != nullptr && Content->FindResourceNode(Node.Def) == nullptr))
        {
            return MakeFailureAt(MapValidationReason::ResourceNodeInvalidContent, Node.Tile, I);
        }
        if (!IsTilePassable(Map.Tiles[Map.TileIndex(Node.Tile.X, Node.Tile.Y)]))
        {
            // A harvester has to drive onto the tile to work it, so an ore field on
            // water or under a cliff is unharvestable and the credits it represents
            // never enter the match.
            return MakeFailureAt(MapValidationReason::ResourceNodeNotOnPassableTile, Node.Tile, I);
        }
        for (int32_t J = 0; J < I; ++J)
        {
            if (ResourceNodes[size_t(J)].Tile == Node.Tile)
            {
                // Tile_Resource is one bit. Two nodes on a tile means exhausting the
                // first clears the flag while the second still exists, and the tile
                // becomes buildable on top of a live ore node.
                return MakeFailureAt(MapValidationReason::DuplicateResourceNode, Node.Tile, I);
            }
        }
    }

    return MapValidationResult();
}

MapValidationResult ValidateMap(const MapFile& File, const ContentDatabase* Content)
{
    return ValidateMap(File.Map, File.ResourceNodes, Content);
}

// ---------------------------------------------------------------------------
// Serialization
// ---------------------------------------------------------------------------
//
// Field order matches SimWorld::Serialize's map block exactly -- name, width,
// height, tile count, then the tiles -- so that the two payloads stay readable
// against one mental model. Start positions and resource nodes follow, count-prefixed
// in the same style as every other variable-length run in the save.

void MapFile::Serialize(ByteWriter& W) const
{
    W.WriteUInt32(kMapFileMagic);
    W.WriteUInt32(kMapFileVersion);

    W.WriteString(Map.Name);
    W.WriteInt32(Map.Width);
    W.WriteInt32(Map.Height);
    W.WriteUInt32(static_cast<uint32_t>(Map.Tiles.size()));
    for (uint8_t Tile : Map.Tiles)
    {
        W.WriteUInt8(Tile);
    }

    // Fixed-point raw values, never a float: Fixed::Raw is the authoritative
    // representation and writing anything derived from it would make the map's
    // spawn points platform-dependent.
    W.WriteUInt32(static_cast<uint32_t>(Map.StartPositions.size()));
    for (const Vec2& P : Map.StartPositions)
    {
        W.WriteInt64(P.X.Raw);
        W.WriteInt64(P.Y.Raw);
    }

    W.WriteUInt32(static_cast<uint32_t>(ResourceNodes.size()));
    for (const MapResourceNode& Node : ResourceNodes)
    {
        W.WriteInt32(Node.Tile.X);
        W.WriteInt32(Node.Tile.Y);
        W.WriteUInt32(Node.Def.Value);
        W.WriteInt32(Node.Amount);
    }
}

std::vector<uint8_t> SerializeMap(const MapFile& File)
{
    ByteWriter W;
    File.Serialize(W);
    return W.GetBuffer();
}

bool DeserializeMap(ByteReader& R, MapFile& Out, MapValidationResult& OutResult, const ContentDatabase* Content)
{
    if (R.ReadUInt32() != kMapFileMagic)
    {
        OutResult = MakeFailure(MapValidationReason::BadMagic);
        return false;
    }

    // Version rejection mirrors SimWorld::Deserialize: read the version, compare
    // against what this build understands, and refuse anything else before touching
    // the payload. v1 is the only version, so there is no back-compat branch yet --
    // when one is added it belongs here, as an explicit allowance like the save's
    // "Version != kSimSaveVersion && Version != 2".
    const uint32_t Version = R.ReadUInt32();
    if (Version != kMapFileVersion)
    {
        OutResult = MakeFailureValue(MapValidationReason::UnsupportedVersion, Version);
        return false;
    }

    // Parsed into a local, not into Out. A caller that ignores the return value must
    // not end up holding a half-parsed map that looks startable.
    MapFile Parsed;
    Parsed.Version = Version;

    Parsed.Map.Name = R.ReadString();
    Parsed.Map.Width = R.ReadInt32();
    Parsed.Map.Height = R.ReadInt32();

    const uint32_t TileCount = R.ReadUInt32();
    if (R.HasError())
    {
        OutResult = MakeFailure(MapValidationReason::Truncated);
        return false;
    }
    // Sanity-check the count against the dimensions BEFORE resizing. An attacker-
    // supplied 4 GB tile count would otherwise be a single allocation away from
    // taking the process down, and a dedicated server must not be killable by a
    // malformed map upload.
    if (Parsed.Map.Width < kMinMapTilesPerSide || Parsed.Map.Width > kMaxMapTiles)
    {
        OutResult = MakeFailureValue(MapValidationReason::WidthOutOfRange, Parsed.Map.Width);
        return false;
    }
    if (Parsed.Map.Height < kMinMapTilesPerSide || Parsed.Map.Height > kMaxMapTiles)
    {
        OutResult = MakeFailureValue(MapValidationReason::HeightOutOfRange, Parsed.Map.Height);
        return false;
    }
    if (size_t(TileCount) != size_t(Parsed.Map.Width) * size_t(Parsed.Map.Height))
    {
        OutResult = MakeFailureValue(MapValidationReason::TileCountMismatch, int64_t(TileCount));
        return false;
    }
    if (R.Remaining() < size_t(TileCount))
    {
        OutResult = MakeFailure(MapValidationReason::Truncated);
        return false;
    }
    Parsed.Map.Tiles.resize(TileCount);
    for (uint32_t I = 0; I < TileCount; ++I)
    {
        Parsed.Map.Tiles[I] = R.ReadUInt8();
    }

    const uint32_t StartCount = R.ReadUInt32();
    if (R.HasError())
    {
        OutResult = MakeFailure(MapValidationReason::Truncated);
        return false;
    }
    // Bounded before reserving, for the same reason as the tile count. The real
    // "too many spawns" rule still runs in ValidateMap and produces the specific
    // reason; this only refuses a count too large to be worth allocating for.
    if (StartCount > uint32_t(kMaxPlayers))
    {
        OutResult = MakeFailureValue(MapValidationReason::TooManyStartPositions, StartCount);
        return false;
    }
    Parsed.Map.StartPositions.reserve(StartCount);
    for (uint32_t I = 0; I < StartCount; ++I)
    {
        const int64_t X = R.ReadInt64();
        const int64_t Y = R.ReadInt64();
        Parsed.Map.StartPositions.push_back(Vec2(Fixed::FromRaw(X), Fixed::FromRaw(Y)));
    }
    if (R.HasError())
    {
        OutResult = MakeFailure(MapValidationReason::Truncated);
        return false;
    }

    const uint32_t NodeCount = R.ReadUInt32();
    if (R.HasError())
    {
        OutResult = MakeFailure(MapValidationReason::Truncated);
        return false;
    }
    // Each node is 16 bytes on the wire. Checking the remaining length against the
    // claimed count refuses an inflated count without allocating for it first.
    constexpr size_t kNodeBytes = 16;
    if (R.Remaining() < size_t(NodeCount) * kNodeBytes)
    {
        OutResult = MakeFailure(MapValidationReason::Truncated);
        return false;
    }
    Parsed.ResourceNodes.reserve(NodeCount);
    for (uint32_t I = 0; I < NodeCount; ++I)
    {
        MapResourceNode Node;
        Node.Tile.X = R.ReadInt32();
        Node.Tile.Y = R.ReadInt32();
        Node.Def = ContentId(R.ReadUInt32());
        Node.Amount = R.ReadInt32();
        Parsed.ResourceNodes.push_back(Node);
    }

    if (R.HasError())
    {
        OutResult = MakeFailure(MapValidationReason::Truncated);
        return false;
    }
    if (R.Remaining() != 0)
    {
        // The payload ended before the file did. Either the file was written by a
        // format that carries more and lied about its version, or it is corrupt.
        // Either way it is not a v1 map, and guessing is how a silent misread starts.
        OutResult = MakeFailureValue(MapValidationReason::TrailingBytes, int64_t(R.Remaining()));
        return false;
    }

    // Framing is sound; now the map itself has to be playable.
    const MapValidationResult Validation = ValidateMap(Parsed.Map, Parsed.ResourceNodes, Content);
    if (!Validation.IsValid())
    {
        OutResult = Validation;
        return false;
    }

    Out = Parsed;
    OutResult = MapValidationResult();
    return true;
}

bool DeserializeMap(const std::vector<uint8_t>& Bytes, MapFile& Out, MapValidationResult& OutResult,
                    const ContentDatabase* Content)
{
    ByteReader R(Bytes);
    return DeserializeMap(R, Out, OutResult, Content);
}

// ---------------------------------------------------------------------------
// File access
// ---------------------------------------------------------------------------
//
// std::fopen / fread / fwrite, matching ReplayRecorder::SaveToFile and
// LoadReplayFromFile in RA4Replay/Private/Replay.cpp. No Unreal file API, so map
// validation runs in the headless build and therefore in CI.

bool LoadMapFromFile(const std::string& Path, MapFile& Out, MapValidationResult& OutResult,
                     const ContentDatabase* Content)
{
    std::FILE* File = std::fopen(Path.c_str(), "rb");
    if (File == nullptr)
    {
        OutResult = MakeFailure(MapValidationReason::Truncated);
        return false;
    }
    std::fseek(File, 0, SEEK_END);
    const long Size = std::ftell(File);
    std::fseek(File, 0, SEEK_SET);
    if (Size <= 0)
    {
        std::fclose(File);
        OutResult = MakeFailure(MapValidationReason::Truncated);
        return false;
    }
    std::vector<uint8_t> Bytes(static_cast<size_t>(Size));
    const size_t Read = std::fread(Bytes.data(), 1, Bytes.size(), File);
    std::fclose(File);
    if (Read != Bytes.size())
    {
        OutResult = MakeFailure(MapValidationReason::Truncated);
        return false;
    }
    return DeserializeMap(Bytes, Out, OutResult, Content);
}

bool SaveMapToFile(const std::string& Path, const MapFile& File, MapValidationResult& OutResult,
                   const ContentDatabase* Content)
{
    // Validate before writing. Writing an invalid map would put a file in the content
    // tree that LoadMapFromFile refuses, which turns an authoring mistake into a
    // mystery at match start instead of an error at save time.
    OutResult = ValidateMap(File.Map, File.ResourceNodes, Content);
    if (!OutResult.IsValid())
    {
        return false;
    }

    const std::vector<uint8_t> Bytes = SerializeMap(File);
    std::FILE* Handle = std::fopen(Path.c_str(), "wb");
    if (Handle == nullptr)
    {
        return false;
    }
    const size_t Written = std::fwrite(Bytes.data(), 1, Bytes.size(), Handle);
    std::fclose(Handle);
    return Written == Bytes.size();
}

} // namespace RA4
