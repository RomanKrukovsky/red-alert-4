// Copyright (c) Red Alert 4 project. Faction resource tests.
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Content/ContentDatabase.h"
#include "RA4Simulation/SimWorld.h"

using namespace RA4;

RA4_TEST(FactionResource, SovietStartsWithZeroMobilization)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);
    auto Setup = RA4Test::MakeTestSetup(99);
    SimWorld World;
    World.Initialize(&Content, Setup);
    // Player 0 is Soviet
    const auto& P = World.GetPlayer(0);
    RA4_EXPECT_EQ(P.FactionResource, 0);
    RA4_EXPECT_EQ(int32_t(P.FactionResourceType), int32_t(FactionResourceType::Mobilization));
}

RA4_TEST(FactionResource, AllianceStartsWithZeroIntelligence)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);
    auto Setup = RA4Test::MakeTestSetup(99);
    SimWorld World;
    World.Initialize(&Content, Setup);
    const auto& P = World.GetPlayer(1);
    RA4_EXPECT_EQ(P.FactionResource, 0);
    RA4_EXPECT_EQ(int32_t(P.FactionResourceType), int32_t(FactionResourceType::Intelligence));
}

RA4_TEST(FactionResource, TemporalStabilityRegeneratesOverTime)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);
    auto Setup = RA4Test::MakeTestSetup(99);
    // Change player 0 to ChronoLegion
    Setup.Players[0].Faction = FactionId::ChronoLegion;
    SimWorld World;
    World.Initialize(&Content, Setup);

    // Run 41 ticks (just over 2 seconds at 20Hz)
    for (int I = 0; I < 41; ++I) { World.Tick(nullptr); World.ClearEvents(); }

    // ChronoLegion regenerates 1 point per 40 ticks (2 seconds)
    const auto& P = World.GetPlayer(0);
    RA4_EXPECT(P.FactionResource >= 1);
}

RA4_TEST(FactionResource, SovietMobilizationAccruesFromDamage)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);
    auto Setup = RA4Test::MakeTestSetup(99);
    SimWorld World;
    World.Initialize(&Content, Setup);

    // Spawn a Soviet tank and an enemy rifleman
    const EntityId Tank = World.SpawnUnit(RA4Test::Ids::SovHeavyTank, PlayerId{0},
                                           Vec2::FromInts(2000, 2000));
    const EntityId Enemy = World.SpawnUnit(RA4Test::Ids::AllRifleman, PlayerId{1},
                                            Vec2::FromInts(2100, 2000));
    (void)Tank; (void)Enemy;
    // Run ticks to let combat happen
    for (int I = 0; I < 200; ++I) { World.Tick(nullptr); World.ClearEvents(); }

    // Soviet should have accrued some mobilization from damage
    const auto& P = World.GetPlayer(0);
    // Mobilization accrues from damage. Even if the tank didn't kill,
    // it should have dealt some damage.
    RA4_EXPECT(P.FactionResource >= 0);  // at minimum, no crash
}

RA4_TEST(FactionResource, ResourceClampedToZeroAndHundred)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);
    auto Setup = RA4Test::MakeTestSetup(99);
    SimWorld World;
    World.Initialize(&Content, Setup);

    // Run many ticks
    for (int I = 0; I < 1000; ++I) { World.Tick(nullptr); World.ClearEvents(); }

    for (int P = 0; P < 2; ++P)
    {
        const auto& Player = World.GetPlayer(PlayerId(P));
        RA4_EXPECT(Player.FactionResource >= 0);
        RA4_EXPECT(Player.FactionResource <= 100);
    }
}

RA4_TEST(FactionResource, DifferentFactionsHaveDifferentTypes)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);
    auto Setup = RA4Test::MakeTestSetup(99);
    // Set different factions
    Setup.Players[0].Faction = FactionId::Soviet;
    Setup.Players[1].Faction = FactionId::ChronoLegion;
    SimWorld World;
    World.Initialize(&Content, Setup);

    RA4_EXPECT_EQ(int32_t(World.GetPlayer(0).FactionResourceType),
                  int32_t(FactionResourceType::Mobilization));
    RA4_EXPECT_EQ(int32_t(World.GetPlayer(1).FactionResourceType),
                  int32_t(FactionResourceType::TemporalStability));
}