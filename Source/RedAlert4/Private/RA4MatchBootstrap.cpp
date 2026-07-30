// Copyright (c) Red Alert 4 project.
#include "RA4MatchBootstrap.h"

#include "RA4Content/ContentDatabase.h"
#include "RA4Core/SimConfig.h"
#include "RA4Simulation/SimWorld.h"

using namespace RA4;

namespace
{
constexpr int32 kMapTiles = 64;

// Mirrors the ids in RA4Content/DefaultContent.cpp. Resolved by name so that a
// content rename fails loudly here instead of silently seeding an empty base.
const ContentId OreField = MakeContentId("resource.ore_field");

// One faction's opening loadout. Grouped in a struct so both sides are seeded by the
// same code and cannot drift apart -- an asymmetric start is a bug that only shows up
// as "the AI always wins".
struct StartingForce
{
    ContentId Yard;
    ContentId Refinery;
    ContentId Harvester;
    ContentId Infantry;
    ContentId Tank;
};

const StartingForce SovietForce{
    MakeContentId("building.sov.construction_yard"), MakeContentId("building.sov.ore_refinery"),
    MakeContentId("unit.sov.ore_harvester"), MakeContentId("unit.sov.conscript"),
    MakeContentId("unit.sov.heavy_tank")};

const StartingForce AllianceForce{
    MakeContentId("building.all.construction_yard"), MakeContentId("building.all.ore_refinery"),
    MakeContentId("unit.all.ore_harvester"), MakeContentId("unit.all.rifleman"),
    MakeContentId("unit.all.light_tank")};

Vec2 TileCentre(const TileCoord& Tile)
{
    return Vec2(Fixed::FromInt(int64(Tile.X) * kTileSizeUnits + kTileSizeUnits / 2),
                Fixed::FromInt(int64(Tile.Y) * kTileSizeUnits + kTileSizeUnits / 2));
}

// A construction yard on an empty field is not a playable start: with nothing to
// select there is no selection, no orders and nothing for the sidebar to attach to.
// Both sides open with the classic skirmish kit -- yard, refinery, a harvester on the
// ore and a small escort.
void SeedBase(SimWorld& World, const StartingForce& Force, PlayerId Owner, const TileCoord& YardTile,
              const TileCoord& OreOrigin)
{
    World.SpawnBuilding(Force.Yard, Owner, YardTile, /*bInstantComplete*/ true);
    World.SpawnBuilding(Force.Refinery, Owner, TileCoord(YardTile.X + 4, YardTile.Y), true);

    for (int32 X = 0; X < 3; ++X)
    {
        for (int32 Y = 0; Y < 3; ++Y)
        {
            World.SpawnResourceNode(OreField, TileCoord(OreOrigin.X + X, OreOrigin.Y + Y), 3000);
        }
    }

    World.SpawnUnit(Force.Harvester, Owner, TileCentre(TileCoord(YardTile.X + 4, YardTile.Y + 2)));

    // Spread along a row rather than stacked on one tile, so the opening screen shows
    // a formation and the movement systems are not asked to untangle an overlap on
    // the first tick.
    for (int32 Index = 0; Index < 4; ++Index)
    {
        World.SpawnUnit(Force.Infantry, Owner, TileCentre(TileCoord(YardTile.X - 2 + Index, YardTile.Y + 3)));
    }
    for (int32 Index = 0; Index < 2; ++Index)
    {
        World.SpawnUnit(Force.Tank, Owner, TileCentre(TileCoord(YardTile.X - 2 + Index * 3, YardTile.Y + 5)));
    }
}
} // namespace

void FRA4MatchBootstrap::BuildSkirmish(ContentDatabase& Content, SimWorld& World, uint64 Seed)
{
    BuildDefaultContent(Content);

    // Authoring mistakes in content must not reach a running match; they are far
    // cheaper to diagnose here than as a crash twenty minutes in.
    std::vector<std::string> Errors;
    if (!Content.Validate(Errors))
    {
        for (const std::string& Error : Errors)
        {
            UE_LOG(LogTemp, Error, TEXT("RA4 content validation: %s"), UTF8_TO_TCHAR(Error.c_str()));
        }
    }

    MatchSetup Setup;
    Setup.Seed = Seed;
    Setup.Map.Name = "skirmish.plains";
    Setup.Map.Resize(kMapTiles, kMapTiles, Tile_GroundPassable);

    Setup.Players[0].bActive = true;
    Setup.Players[0].Faction = FactionId::Soviet;
    Setup.Players[0].StartingCredits = 10000;

    Setup.Players[1].bActive = true;
    Setup.Players[1].Faction = FactionId::Alliance;
    Setup.Players[1].StartingCredits = 10000;

    World.Initialize(&Content, Setup);

    SeedBase(World, SovietForce, 0, TileCoord(10, 10), TileCoord(6, 15));
    SeedBase(World, AllianceForce, 1, TileCoord(48, 48), TileCoord(53, 43));
    World.ClearEvents();
}
