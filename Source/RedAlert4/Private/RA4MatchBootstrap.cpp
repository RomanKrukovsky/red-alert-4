// Copyright (c) Red Alert 4 project.
#include "RA4MatchBootstrap.h"

#include "RA4Content/ContentDatabase.h"
#include "RA4Content/BibleContentLoader.h"
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
    ContentId Mcv;
    ContentId BasicInfantry;
    ContentId AntiArmorInfantry;
    ContentId MainTank;
    ContentId Artillery;
    ContentId SuperTank;
};

const StartingForce SovietForce{
    MakeContentId("unit.sov.mcv"),
    MakeContentId("unit.sov.conscript"),
    MakeContentId("unit.sov.rocket_trooper"),
    MakeContentId("unit.sov.heavy_tank"),
    MakeContentId("unit.sov.zarevo_mlrs"),
    MakeContentId("unit.sov.heavy_tank")};

const StartingForce AllianceForce{
    MakeContentId("unit.all.mcv"),
    MakeContentId("unit.all.rifleman"),
    MakeContentId("unit.all.missile_infantry"),
    MakeContentId("unit.all.light_tank"),
    MakeContentId("unit.all.oracle_artillery"),
    MakeContentId("unit.all.light_tank")};

Vec2 TileCentre(const TileCoord& Tile)
{
    return Vec2(RA4::Fixed::FromInt(int64(Tile.X) * kTileSizeUnits + kTileSizeUnits / 2),
                RA4::Fixed::FromInt(int64(Tile.Y) * kTileSizeUnits + kTileSizeUnits / 2));
}

// Opens with only the MCV (Mobile Construction Vehicle) and a robust starting combat army:
// 10 Infantry (6 basic + 4 rocket troopers), 5 Main Battle Tanks, 2 Rocket Launchers/Artillery,
// 1 Super Tank, and an Ore field for economic expansion.
void SeedBase(SimWorld& World, const StartingForce& Force, PlayerId Owner, const TileCoord& YardTile,
              const TileCoord& OreOrigin)
{
    // 1. Ore Field Resource Nodes
    for (int32 X = 0; X < 3; ++X)
    {
        for (int32 Y = 0; Y < 3; ++Y)
        {
            World.SpawnResourceNode(OreField, TileCoord(OreOrigin.X + X, OreOrigin.Y + Y), 3000);
        }
    }

    // 2. Mobile Construction Vehicle (MCV / МСЦ)
    World.SpawnUnit(Force.Mcv, Owner, TileCentre(YardTile));

    // 3. 10x Infantry (6 Basic + 4 Anti-Armor / Rocket Troopers)
    for (int32 Index = 0; Index < 6; ++Index)
    {
        World.SpawnUnit(Force.BasicInfantry, Owner, TileCentre(TileCoord(YardTile.X - 3 + Index, YardTile.Y + 3)));
    }
    for (int32 Index = 0; Index < 4; ++Index)
    {
        World.SpawnUnit(Force.AntiArmorInfantry, Owner, TileCentre(TileCoord(YardTile.X - 2 + Index, YardTile.Y + 4)));
    }

    // 4. 5x Main Battle Tanks
    for (int32 Index = 0; Index < 5; ++Index)
    {
        World.SpawnUnit(Force.MainTank, Owner, TileCentre(TileCoord(YardTile.X - 4 + Index * 2, YardTile.Y + 6)));
    }

    // 5. 2x Rocket Artillery (Zarevo MLRS / Oracle Artillery)
    World.SpawnUnit(Force.Artillery, Owner, TileCentre(TileCoord(YardTile.X - 3, YardTile.Y + 8)));
    World.SpawnUnit(Force.Artillery, Owner, TileCentre(TileCoord(YardTile.X + 3, YardTile.Y + 8)));

    // 6. 1x Super Tank / Command Assault Armor
    World.SpawnUnit(Force.SuperTank, Owner, TileCentre(TileCoord(YardTile.X, YardTile.Y + 8)));
}
} // namespace

void FRA4MatchBootstrap::BuildSkirmish(ContentDatabase& Content, SimWorld& World, uint64 Seed,
                                       const TArray<FRA4SkirmishSlotConfig>& PlayerSlots,
                                       int32 StartingCredits,
                                       const RA4::Recon::ReconSettings* ReconSettings)
{
    BuildDefaultContent(Content);
    // Supplement with bible-defined roster (78 units + 64 buildings) for full RA3 parity.
    // If the normalized JSON is absent (CI headless), we keep the default set.
    {
        std::vector<std::string> BibleErrors;
        const bool bBibleOk = LoadBibleContent(Content, "Content/RA4/Data/Generated/ra4_content.normalized.json", BibleErrors);
        if (bBibleOk)
        {
            UE_LOG(LogTemp, Display, TEXT("RA4 content: bible roster merged (%d entities)"), int32(Content.GetEntities().size()));
        }
        else if (!BibleErrors.empty())
        {
            UE_LOG(LogTemp, Verbose, TEXT("RA4 content: bible not loaded (%s)"), UTF8_TO_TCHAR(BibleErrors[0].c_str()));
        }
    }

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

    // The map's start spots, in spots-combobox order (Zapad = spot 1, Vostok =
    // spot 2, then the remaining corners). Yard tile plus the ore field that
    // belongs to it.
    struct StartSpot
    {
        TileCoord Yard;
        TileCoord Ore;
    };
    static const StartSpot Spots[kMaxPlayers] = {
        {TileCoord(10, 10), TileCoord(6, 15)},   // spot 1: NW corner
        {TileCoord(48, 48), TileCoord(53, 43)},  // spot 2: SE corner
        {TileCoord(48, 10), TileCoord(53, 6)},    // spot 3: NE corner
        {TileCoord(10, 48), TileCoord(6, 43)},   // spot 4: SW corner
        {TileCoord(29, 5),  TileCoord(25, 10)},   // spot 5: N edge center
        {TileCoord(29, 53), TileCoord(25, 48)},   // spot 6: S edge center
        {TileCoord(5, 29),  TileCoord(10, 25)},   // spot 7: W edge center
        {TileCoord(53, 29), TileCoord(48, 33)},   // spot 8: E edge center
        {TileCoord(29, 29), TileCoord(24, 34)},   // spot 9: map center
    };
    constexpr int32 kSpotCount = int32(sizeof(Spots) / sizeof(Spots[0]));

    bool TakenSpots[kSpotCount] = {};
    int32 ActivePlayers = 0;
    for (int32 Slot = 0; Slot < kMaxPlayers; ++Slot)
    {
        if (!PlayerSlots.IsValidIndex(Slot) || !PlayerSlots[Slot].bActive)
        {
            continue;
        }

        const FRA4SkirmishSlotConfig& Config = PlayerSlots[Slot];
        int32 Spot = FMath::Clamp(Config.StartSpot, 0, kSpotCount - 1);
        if (TakenSpots[Spot])
        {
            for (int32 Candidate = 0; Candidate < kSpotCount; ++Candidate)
            {
                if (!TakenSpots[Candidate])
                {
                    Spot = Candidate;
                    break;
                }
            }
        }
        TakenSpots[Spot] = true;
        Setup.Players[Slot].bActive = true;
        Setup.Players[Slot].Faction = Config.Faction;
        Setup.Players[Slot].Team = Config.Team;
        Setup.Players[Slot].StartingCredits = StartingCredits;
        Setup.Players[Slot].StartPositionIndex = Spot;
        ++ActivePlayers;
    }

    checkf(ActivePlayers >= 2, TEXT("A skirmish requires at least two active slots"));

    World.Initialize(&Content, Setup, ReconSettings);

    const auto ForceFor = [](RA4::FactionId Faction) -> const StartingForce&
    {
        return (Faction == RA4::FactionId::Soviet) ? SovietForce : AllianceForce;
    };

    for (int32 Slot = 0; Slot < kMaxPlayers; ++Slot)
    {
        if (!Setup.Players[Slot].bActive)
        {
            continue;
        }
        SeedBase(World, ForceFor(Setup.Players[Slot].Faction), PlayerId(Slot),
                 Spots[Setup.Players[Slot].StartPositionIndex].Yard,
                 Spots[Setup.Players[Slot].StartPositionIndex].Ore);
    }
    World.ClearEvents();
}
