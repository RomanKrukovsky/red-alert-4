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
const ContentId SovietYard = MakeContentId("building.sov.construction_yard");
const ContentId AllianceYard = MakeContentId("building.all.construction_yard");
const ContentId OreField = MakeContentId("resource.ore_field");

void SeedBase(SimWorld& World, ContentId Yard, PlayerId Owner, const TileCoord& YardTile,
              const TileCoord& OreOrigin)
{
    World.SpawnBuilding(Yard, Owner, YardTile, /*bInstantComplete*/ true);
    for (int32 X = 0; X < 3; ++X)
    {
        for (int32 Y = 0; Y < 3; ++Y)
        {
            World.SpawnResourceNode(OreField, TileCoord(OreOrigin.X + X, OreOrigin.Y + Y), 3000);
        }
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

    SeedBase(World, SovietYard, 0, TileCoord(10, 10), TileCoord(6, 15));
    SeedBase(World, AllianceYard, 1, TileCoord(48, 48), TileCoord(53, 43));
    World.ClearEvents();
}
