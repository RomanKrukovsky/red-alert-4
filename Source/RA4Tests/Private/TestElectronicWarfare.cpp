// Copyright (c) Red Alert 4 project. Tests for Stage 16 (Electronic Warfare, Radar Jamming & Spy Sabotage).
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Content/ContentDatabase.h"
#include "RA4Core/Fixed.h"
#include "RA4Core/Vector.h"
#include "RA4Simulation/ElectronicWarfare.h"
#include "RA4Simulation/SimWorld.h"

#include <memory>

using namespace RA4;
using namespace RA4Test;

namespace
{

std::unique_ptr<ContentDatabase> MakeEWContentDb()
{
    auto Db = std::make_unique<ContentDatabase>();
    Db->ResetDamageTableToDefaults();

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

    // Mobile Radar Jammer
    {
        EntityDef E;
        E.Id = MakeContentId("unit.sov.jammer");
        E.Name = "unit.sov.jammer";
        E.Kind = EntityKind::Unit;
        E.MaxHealth = 400;
        E.Armor = ArmorClass::LightVehicle;
        E.Production.Cost = 800;
        E.Unit.Layer = MovementLayer::Wheeled;
        E.Unit.MaxSpeed = Fixed::FromInt(120);
        E.Unit.Acceleration = Fixed::FromInt(200);
        E.Unit.TurnRatePerSecond = 1024;
        E.Unit.CollisionRadius = Fixed::FromInt(18);
        Db->AddEntity(E);
    }

    return Db;
}

} // namespace

// --- 1. Radar Jammer Coverage & Unit Lifecycle Cleanup ---

RA4_TEST(ElectronicWarfare, RadarJammingCoverageAndDeactivation)
{
    auto Content = MakeEWContentDb();
    MatchSetup Setup = MakeTestSetup(601);

    SimWorld World;
    World.Initialize(Content.get(), Setup);

    const ContentId ConYard = MakeContentId("building.sov.conyard");
    World.SpawnBuilding(ConYard, 0, TileCoord(2, 2), true);
    World.SpawnBuilding(ConYard, 1, TileCoord(50, 50), true);

    const ContentId JammerDef = MakeContentId("unit.sov.jammer");
    const Vec2 JammerPos(Fixed::FromInt(1000), Fixed::FromInt(1000));
    const EntityId JammerUnit = World.SpawnUnit(JammerDef, 0, JammerPos);

    ElectronicWarfareSystem EW;
    EW.RegisterJammer(JammerUnit, 0, JammerPos, Fixed::FromInt(500));

    // Enemy viewer (Player 1) near jammer -> jammed
    const Vec2 NearbyPos(Fixed::FromInt(1200), Fixed::FromInt(1000)); // 200 units away
    RA4_EXPECT_EQ(EW.IsLocationJammedFor(1, NearbyPos), true);

    // Friendly viewer (Player 0) near jammer -> NOT jammed
    RA4_EXPECT_EQ(EW.IsLocationJammedFor(0, NearbyPos), false);

    // Enemy viewer far from jammer -> NOT jammed
    const Vec2 FarPos(Fixed::FromInt(2000), Fixed::FromInt(1000)); // 1000 units away
    RA4_EXPECT_EQ(EW.IsLocationJammedFor(1, FarPos), false);

    // Destroy jammer unit
    World.DebugDamage(JammerUnit, 9999);
    World.Tick(nullptr); // Process deaths

    EW.Tick(World); // Cleans up dead jammers

    // Jamming field is immediately deactivated
    RA4_EXPECT_EQ(EW.IsLocationJammedFor(1, NearbyPos), false);
}

// --- 2. Spy Infiltration Power Grid Sabotage & Restoration ---

RA4_TEST(ElectronicWarfare, SpyInfiltrationPowerGridSabotage)
{
    auto Content = MakeEWContentDb();
    MatchSetup Setup = MakeTestSetup(602);

    SimWorld World;
    World.Initialize(Content.get(), Setup);

    ElectronicWarfareSystem EW;

    // Infiltrate enemy Player 1 power plant for 30 ticks (1.5s)
    EW.ApplyInfiltrationSabotage(1, true, false, 30);

    RA4_EXPECT_EQ(EW.IsPowerGridSabotaged(1), true);
    RA4_EXPECT_EQ(EW.IsPowerGridSabotaged(0), false);

    // Advance 20 ticks -> still sabotaged
    for (int I = 0; I < 20; ++I)
    {
        EW.Tick(World);
    }
    RA4_EXPECT_EQ(EW.IsPowerGridSabotaged(1), true);

    // Advance remaining 10 ticks -> power grid restored
    for (int I = 0; I < 10; ++I)
    {
        EW.Tick(World);
    }
    RA4_EXPECT_EQ(EW.IsPowerGridSabotaged(1), false);
}
