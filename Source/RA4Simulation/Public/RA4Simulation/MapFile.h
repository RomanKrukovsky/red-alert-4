// Copyright (c) Red Alert 4 project. Standalone map format: load, validate, save.
//
// Until now a playable world could only be produced by code: the tile grid was
// generated procedurally and the opening base was hardcoded (RA4MatchBootstrap.cpp
// SeedBase, and the Unity MatchRunner equivalent). Adding a map therefore meant
// editing and recompiling the game. This file closes that: a map is data, and
// LoadMapFromFile plus ValidateMap are the only two things the engine needs to
// turn that data into a startable match.
//
// The format carries what a match cannot infer:
//   * the tile grid                (terrain)
//   * the start positions          (who spawns where)
//   * the resource node placements (what there is to harvest)
//
// It deliberately does NOT carry the starting force. Which yard and which escort a
// faction opens with is content, keyed by FactionId, and duplicating it per map
// would let a map contradict the faction it is played with.
//
// Engine-free and deterministic by construction: no Unreal type, no UObject, no
// FString, no float, no wall-clock, and every container walk is in index order. The
// same sources compile in Tools/HeadlessBuild, which is what lets map validation run
// in CI without the editor -- the existing URA4MapBaker::ValidateLevelSetup cannot,
// because it is a UObject.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "RA4Core/ByteStream.h"
#include "RA4Core/Ids.h"
#include "RA4Core/Vector.h"
#include "RA4Simulation/SimTypes.h"

#ifndef RA4SIMULATION_API
#define RA4SIMULATION_API
#endif

namespace RA4
{

// Declared rather than included: only the resource-node content check touches the
// database, and pulling ContentDatabase.h in here would put <unordered_map> on every
// consumer of the map format for one optional lookup.
class ContentDatabase;

// Declared rather than included for the same reason, and for a second one: including
// SimWorld.h here would make the map format depend on the whole simulation, and the
// format has to stay loadable (and CI-checkable) without it. Only SpawnMapResourceNodes
// touches a world, and it is defined in MapFile.cpp where SimWorld.h is available.
class SimWorld;
struct MatchSetup;

// Magic and version follow the save convention in SimWorld.cpp (kSimSaveMagic /
// kSimSaveVersion): a four-character tag, then an explicit integer version, then
// fields. Bump the version on ANY change to the field order or meaning below; the
// loader rejects an unrecognised version up front rather than misreading it.
constexpr uint32_t kMapFileMagic = 0x5241344Du;   // "RA4M"
constexpr uint32_t kMapFileVersion = 1;           // v1: tiles, start positions, resource nodes

// A map may not exceed the simulation's hard ceiling on grid size. Authority:
// RA4Core/SimConfig.h kMaxMapTiles, whose comment states that exceeding it "is a
// content or map authoring error, not a runtime condition". That is exactly this
// check's job, moved from an assert to a rejection with a reason -- a malformed map
// file is untrusted input and must not be able to abort the process.
constexpr int32_t kMinMapTilesPerSide = 16;

// Start positions are player slots, so the ceiling is the player count. Authority:
// RA4Core/Ids.h kMaxPlayers.
constexpr int32_t kMinStartPositions = 2;

/** One authored ore/gem field cell.

    Amount is authored per cell rather than per field: a field that thins out toward
    its edge is the standard RTS layout, and a single per-field number cannot express
    it. Def selects which resource content this is (ore, gems, ...) so a map can mix
    them without a code change.

    Tile_Resource is deliberately NOT authored in the tile grid. SimWorld sets that
    flag when the node entity is spawned (SimWorld.cpp SpawnResourceNode) and clears
    it when the field is exhausted, so a map file asserting it would go stale the
    moment a harvester finished the patch. The nodes listed here are the authority;
    the flag is derived. */
struct MapResourceNode
{
    TileCoord Tile;
    ContentId Def;
    int32_t Amount = 0;
};

/** A map as it exists on disk: the simulation's MapDescription plus the two things
    MapDescription has no room for -- the resource fields and the format version the
    payload was written at.

    MapDescription is reused rather than mirrored so that loading a map is an
    assignment into SimWorld::Map, not a translation step that could drift from it. */
struct MapFile
{
    uint32_t Version = kMapFileVersion;
    MapDescription Map;
    std::vector<MapResourceNode> ResourceNodes;

    void Serialize(ByteWriter& W) const;
};

/** Why a map was refused.

    A bare bool was not enough. The existing placement validator in the simulation
    (SimWorld::CanPlaceBuilding) returns one, and the cost of that shows up the first
    time a map fails to load: there is nothing to tell the author whether the grid is
    the wrong size, a spawn is in a lake, or the file is simply truncated. Every
    rejection below names the rule that fired, and MapValidationResult carries the
    coordinate or index that tripped it. */
enum class MapValidationReason : uint8_t
{
    Valid = 0,

    // --- container / framing ---
    BadMagic,                    // not a map file at all
    UnsupportedVersion,          // written by a different build of the format
    Truncated,                   // stream ran out mid-payload
    TrailingBytes,               // payload ended early; the file says more than v1 holds
    NameTooLong,                 // exceeds what ByteWriter::WriteString can round-trip

    // --- dimensions ---
    WidthOutOfRange,
    HeightOutOfRange,
    TileCountMismatch,           // Tiles.size() != Width * Height

    // --- tile grid ---
    UnknownTileFlagBit,          // a bit outside the TileFlags enum is set
    RuntimeOnlyTileFlagAuthored, // Tile_Occupied / Tile_Resource are set by the sim, not authored

    // --- start positions ---
    TooFewStartPositions,
    TooManyStartPositions,
    StartPositionOutOfBounds,
    StartPositionNotPassable,
    DuplicateStartPosition,      // two players sharing one tile is not a start

    // --- resource nodes ---
    ResourceNodeOutOfBounds,
    ResourceNodeAmountNotPositive,
    ResourceNodeInvalidContent,
    ResourceNodeNotOnPassableTile,
    DuplicateResourceNode,       // two nodes on one tile: the tile flag cannot represent it
};

/** The outcome of a validation pass, with enough context to fix the map.

    Only the first failure is reported. Validation is cheap to re-run, and a map with
    a 40x40 grid where every tile has a bad flag bit would otherwise produce sixteen
    hundred identical complaints that say nothing the first one did not. */
struct MapValidationResult
{
    MapValidationReason Reason = MapValidationReason::Valid;

    // Set when the failure is tied to a place in the grid; {-1,-1} when it is not.
    TileCoord Tile = TileCoord(-1, -1);
    // Set when the failure is tied to an entry in StartPositions or ResourceNodes;
    // -1 when it is not.
    int32_t Index = -1;
    // The offending measurement, for the range and count failures (the width that
    // was too large, the number of start positions that was too small, ...).
    int64_t Value = 0;

    bool IsValid() const { return Reason == MapValidationReason::Valid; }
};

/** Human-readable name of a rejection reason, for logs and editor messages.

    Mirrors ToString(CommandReject) in RA4Core/Command.h: the enumerator spelling,
    not a sentence, so that log output greps cleanly. */
RA4SIMULATION_API const char* ToString(MapValidationReason Reason);

/** One sentence explaining a result, including its coordinate or index.

    Separate from ToString so that callers that only want the tag are not forced to
    allocate a string. */
RA4SIMULATION_API std::string DescribeMapValidation(const MapValidationResult& Result);

/** Checks a map against the rules a match cannot start without.

    Runs on any MapDescription plus node list, whether it came from a file, an editor
    bake or a test fixture -- so the file loader and the editor share one definition
    of "valid" instead of drifting apart.

    Content may be null. When supplied, resource node Def values are checked against
    the loaded content database, which catches a map referring to a resource type this
    build does not have; when null that one rule is skipped and the rest still run. */
RA4SIMULATION_API MapValidationResult ValidateMap(const MapDescription& Map,
                                                 const std::vector<MapResourceNode>& ResourceNodes,
                                                 const ContentDatabase* Content = nullptr);

/** Convenience overload for an already-assembled MapFile. */
RA4SIMULATION_API MapValidationResult ValidateMap(const MapFile& File, const ContentDatabase* Content = nullptr);

/** Reads a map from bytes and validates it.

    Returns false and fills OutResult on wrong magic, an unsupported version, a
    truncated payload, or any rule in ValidateMap. Out is left untouched on failure,
    so a caller cannot accidentally start a match on a half-parsed map. */
RA4SIMULATION_API bool DeserializeMap(const std::vector<uint8_t>& Bytes, MapFile& Out,
                                     MapValidationResult& OutResult, const ContentDatabase* Content = nullptr);

/** Reads a map from a ByteReader positioned at the magic. */
RA4SIMULATION_API bool DeserializeMap(ByteReader& R, MapFile& Out, MapValidationResult& OutResult,
                                      const ContentDatabase* Content = nullptr);

/** Serializes a validated map to bytes. */
RA4SIMULATION_API std::vector<uint8_t> SerializeMap(const MapFile& File);

// --- File access -----------------------------------------------------------
//
// I/O policy follows the precedent already set by RA4Replay (Replay.h
// SaveToFile / LoadReplayFromFile, implemented with std::fopen/fread in
// Replay.cpp): the core does its own reads through <cstdio>, as free functions
// taking a std::string path and reporting failure by return value rather than by
// throwing. No new I/O policy is invented here, and no Unreal file API is reached
// for, because that would make the format uncheckable from the headless build.

/** Loads and validates a map file. OutResult distinguishes "cannot read the file"
    (Truncated) from "the map is wrong" (a specific rule). */
RA4SIMULATION_API bool LoadMapFromFile(const std::string& Path, MapFile& Out, MapValidationResult& OutResult,
                                       const ContentDatabase* Content = nullptr);

/** Writes a map file. Refuses to write an invalid map: a map that cannot be loaded
    is not worth the bytes, and catching it here keeps broken maps out of the
    content tree instead of out of a match. */
RA4SIMULATION_API bool SaveMapToFile(const std::string& Path, const MapFile& File, MapValidationResult& OutResult,
                                     const ContentDatabase* Content = nullptr);

// --- Helpers ---------------------------------------------------------------

/** Every bit the TileFlags enum defines. Any other bit set in an authored tile is a
    corrupt or future-format grid, not a terrain feature. Kept next to the format so
    that adding a flag to SimTypes.h and forgetting the format shows up as a compile
    site to revisit rather than as a silently accepted unknown bit. */
constexpr uint8_t kAllTileFlags = static_cast<uint8_t>(Tile_GroundPassable | Tile_Water | Tile_Cliff |
                                                       Tile_Occupied | Tile_Resource | Tile_Bridge);

/** Flags the simulation owns at runtime and a map file must not assert.

    Tile_Occupied is set by SimWorld::OccupyTiles when a building footprint lands,
    and Tile_Resource by SpawnResourceNode. Authoring either would make the grid
    disagree with the entities within one tick. */
constexpr uint8_t kRuntimeOwnedTileFlags = static_cast<uint8_t>(Tile_Occupied | Tile_Resource);

/** Whether a tile can be stood on, using the same predicate as the simulation.

    Authority: SimWorld.cpp's spawn-ring search, which accepts a tile when
    Tile_GroundPassable is set and Tile_Occupied is clear. A bridge deck is passable
    over water, so water alone is not disqualifying -- the flag combination is what
    decides, not the terrain type. */
inline bool IsTilePassable(uint8_t TileValue)
{
    return (TileValue & static_cast<uint8_t>(Tile_GroundPassable)) != 0 &&
           (TileValue & static_cast<uint8_t>(Tile_Occupied)) == 0;
}

/** Converts a start position to its tile without MapDescription::WorldToTile's
    negative-coordinate quirk.

    WorldToTile divides ToIntFloor() by the tile size, and C++ integer division
    truncates toward zero, so a position at world -50 lands on tile 0 rather than
    tile -1 -- an out-of-bounds spawn that reads as in-bounds. Validation must not
    inherit that, so negatives are reported here instead of being folded away.
    Returns false when the position is negative on either axis. */
RA4SIMULATION_API bool StartPositionToTile(const Vec2& Position, TileCoord& OutTile);

/** World-space centre of a tile, for authoring a start position from tile coordinates.

    Delegates to MapDescription::TileCenterToWorld, so an authored spawn lands on the
    same point the simulation would compute for that tile rather than on its corner --
    a corner position is ambiguous between four tiles once it round-trips. */
RA4SIMULATION_API Vec2 TileCenterPosition(const TileCoord& Tile);

// --- Using a map -----------------------------------------------------------
//
// A validated MapFile is not yet a match. Two things still have to happen: the grid
// and spawn points have to reach MatchSetup (which SimWorld::Initialize copies into
// SimWorld::Map), and the resource nodes have to become entities (which only
// SimWorld::SpawnResourceNode can do, because it allocates the entity AND sets
// Tile_Resource). The two functions below are that bridge, and they are the reason
// this format is usable rather than scaffolding.
//
// The order is not interchangeable. Nodes must be spawned AFTER Initialize, because
// Initialize calls Reset() -- spawning first would discard the entities. The full
// sequence a caller runs is documented on ApplyMapToMatchSetup.

/** Copies a map's grid, name and start positions into a MatchSetup.

    Does NOT touch MatchSetup::Players: which faction sits in which slot and what it
    starts with is lobby state, not map data, and a map that could overwrite it could
    contradict the match it is played in. The caller fills the slots; this fills the
    terrain. Players[i].StartPositionIndex indexes Map.StartPositions, so a slot is
    matched to a spawn by index -- the same convention MissionRuntime.cpp:280 uses.

    Assigns rather than merges, so calling it twice is idempotent.

    Full sequence to start a match from a map file:
        MapFile File; MapValidationResult Result;
        if (!LoadMapFromFile(Path, File, Result, &Content)) { report Result; return; }
        MatchSetup Setup;
        Setup.Seed = Seed;
        ApplyMapToMatchSetup(File, Setup);            // terrain + spawns
        Setup.Players[0].bActive = true;              // lobby state, caller's job
        Setup.Players[0].Faction = FactionId::Soviet;
        Setup.Players[0].StartPositionIndex = 0;
        Setup.Players[1].bActive = true;
        Setup.Players[1].Faction = FactionId::Alliance;
        Setup.Players[1].StartPositionIndex = 1;
        World.Initialize(&Content, Setup, ReconSettings);
        SpawnMapResourceNodes(File, World);           // ore, AFTER Initialize

    Returns the number of start positions made available, so a caller can refuse to
    seat more players than the map has spawns for. */
RA4SIMULATION_API int32_t ApplyMapToMatchSetup(const MapFile& File, MatchSetup& Setup);

/** Spawns a map's resource nodes into an initialized world.

    Must be called AFTER SimWorld::Initialize: Initialize begins with Reset(), which
    clears the entity arrays, so nodes spawned before it would vanish. Uses
    SimWorld::SpawnResourceNode rather than writing the arrays directly, so the node
    entity and the Tile_Resource flag are set by the one function that owns both --
    which is also why the map file does not author that flag.

    Returns the number of nodes that produced an entity. A count lower than
    File.ResourceNodes.size() means SpawnResourceNode refused some: either the content
    database has no such resource def, or the entity pool is exhausted. Validating
    with a non-null ContentDatabase first rules out the former. */
RA4SIMULATION_API int32_t SpawnMapResourceNodes(const MapFile& File, SimWorld& World);

// --- Authored maps ---------------------------------------------------------

/** Builds the stock symmetric 1v1 map as data.

    A format with no instance is untested, so this is a real map rather than an
    example: MakeSymmetricDuelMap(96) is the map the headless tools and the AI can
    actually play, and it is generated rather than checked in as bytes so that a
    change to the tile flags cannot leave a stale binary blob behind.

    Symmetry is point reflection through the centre, (x,y) -> (W-x, H-y), applied to
    terrain and to ore alike. That specific convention is load-bearing, not cosmetic:
    AICommander::NextScoutWaypoint (AICommander.cpp:544) sends its first scout to
    (W - W/8, H - H/8) with the comment "in a symmetric skirmish that is where the
    opponent starts". Bases therefore sit at (W/8, H/8) and (W - W/8, H - H/8) -- on
    96x96, tiles (12,12) and (84,84) -- so the AI scouts the enemy base instead of
    empty ground.

    Ore is dealt symmetrically under the same reflection, so both players' haul
    distance from base to nearest field is equal by construction rather than by
    eyeball. Per-node amount is the ore def's own declared InitialAmount (8000 in
    DefaultContent.cpp:755); exceeding a content-declared maximum would make the map
    disagree with the content it is played with.

    Size must be a multiple of 8 so that W/8 is exact and the two bases are truly
    reflections; returns a map that fails ValidateMap otherwise, never a silently
    rounded one. The result is validated by the caller like any other map -- it is not
    exempt, and MapFileSelfTest exercises exactly that. */
RA4SIMULATION_API MapFile MakeSymmetricDuelMap(int32_t Size = 96,
                                              ContentId OreDef = MakeContentId("resource.ore_field"),
                                              int32_t OreAmountPerNode = 8000);

} // namespace RA4
