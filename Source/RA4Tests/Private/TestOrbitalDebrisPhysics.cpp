// Copyright (c) Red Alert 4 project. Tests for Stage 18 (Magnetic Satellite & Orbital Debris Physics).
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Content/ContentDatabase.h"
#include "RA4Core/Fixed.h"
#include "RA4Core/Vector.h"
#include "RA4Simulation/OrbitalDebrisPhysics.h"
#include "RA4Simulation/SimWorld.h"

#include <memory>

using namespace RA4;
using namespace RA4Test;

namespace
{

std::unique_ptr<ContentDatabase> MakeOrbitalContentDb()
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

    // Heavy Tank (Tracked vehicle)
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

    // Conscript Infantry
    {
        EntityDef E;
        E.Id = MakeContentId("unit.sov.conscript");
        E.Name = "unit.sov.conscript";
        E.Kind = EntityKind::Unit;
        E.MaxHealth = 100;
        E.Armor = ArmorClass::LightInfantry;
        E.Production.Cost = 100;
        E.Unit.Layer = MovementLayer::Infantry;
        E.Unit.MaxSpeed = Fixed::FromInt(100);
        E.Unit.Acceleration = Fixed::FromInt(400);
        E.Unit.TurnRatePerSecond = 2048;
        E.Unit.CollisionRadius = Fixed::FromInt(10);
        Db->AddEntity(E);
    }

    return Db;
}

} // namespace

// --- 1. Magnetic Satellite Vehicle Ascension ---

RA4_TEST(OrbitalDebrisPhysics, MagneticSatelliteVehicleAscension)
{
    auto Content = MakeOrbitalContentDb();
    MatchSetup Setup = MakeTestSetup(801);

    SimWorld World;
    World.Initialize(Content.get(), Setup);

    const ContentId ConYard = MakeContentId("building.sov.conyard");
    World.SpawnBuilding(ConYard, 0, TileCoord(2, 2), true);
    World.SpawnBuilding(ConYard, 1, TileCoord(50, 50), true);

    const ContentId TankDef = MakeContentId("unit.sov.heavy_tank");
    const ContentId ConscriptDef = MakeContentId("unit.sov.conscript");

    const EntityId Tank = World.SpawnUnit(TankDef, 1, Vec2(Fixed::FromInt(1000), Fixed::FromInt(1000)));
    const EntityId Infantry = World.SpawnUnit(ConscriptDef, 1, Vec2(Fixed::FromInt(1020), Fixed::FromInt(1020)));

    OrbitalDebrisPhysics Orbital;
    Orbital.TriggerMagneticSatellite(Vec2(Fixed::FromInt(1000), Fixed::FromInt(1000)), Fixed::FromInt(200), 20);

    // Tick magnetic attraction
    Orbital.Tick(World);
    World.Tick(nullptr); // Process deaths

    // Heavy Tank is sucked into orbit
    RA4_EXPECT_EQ(World.IsAlive(Tank), false);

    // Biological infantry is NOT magnetic -> stays unharmed on ground
    RA4_EXPECT_EQ(World.IsAlive(Infantry), true);
}

// --- 2. Orbital Debris Kinetic Re-entry & Impact ---

RA4_TEST(OrbitalDebrisPhysics, OrbitalDebrisKineticImpact)
{
    auto Content = MakeOrbitalContentDb();
    MatchSetup Setup = MakeTestSetup(802);

    SimWorld World;
    World.Initialize(Content.get(), Setup);

    const ContentId ConYard = MakeContentId("building.sov.conyard");
    const EntityId TargetBuilding = World.SpawnBuilding(ConYard, 1, TileCoord(20, 20), true);
    const Vec2 TargetPos = World.GetTransform(TargetBuilding)->Position;

    OrbitalDebrisPhysics Orbital;
    Orbital.TriggerOrbitalDrop(TargetPos, Fixed::FromInt(300), 1500);

    // Advance 10 ticks (mid-fall) -> debris has not yet impacted
    for (int I = 0; I < 10; ++I)
    {
        Orbital.Tick(World);
    }
    RA4_EXPECT_EQ(World.GetHealth(TargetBuilding)->Current, 2000);

    // Advance remaining 10 ticks (20 total) -> kinetic impact!
    for (int I = 0; I < 10; ++I)
    {
        Orbital.Tick(World);
    }
    World.Tick(nullptr);

    // Took 1500 damage -> 500 HP left
    RA4_EXPECT_EQ(World.GetHealth(TargetBuilding)->Current, 500);
}
