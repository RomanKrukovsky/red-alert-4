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
    return Vec2(RA4::Fixed::FromInt(int64(Tile.X) * kTileSizeUnits + kTileSizeUnits / 2),
                RA4::Fixed::FromInt(int64(Tile.Y) * kTileSizeUnits + kTileSizeUnits / 2));
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

void FRA4MatchBootstrap::BuildSkirmish(ContentDatabase& Content, SimWorld& World, uint64 Seed,
                                       RA4::FactionId PlayerFaction, RA4::FactionId EnemyFaction,
                                       int32 NumAIPlayers, int32 AISpot,
                                       const RA4::Recon::ReconSettings* ReconSettings)
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
    Setup.Players[0].Faction = PlayerFaction;
    Setup.Players[0].StartingCredits = 10000;

    // The map's start spots, in spots-combobox order (Zapad = spot 1, Vostok =
    // spot 2, then the remaining corners). Yard tile plus the ore field that
    // belongs to it.
    struct StartSpot
    {
        TileCoord Yard;
        TileCoord Ore;
    };
    static const StartSpot Spots[4] = {
        {TileCoord(10, 10), TileCoord(6, 15)},   // spot 1: north-west
        {TileCoord(48, 48), TileCoord(53, 43)},  // spot 2: south-east
        {TileCoord(48, 10), TileCoord(53, 6)},   // spot 3: north-east
        {TileCoord(10, 48), TileCoord(6, 43)},   // spot 4: south-west
    };
    constexpr int32 kSpotCount = int32(sizeof(Spots) / sizeof(Spots[0]));

    NumAIPlayers = NumAIPlayers < 1 ? 1 : NumAIPlayers;
    const int32 MaxAIPlayers = kMaxPlayers - 1 < kSpotCount - 1 ? kMaxPlayers - 1 : kSpotCount - 1;
    if (NumAIPlayers > MaxAIPlayers)
    {
        UE_LOG(LogTemp, Warning, TEXT("RA4 BuildSkirmish: NumAIPlayers=%d clamped to %d"), NumAIPlayers, MaxAIPlayers);
        NumAIPlayers = MaxAIPlayers;
    }

    // Assign each AI slot a spot: the first takes AISpot (or spot 2 by default),
    // the rest take the remaining spots in table order. One array drives both the
    // player setup and the base seeding, so the two cannot drift apart.
    const int32 PlayerSpot = 0;
    int32 EnemySpot = (AISpot >= 0) ? (AISpot % kSpotCount) : 1;
    if (EnemySpot == PlayerSpot)
    {
        EnemySpot = (EnemySpot + 1) % kSpotCount;
    }
    int32 SpotOfSlot[kMaxPlayers];
    SpotOfSlot[0] = PlayerSpot;
    SpotOfSlot[1] = EnemySpot;
    {
        int32 Next = 0;
        for (int32 Slot = 2; Slot <= NumAIPlayers; ++Slot)
        {
            while (Next < kSpotCount)
            {
                bool bTaken = false;
                for (int32 Other = 0; Other < Slot; ++Other)
                {
                    if (SpotOfSlot[Other] == Next)
                    {
                        bTaken = true;
                        break;
                    }
                }
                if (!bTaken)
                {
                    break;
                }
                ++Next;
            }
            SpotOfSlot[Slot] = Next;
        }
    }

    // Additional AI slots alternate between the two playable factions, so two AIs
    // in one match do not mirror each other's build order.
    for (int32 Slot = 1; Slot <= NumAIPlayers; ++Slot)
    {
        Setup.Players[Slot].bActive = true;
        Setup.Players[Slot].Faction = EnemyFaction;
        Setup.Players[Slot].StartingCredits = 10000;
    }
    for (int32 Slot = 2; Slot <= NumAIPlayers; Slot += 2)
    {
        Setup.Players[Slot].Faction =
            EnemyFaction == RA4::FactionId::Soviet ? RA4::FactionId::Alliance : RA4::FactionId::Soviet;
    }

    World.Initialize(&Content, Setup, ReconSettings);

    const auto ForceFor = [](RA4::FactionId Faction) -> const StartingForce&
    {
        return (Faction == RA4::FactionId::Soviet) ? SovietForce : AllianceForce;
    };

    for (int32 Slot = 0; Slot <= NumAIPlayers; ++Slot)
    {
        SeedBase(World, ForceFor(Setup.Players[Slot].Faction), PlayerId(Slot),
                 Spots[SpotOfSlot[Slot]].Yard, Spots[SpotOfSlot[Slot]].Ore);
    }
    World.ClearEvents();
}
