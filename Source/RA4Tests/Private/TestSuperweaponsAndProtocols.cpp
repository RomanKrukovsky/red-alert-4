// Copyright (c) Red Alert 4 project. Tests for Stage 8 (Top-Secret Protocols, Global Powers & Superweapons).
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Content/ContentDatabase.h"
#include "RA4Core/Command.h"
#include "RA4Core/Fixed.h"
#include "RA4Core/Vector.h"
#include "RA4Simulation/ProtocolRuntime.h"
#include "RA4Simulation/SimWorld.h"

#include <memory>
#include <vector>

using namespace RA4;
using namespace RA4Test;

namespace
{

std::unique_ptr<ContentDatabase> MakeProtocolTestContent()
{
    auto Db = std::make_unique<ContentDatabase>();

    // ConYard
    {
        EntityDef E;
        E.Id = MakeContentId("building.sov.conyard");
        E.Name = "building.sov.conyard";
        E.Kind = EntityKind::Building;
        E.MaxHealth = 2000;
        E.Armor = ArmorClass::Building;
        E.Building.FootprintX = 2;
        E.Building.FootprintY = 2;
        E.Building.bIsConstructionYard = true;
        E.Building.PowerProduced = 100;
        Db->AddEntity(E);
    }

    // Superweapon Building
    {
        EntityDef E;
        E.Id = MakeContentId("building.sov.iron_barrage");
        E.Name = "building.sov.iron_barrage";
        E.Kind = EntityKind::Building;
        E.MaxHealth = 3000;
        E.Armor = ArmorClass::Building;
        E.Building.FootprintX = 3;
        E.Building.FootprintY = 3;
        E.Building.PowerConsumed = 50;
        E.Building.SuperweaponRechargeTicks = 200; // 10s for fast testing
        E.Building.SuperweaponDamage = 2000;
        E.Building.SuperweaponRadius = Fixed::FromInt(500);
        E.Building.SuperweaponWarhead = WarheadClass::Siege;
        Db->AddEntity(E);
    }

    // Heavy Tank
    {
        EntityDef E;
        E.Id = MakeContentId("unit.sov.heavy_tank");
        E.Name = "unit.sov.heavy_tank";
        E.Kind = EntityKind::Unit;
        E.MaxHealth = 1000;
        E.Armor = ArmorClass::HeavyVehicle;
        E.Production.Cost = 1200;
        E.Unit.Layer = MovementLayer::Tracked;
        E.Unit.MaxSpeed = Fixed::FromInt(150);
        E.Unit.Acceleration = Fixed::FromInt(300);
        E.Unit.TurnRatePerSecond = 1024;
        E.Unit.CollisionRadius = Fixed::FromInt(25);
        Db->AddEntity(E);
    }

    return Db;
}

} // namespace

// --- 1. Protocol XP, Points & Tech Tree Prerequisite Unlocks ---

RA4_TEST(Protocols, ExperiencePointsAndTreeUnlocking)
{
    ProtocolRuntime Protocols;

    // Initially 0 XP and 0 points
    const auto& InitialState = Protocols.GetPlayerState(0);
    RA4_EXPECT_EQ(InitialState.TotalExperience, 0u);
    RA4_EXPECT_EQ(InitialState.AvailablePoints, 0u);

    // Award 3500 XP (thresholds: 1000 -> 1 pt, 3000 -> 2 pts)
    Protocols.AwardExperience(0, 3500);
    const auto& StateAfterXP = Protocols.GetPlayerState(0);
    RA4_EXPECT_EQ(StateAfterXP.AvailablePoints, 2u);

    // Try unlocking Tier 2 or Tier 3 without prerequisite -> must fail
    RA4_EXPECT_EQ(Protocols.CanUnlockProtocol(0, "sov_protocol_orbital_strike"), false);
    RA4_EXPECT_EQ(Protocols.CanUnlockProtocol(0, "sov_protocol_magnetic_satellite"), false);

    // Unlock Tier 1 prerequisite
    RA4_EXPECT_EQ(Protocols.CanUnlockProtocol(0, "sov_protocol_production"), true);
    RA4_EXPECT_EQ(Protocols.UnlockProtocol(0, "sov_protocol_production"), true);

    RA4_EXPECT_EQ(Protocols.GetPlayerState(0).AvailablePoints, 1u);
    RA4_EXPECT_EQ(Protocols.GetPlayerState(0).HasProtocol("sov_protocol_production"), true);

    // Now Tier 2 prerequisite is satisfied -> can unlock
    RA4_EXPECT_EQ(Protocols.CanUnlockProtocol(0, "sov_protocol_orbital_strike"), true);
    RA4_EXPECT_EQ(Protocols.UnlockProtocol(0, "sov_protocol_orbital_strike"), true);

    RA4_EXPECT_EQ(Protocols.GetPlayerState(0).AvailablePoints, 0u);
    RA4_EXPECT_EQ(Protocols.GetPlayerState(0).HasProtocol("sov_protocol_orbital_strike"), true);

    // With 0 points left, unlocking Tier 3 must fail
    RA4_EXPECT_EQ(Protocols.CanUnlockProtocol(0, "sov_protocol_magnetic_satellite"), false);
}

// --- 2. Global Power Casting, AOE Impact & Cooldown Runtime ---

RA4_TEST(Protocols, GlobalPowerExecutionAndCooldowns)
{
    auto Content = MakeProtocolTestContent();
    MatchSetup Setup = MakeTestSetup(201);

    SimWorld World;
    World.Initialize(Content.get(), Setup);

    const ContentId ConYard = MakeContentId("building.sov.conyard");
    World.SpawnBuilding(ConYard, 0, TileCoord(2, 2), true);
    World.SpawnBuilding(ConYard, 1, TileCoord(50, 50), true);

    const ContentId TankDef = MakeContentId("unit.sov.heavy_tank");
    const EntityId EnemyTank = World.SpawnUnit(TankDef, 1, Vec2(Fixed::FromInt(2000), Fixed::FromInt(2000)));

    ProtocolRuntime Protocols;
    Protocols.AwardExperience(0, 5000);
    Protocols.UnlockProtocol(0, "sov_protocol_production");
    Protocols.UnlockProtocol(0, "sov_protocol_orbital_strike");

    const Vec2 StrikeTarget(Fixed::FromInt(2000), Fixed::FromInt(2000));

    // Cast Orbital Strike at enemy tank position
    RA4_EXPECT_EQ(Protocols.CanCastPower(0, "sov_protocol_orbital_strike", StrikeTarget, World), true);
    RA4_EXPECT_EQ(Protocols.CastPower(0, "sov_protocol_orbital_strike", StrikeTarget, World), true);

    // Enemy tank had 1000 health, Orbital Strike dealt 1000 damage -> destroyed
    RA4_EXPECT_EQ(World.IsAlive(EnemyTank), false);

    // Power must now be on cooldown
    RA4_EXPECT_EQ(Protocols.GetPlayerState(0).IsOnCooldown("sov_protocol_orbital_strike", World.GetTick()), true);
    RA4_EXPECT_EQ(Protocols.CanCastPower(0, "sov_protocol_orbital_strike", StrikeTarget, World), false);
    RA4_EXPECT_EQ(Protocols.CastPower(0, "sov_protocol_orbital_strike", StrikeTarget, World), false);
}

// --- 3. Superweapon Status Tracking & Power Grid Dependencies ---

RA4_TEST(Superweapons, StatusTrackingAndPowerGridDependency)
{
    auto Content = MakeProtocolTestContent();
    MatchSetup Setup = MakeTestSetup(202);

    SimWorld World;
    World.Initialize(Content.get(), Setup);

    const ContentId ConYard = MakeContentId("building.sov.conyard");
    World.SpawnBuilding(ConYard, 0, TileCoord(2, 2), true);
    World.SpawnBuilding(ConYard, 1, TileCoord(50, 50), true);

    const ContentId SuperDef = MakeContentId("building.sov.iron_barrage");
    const EntityId SuperBld = World.SpawnBuilding(SuperDef, 0, TileCoord(10, 10), true);

    ProtocolRuntime Protocols;

    // Check initial superweapon status
    auto Statuses = Protocols.GetSuperweaponStatuses(World);
    RA4_REQUIRE(Statuses.size() == 1u);
    RA4_EXPECT(Statuses[0].BuildingEntity == SuperBld);
    RA4_EXPECT_EQ(Statuses[0].Owner, 0u);
    RA4_EXPECT_EQ(Statuses[0].bReady, false);
    RA4_EXPECT_EQ(Statuses[0].bPowered, true);
    RA4_EXPECT_EQ(Statuses[0].TotalRechargeTicks, 200);

    // Advance 100 ticks
    for (int I = 0; I < 100; ++I)
    {
        World.Tick(nullptr);
    }

    Statuses = Protocols.GetSuperweaponStatuses(World);
    RA4_REQUIRE(Statuses.size() == 1u);
    RA4_EXPECT_EQ(Statuses[0].ChargeTicks, 100);
    RA4_EXPECT_EQ(Statuses[0].ChargePercent, 50);
    RA4_EXPECT_EQ(Statuses[0].bReady, false);

    // Advance another 100 ticks to fully charge
    for (int I = 0; I < 100; ++I)
    {
        World.Tick(nullptr);
    }

    Statuses = Protocols.GetSuperweaponStatuses(World);
    RA4_REQUIRE(Statuses.size() == 1u);
    RA4_EXPECT_EQ(Statuses[0].ChargeTicks, 200);
    RA4_EXPECT_EQ(Statuses[0].ChargePercent, 100);
    RA4_EXPECT_EQ(Statuses[0].bReady, true);
}
