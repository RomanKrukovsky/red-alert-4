// Copyright (c) Red Alert 4 project. Tests for Stage 14 (Tactical Combat Micro & Focus Fire).
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4AI/TacticalCombatMicro.h"
#include "RA4Content/ContentDatabase.h"
#include "RA4Core/Fixed.h"
#include "RA4Core/Vector.h"
#include "RA4Simulation/SimWorld.h"

#include <memory>
#include <vector>

using namespace RA4;
using namespace RA4Test;

namespace
{

std::unique_ptr<ContentDatabase> MakeTacticalContentDb()
{
    auto Db = std::make_unique<ContentDatabase>();
    Db->ResetDamageTableToDefaults();

    // ConYard
    {
        EntityDef E;
        E.Id = MakeContentId("building.allied.conyard");
        E.Name = "building.allied.conyard";
        E.Kind = EntityKind::Building;
        E.MaxHealth = 2000;
        E.Armor = ArmorClass::Building;
        E.Building.FootprintX = 2;
        E.Building.FootprintY = 2;
        E.Building.bIsConstructionYard = true;
        E.Building.PowerProduced = 100;
        Db->AddEntity(E);
    }

    // Allied Peacekeeper (Ranged Infantry)
    {
        EntityDef E;
        E.Id = MakeContentId("unit.allied.peacekeeper");
        E.Name = "unit.allied.peacekeeper";
        E.Kind = EntityKind::Unit;
        E.MaxHealth = 150;
        E.Armor = ArmorClass::LightInfantry;
        E.Production.Cost = 200;
        E.Unit.Layer = MovementLayer::Infantry;
        E.Unit.MaxSpeed = Fixed::FromInt(110);
        E.Unit.Acceleration = Fixed::FromInt(300);
        E.Unit.TurnRatePerSecond = 2048;
        E.Unit.CollisionRadius = Fixed::FromInt(12);
        Db->AddEntity(E);
    }

    // Heavy Apocalypse Tank
    {
        EntityDef E;
        E.Id = MakeContentId("unit.sov.apocalypse");
        E.Name = "unit.sov.apocalypse";
        E.Kind = EntityKind::Unit;
        E.MaxHealth = 1200;
        E.Armor = ArmorClass::HeavyVehicle;
        E.Production.Cost = 2000;
        E.Unit.Layer = MovementLayer::Tracked;
        E.Unit.MaxSpeed = Fixed::FromInt(80);
        E.Unit.Acceleration = Fixed::FromInt(150);
        E.Unit.TurnRatePerSecond = 512;
        E.Unit.CollisionRadius = Fixed::FromInt(30);
        Db->AddEntity(E);
    }

    return Db;
}

} // namespace

// --- 1. Stutter-Step Kiting Away From Close Threat ---

RA4_TEST(TacticalCombatMicro, StutterStepKiteAwayFromCloseMelee)
{
    auto Content = MakeTacticalContentDb();
    MatchSetup Setup = MakeTestSetup(501);

    SimWorld World;
    World.Initialize(Content.get(), Setup);

    const ContentId ConYard = MakeContentId("building.allied.conyard");
    World.SpawnBuilding(ConYard, 0, TileCoord(2, 2), true);
    World.SpawnBuilding(ConYard, 1, TileCoord(50, 50), true);

    const ContentId PeacekeeperDef = MakeContentId("unit.allied.peacekeeper");
    const ContentId TankDef = MakeContentId("unit.sov.apocalypse");

    const EntityId MyUnit = World.SpawnUnit(PeacekeeperDef, 0, Vec2(Fixed::FromInt(1000), Fixed::FromInt(1000)));
    const EntityId Threat = World.SpawnUnit(TankDef, 1, Vec2(Fixed::FromInt(1050), Fixed::FromInt(1000)));

    const Fixed PreferredRange = Fixed::FromInt(300);
    const MicroDecision Decision = TacticalCombatMicro::EvaluateKiteStep(MyUnit, Threat, World, PreferredRange);

    // Threat is at 50 units distance (within 250 units danger threshold) -> micro must kite away
    RA4_EXPECT(Decision.Action == MicroActionType::HoldFireAndRetreat);
    RA4_EXPECT(Decision.TargetLocation.X < Fixed::FromInt(1000));
}

// --- 2. Engage Target When at Safe Distance & Reloaded ---

RA4_TEST(TacticalCombatMicro, EngageTargetWhenSafeAndReloaded)
{
    auto Content = MakeTacticalContentDb();
    MatchSetup Setup = MakeTestSetup(502);

    SimWorld World;
    World.Initialize(Content.get(), Setup);

    const ContentId ConYard = MakeContentId("building.allied.conyard");
    World.SpawnBuilding(ConYard, 0, TileCoord(2, 2), true);
    World.SpawnBuilding(ConYard, 1, TileCoord(50, 50), true);

    const ContentId PeacekeeperDef = MakeContentId("unit.allied.peacekeeper");
    const ContentId TankDef = MakeContentId("unit.sov.apocalypse");

    const EntityId MyUnit = World.SpawnUnit(PeacekeeperDef, 0, Vec2(Fixed::FromInt(1000), Fixed::FromInt(1000)));
    const EntityId Threat = World.SpawnUnit(TankDef, 1, Vec2(Fixed::FromInt(1280), Fixed::FromInt(1000)));

    const Fixed PreferredRange = Fixed::FromInt(300);
    const MicroDecision Decision = TacticalCombatMicro::EvaluateKiteStep(MyUnit, Threat, World, PreferredRange);

    // Threat is at 280 units (safe and in weapon range) -> micro engages
    RA4_EXPECT(Decision.Action == MicroActionType::EngageTarget);
    RA4_EXPECT(Decision.TargetEntity == Threat);
}

// --- 3. Focus Fire Optimal Selection ---

RA4_TEST(TacticalCombatMicro, FocusFireLowHealthHighValueTarget)
{
    auto Content = MakeTacticalContentDb();
    MatchSetup Setup = MakeTestSetup(503);

    SimWorld World;
    World.Initialize(Content.get(), Setup);

    const ContentId ConYard = MakeContentId("building.allied.conyard");
    World.SpawnBuilding(ConYard, 0, TileCoord(2, 2), true);
    World.SpawnBuilding(ConYard, 1, TileCoord(50, 50), true);

    const ContentId PeacekeeperDef = MakeContentId("unit.allied.peacekeeper");
    const ContentId ApocalypseDef = MakeContentId("unit.sov.apocalypse");

    const EntityId Attacker = World.SpawnUnit(PeacekeeperDef, 0, Vec2(Fixed::FromInt(1000), Fixed::FromInt(1000)));

    const EntityId HealthyInfantry = World.SpawnUnit(PeacekeeperDef, 1, Vec2(Fixed::FromInt(1100), Fixed::FromInt(1000)));
    const EntityId HealthyApocalypse = World.SpawnUnit(ApocalypseDef, 1, Vec2(Fixed::FromInt(1150), Fixed::FromInt(1000)));
    const EntityId DamagedApocalypse = World.SpawnUnit(ApocalypseDef, 1, Vec2(Fixed::FromInt(1200), Fixed::FromInt(1000)));

    // Damage DamagedApocalypse down to 5% health (60 HP out of 1200)
    World.DebugDamage(DamagedApocalypse, 1140);

    std::vector<EntityId> Candidates = { HealthyInfantry, HealthyApocalypse, DamagedApocalypse };
    const EntityId BestTarget = TacticalCombatMicro::SelectOptimalTarget(Attacker, Candidates, World);

    // AI must prioritize the heavily damaged 2000-credit Apocalypse to remove high-threat DPS quickly
    RA4_EXPECT(BestTarget == DamagedApocalypse);
}
