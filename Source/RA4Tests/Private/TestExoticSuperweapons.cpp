// Copyright (c) Red Alert 4 project. Tests for Stage 13 (Exotic Superweapons: Vacuum Imploder, Iron Curtain, Chrono Sphere).
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Content/ContentDatabase.h"
#include "RA4Core/Fixed.h"
#include "RA4Core/Vector.h"
#include "RA4Simulation/ExoticSuperweaponPhysics.h"
#include "RA4Simulation/SimWorld.h"

#include <memory>
#include <vector>

using namespace RA4;
using namespace RA4Test;

namespace
{

std::unique_ptr<ContentDatabase> MakeExoticContentDatabase()
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

    // Heavy Tank (Tracked, Ground only)
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

// --- 1. Vacuum Imploder Gravitational Pull & Detonation ---

RA4_TEST(ExoticSuperweapons, VacuumImploderGravitationalPull)
{
    auto Content = MakeExoticContentDatabase();
    MatchSetup Setup = MakeTestSetup(401);

    SimWorld World;
    World.Initialize(Content.get(), Setup);

    const ContentId ConYard = MakeContentId("building.sov.conyard");
    World.SpawnBuilding(ConYard, 0, TileCoord(2, 2), true);
    World.SpawnBuilding(ConYard, 1, TileCoord(50, 50), true);

    const ContentId TankDef = MakeContentId("unit.sov.heavy_tank");
    const EntityId Tank = World.SpawnUnit(TankDef, 1, Vec2(Fixed::FromInt(2000), Fixed::FromInt(2000)));

    ExoticSuperweaponPhysics Physics;
    const Vec2 Epicenter(Fixed::FromInt(2000), Fixed::FromInt(2400)); // 400 units north

    Physics.TriggerVacuumImploder(Epicenter, Fixed::FromInt(600), 3000);

    // Advance 20 ticks of gravitational pull
    for (int I = 0; I < 20; ++I)
    {
        Physics.Tick(World);
    }

    // Tank should have been pulled toward the singularity (Y increased)
    const Vec2 PulledPos = World.GetTransform(Tank)->Position;
    RA4_EXPECT(PulledPos.Y > Fixed::FromInt(2000));
    RA4_EXPECT(World.IsAlive(Tank));

    // Advance remaining 20 ticks to trigger implosion blast
    for (int I = 0; I < 20; ++I)
    {
        Physics.Tick(World);
    }
    World.Tick(nullptr); // Process SystemDeaths

    // Tank took 3000 siege damage -> destroyed
    RA4_EXPECT_EQ(World.IsAlive(Tank), false);

}

// --- 2. Iron Curtain Infantry Vaporization vs Vehicle Protection ---

RA4_TEST(ExoticSuperweapons, IronCurtainInfantryVaporization)
{
    auto Content = MakeExoticContentDatabase();
    MatchSetup Setup = MakeTestSetup(402);

    SimWorld World;
    World.Initialize(Content.get(), Setup);

    const ContentId ConYard = MakeContentId("building.sov.conyard");
    World.SpawnBuilding(ConYard, 0, TileCoord(2, 2), true);
    World.SpawnBuilding(ConYard, 1, TileCoord(50, 50), true);

    const ContentId TankDef = MakeContentId("unit.sov.heavy_tank");
    const ContentId ConscriptDef = MakeContentId("unit.sov.conscript");

    const EntityId Tank = World.SpawnUnit(TankDef, 0, Vec2(Fixed::FromInt(2000), Fixed::FromInt(2000)));
    const EntityId Infantry = World.SpawnUnit(ConscriptDef, 0, Vec2(Fixed::FromInt(2020), Fixed::FromInt(2020)));

    const Vec2 CurtainCenter(Fixed::FromInt(2000), Fixed::FromInt(2000));
    ExoticSuperweaponPhysics::TriggerIronCurtain(CurtainCenter, Fixed::FromInt(300), 200, World);

    // Infantry is instantly vaporized by the curtain energy field
    RA4_EXPECT_EQ(World.IsAlive(Infantry), false);

    // Tank survives
    RA4_EXPECT_EQ(World.IsAlive(Tank), true);
}

// --- 3. Chrono Sphere Aquatic Drop Teleportation ---

RA4_TEST(ExoticSuperweapons, ChronoSphereAquaticDrop)
{
    auto Content = MakeExoticContentDatabase();
    MatchSetup Setup = MakeTestSetup(403);

    SimWorld World;
    World.Initialize(Content.get(), Setup);

    const ContentId ConYard = MakeContentId("building.sov.conyard");
    World.SpawnBuilding(ConYard, 0, TileCoord(2, 2), true);
    World.SpawnBuilding(ConYard, 1, TileCoord(50, 50), true);

    // Mark water tile at (20, 20)
    World.GetMapMutable().SetTileFlag(20, 20, Tile_Water, true);
    World.GetMapMutable().SetTileFlag(20, 20, Tile_GroundPassable, false);

    const ContentId TankDef = MakeContentId("unit.sov.heavy_tank");
    const EntityId Tank = World.SpawnUnit(TankDef, 0, Vec2(Fixed::FromInt(1000), Fixed::FromInt(1000)));

    const Vec2 SourceCenter(Fixed::FromInt(1000), Fixed::FromInt(1000));
    const Vec2 WaterTarget = World.GetMap().TileCenterToWorld(TileCoord(20, 20));

    ExoticSuperweaponPhysics::TriggerChronoSphere(SourceCenter, WaterTarget, Fixed::FromInt(200), World);

    // Ground vehicle dropped in water -> drowned and destroyed
    RA4_EXPECT_EQ(World.IsAlive(Tank), false);
}
