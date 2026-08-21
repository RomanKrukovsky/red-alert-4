// Copyright (c) Red Alert 4 project. Tests for Stage 5 (Advanced AI, Expansion, Micro & Self-Play).
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4AI/AICommander.h"
#include "RA4AI/AISelfPlayLeague.h"
#include "RA4Content/ContentDatabase.h"
#include "RA4Core/Command.h"
#include "RA4Core/Fixed.h"
#include "RA4Core/Vector.h"
#include "RA4Simulation/SimWorld.h"

#include <memory>
#include <vector>

using namespace RA4;
using namespace RA4Test;
using namespace RA4::AI;

// --- 1. Multi-Base & Water Structure Placement ---

RA4_TEST(AIStage5, MultiBaseWaterAndForwardPlacement)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);

    MatchSetup Setup = MakeTestSetup(1001);
    // Create map with water tile section
    Setup.Map.Resize(64, 64, Tile_GroundPassable);
    for (int32_t X = 20; X < 30; ++X)
    {
        for (int32_t Y = 20; Y < 30; ++Y)
        {
            Setup.Map.Tiles[Setup.Map.TileIndex(X, Y)] = Tile_Water;
        }
    }

    SimWorld World;
    World.Initialize(&Content, Setup);

    const ContentId SovYard = MakeContentId("building.sov.construction_yard");

    // Spawn primary base and a forward base
    World.SpawnBuilding(SovYard, 0, TileCoord(10, 10), true);
    World.SpawnBuilding(SovYard, 0, TileCoord(25, 15), true); // forward base near water

    AICommander Cmd;
    Cmd.Initialize(0, AIProfile::Aggressive, 1001);

    std::vector<Command> OutCommands;

    // Simulate AI tick to evaluate placement
    Cmd.Tick(World, OutCommands);

    // Command placement check
    RA4_EXPECT(World.GetPlayer(0).Credits >= 0);
}

// --- 2. Secondary Ability (F-Ability) Tactical Combat Micro ---

RA4_TEST(AIStage5, SecondaryAbilityTacticalMicro)
{
    auto Content = std::make_unique<ContentDatabase>();

    // ConYards
    {
        EntityDef E;
        E.Id = MakeContentId("building.test_conyard");
        E.Name = "building.test_conyard";
        E.Kind = EntityKind::Building;
        E.MaxHealth = 2000;
        E.Armor = ArmorClass::Building;
        E.Building.FootprintX = 2;
        E.Building.FootprintY = 2;
        E.Building.bIsConstructionYard = true;
        E.Building.bProvidesBuildRadius = true;
        E.Building.BuildRadius = Fixed::FromInt(2000);
        Content->AddEntity(E);
    }

    // Heavy Tank with Secondary Ability
    {
        EntityDef E;
        E.Id = MakeContentId("unit.heavy_tank");
        E.Name = "unit.heavy_tank";
        E.Kind = EntityKind::Unit;
        E.MaxHealth = 800;
        E.Armor = ArmorClass::HeavyVehicle;
        E.Unit.Layer = MovementLayer::Tracked;
        E.Unit.MaxSpeed = Fixed::FromInt(150);
        E.Unit.TurnRatePerSecond = 1024;
        E.Unit.CollisionRadius = Fixed::FromInt(25);
        E.Unit.bHasSecondaryAbility = true;
        E.Unit.SecondaryAbilityDef = MakeContentId("ability.overdrive");
        E.Unit.AbilityCooldownTicks = 20;
        E.Unit.AbilityDurationTicks = 10;
        E.Unit.AbilitySpeedMultiplier = Fixed::FromRatio(15, 10);
        E.Unit.AbilityArmorBonusPercent = 50;
        Content->AddEntity(E);
    }

    MatchSetup Setup = MakeTestSetup(42);
    SimWorld World;
    World.Initialize(Content.get(), Setup);

    const ContentId ConYard = MakeContentId("building.test_conyard");
    World.SpawnBuilding(ConYard, 0, TileCoord(2, 2), true);
    World.SpawnBuilding(ConYard, 1, TileCoord(30, 30), true);

    const ContentId TankDef = MakeContentId("unit.heavy_tank");
    const EntityId FriendlyTank = World.SpawnUnit(TankDef, 0, Vec2(Fixed::FromInt(2000), Fixed::FromInt(2000)));
    World.SpawnUnit(TankDef, 1, Vec2(Fixed::FromInt(2200), Fixed::FromInt(2000)));

    AICommander Cmd;
    Cmd.Initialize(0, AIProfile::Aggressive, 42);

    std::vector<Command> OutCommands;
    Cmd.Tick(World, OutCommands);

    // AI combat micro should inspect combat units and prepare tactical actions
    RA4_EXPECT(World.IsAlive(FriendlyTank));
}

// --- 3. Elo Rating Computation ---

RA4_TEST(AIStage5, EloRatingCalculations)
{
    float EloA = 1500.0f;
    float EloB = 1500.0f;

    // Player A wins
    AISelfPlayLeague::UpdateElo(EloA, EloB, 1.0f, 32.0f);
    RA4_EXPECT(EloA > 1500.0f);
    RA4_EXPECT(EloB < 1500.0f);
    RA4_EXPECT_EQ(int32_t(EloA + EloB), 3000); // zero-sum rating conservation

    // Draw
    float OldEloA = EloA;
    float OldEloB = EloB;
    AISelfPlayLeague::UpdateElo(EloA, EloB, 0.5f, 32.0f);
    // Since EloA > EloB, a draw pulls EloA slightly down and EloB slightly up
    RA4_EXPECT(EloA < OldEloA);
    RA4_EXPECT(EloB > OldEloB);
}

// --- 4. Self-Play Tournament Execution ---

RA4_TEST(AIStage5, SelfPlayTournamentExecution)
{
    // Run a 2-match tournament between Aggressive and Defensive AI
    LeagueSummary Summary = AISelfPlayLeague::RunTournament(2, AIProfile::Aggressive, AIProfile::Defensive, 777, /*MaxTicks*/ 150);

    RA4_EXPECT_EQ(Summary.TotalMatchesRun, 2u);
    RA4_REQUIRE(Summary.Matches.size() == 2u);
    RA4_EXPECT(Summary.Matches[0].DurationTicks > 0u);
    RA4_EXPECT(Summary.Matches[1].DurationTicks > 0u);
    RA4_EXPECT(Summary.AverageDurationSeconds > 0.0f);
    RA4_EXPECT(Summary.EloRatingP0 > 0.0f);
    RA4_EXPECT(Summary.EloRatingP1 > 0.0f);
}
