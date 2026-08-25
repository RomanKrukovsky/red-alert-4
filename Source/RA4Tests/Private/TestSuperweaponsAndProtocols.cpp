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
    // The focused content database starts with a zeroed damage table. Give the
    // fixture cannon one real armor interaction so its shots can kill the tank.
    Db->SetDamageMultiplier(WarheadClass::ArmorPiercing, ArmorClass::HeavyVehicle, 100);

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

    // Test Weapon (same recipe as TestRA3Gameplay: fast, reliable, lethal enough)
    {
        WeaponDef W;
        W.Id = MakeContentId("weapon.test_cannon");
        W.Name = "weapon.test_cannon";
        W.Damage = 100;
        W.Warhead = WarheadClass::ArmorPiercing;
        W.MaxRange = Fixed::FromInt(500);
        W.CooldownTicks = 20;
        W.bCanTargetGround = true;
        Db->AddWeapon(W);
    }

    // Armed Gunner: kills through the real combat path, which is the only way to
    // exercise killer attribution (salvage bounty) and status-gated damage
    // (phase field) -- DebugDamage bypasses both by design.
    {
        EntityDef E;
        E.Id = MakeContentId("unit.test.gunner");
        E.Name = "unit.test.gunner";
        E.Kind = EntityKind::Unit;
        E.MaxHealth = 1000;
        E.Armor = ArmorClass::HeavyVehicle;
        E.Production.Cost = 1200;
        // Keep the fixture's firing solution explicit: live combat is fog-gated,
        // so an armed unit with no sight never exercises killer attribution.
        E.VisionRange = Fixed::FromInt(600);
        E.Weapon = MakeContentId("weapon.test_cannon");
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

// --- 4. New Protocol Powers: TroopDrop / PhaseField / EmpPulse / SalvageBounty ---

namespace
{

// Grants enough XP for two unlock points and unlocks a no-prerequisite test
// protocol directly; keeps each power test focused on the cast, not the tree.
void UnlockTestPower(ProtocolRuntime& Protocols, const char* ProtocolId)
{
    Protocols.AwardExperience(0, 5000);
    RA4_EXPECT_EQ(Protocols.UnlockProtocol(0, ProtocolId), true);
}

} // namespace

RA4_TEST(Protocols, TroopDropSpawnsSquadScatteredAroundTarget)
{
    auto Content = MakeProtocolTestContent();
    MatchSetup Setup = MakeTestSetup(203);

    SimWorld World;
    World.Initialize(Content.get(), Setup);
    World.SpawnBuilding(MakeContentId("building.sov.conyard"), 0, TileCoord(2, 2), true);
    World.SpawnBuilding(MakeContentId("building.sov.conyard"), 1, TileCoord(50, 50), true);

    const ContentId TankDef = MakeContentId("unit.sov.heavy_tank");

    ProtocolRuntime Protocols;
    ProtocolPowerDef Drop;
    Drop.Id = "test.troop_drop";
    Drop.Kind = ProtocolPowerKind::TroopDrop;
    Drop.CooldownTicks = 100;
    Drop.Damage = 4; // unit count per the TroopDrop convention
    Drop.Radius = Fixed::FromInt(250);
    Drop.DeployUnitId = TankDef;
    Protocols.RegisterProtocol(Drop);
    UnlockTestPower(Protocols, "test.troop_drop");

    const Vec2 Target(Fixed::FromInt(4000), Fixed::FromInt(4000));
    RA4_EXPECT_EQ(CountEntitiesOfType(World, 0, TankDef), 0);
    RA4_EXPECT_EQ(Protocols.CastPower(0, "test.troop_drop", Target, World), true);

    // Exactly N troopers landed...
    RA4_EXPECT_EQ(CountEntitiesOfType(World, 0, TankDef), 4);

    // ...all owned by the caster and inside the scatter disc. NextUnitFixed()
    // yields [0,1) so the distance is strictly below Radius.
    const Fixed RadiusSq = Drop.Radius * Drop.Radius;
    int32_t Dropped = 0;
    const std::vector<EntityCore>& Cores = World.GetAllCores();
    for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
    {
        if (!Cores[I].bAlive || Cores[I].Owner != 0 || Cores[I].Def != TankDef)
        {
            continue;
        }
        const TransformComp* T = World.GetTransform(World.MakeId(I));
        RA4_REQUIRE(T != nullptr);
        RA4_EXPECT((T->Position - Target).LengthSquared() <= RadiusSq);
        ++Dropped;
    }
    RA4_EXPECT_EQ(Dropped, 4);

    // Cooldown applies to the new kind like any other power.
    RA4_EXPECT_EQ(Protocols.CastPower(0, "test.troop_drop", Target, World), false);
}

RA4_TEST(Protocols, PhaseFieldShieldsFriendliesInsideRadius)
{
    auto Content = MakeProtocolTestContent();
    MatchSetup Setup = MakeTestSetup(204);

    SimWorld World;
    World.Initialize(Content.get(), Setup);
    World.SpawnBuilding(MakeContentId("building.sov.conyard"), 0, TileCoord(2, 2), true);
    World.SpawnBuilding(MakeContentId("building.sov.conyard"), 1, TileCoord(50, 50), true);

    const Vec2 Target(Fixed::FromInt(4000), Fixed::FromInt(4000));
    const ContentId TankDef = MakeContentId("unit.sov.heavy_tank");
    const ContentId GunnerDef = MakeContentId("unit.test.gunner");

    // Phased friendly sits at the target under the gun of an adjacent enemy;
    // an unphased friend stands far outside the field as the control group.
    const EntityId Phased = World.SpawnUnit(TankDef, 0, Target);
    const EntityId Exposed = World.SpawnUnit(TankDef, 0, Vec2(Fixed::FromInt(8000), Fixed::FromInt(4000)));
    World.SpawnUnit(GunnerDef, 1, Vec2(Fixed::FromInt(4150), Fixed::FromInt(4000)));

    ProtocolRuntime Protocols;
    ProtocolPowerDef Field;
    Field.Id = "test.phase_field";
    Field.Kind = ProtocolPowerKind::PhaseField;
    Field.CooldownTicks = 100;
    Field.Radius = Fixed::FromInt(400);
    Field.StatusDurationTicks = 300; // outlives the combat window below
    Protocols.RegisterProtocol(Field);
    UnlockTestPower(Protocols, "test.phase_field");

    RA4_EXPECT_EQ(Protocols.CastPower(0, "test.phase_field", Target, World), true);

    const StatusComp* PhasedStatus = World.GetStatus(Phased);
    RA4_REQUIRE(PhasedStatus != nullptr);
    RA4_EXPECT(PhasedStatus->InvulnerableTicks > 0);

    const StatusComp* ExposedStatus = World.GetStatus(Exposed);
    RA4_REQUIRE(ExposedStatus != nullptr);
    RA4_EXPECT_EQ(ExposedStatus->InvulnerableTicks, 0);

    // Live fire is the honest check: ApplyDamage honours InvulnerableTicks,
    // whereas DebugDamage bypasses every status by design (it is a debug tool).
    // The window covers target acquisition, turret alignment and several shots.
    for (int32_t I = 0; I < 120; ++I)
    {
        World.Tick(nullptr);
    }

    RA4_EXPECT_EQ(World.IsAlive(Phased), true);
    const HealthComp* PhasedHealth = World.GetHealth(Phased);
    RA4_REQUIRE(PhasedHealth != nullptr);
    RA4_EXPECT_EQ(PhasedHealth->Current, 1000); // untouched through the whole barrage

    // Control: the same damage tool kills the unprotected unit outright.
    World.DebugDamage(Exposed, 9999);
    World.Tick(nullptr);
    RA4_EXPECT_EQ(World.IsAlive(Exposed), false);
}

RA4_TEST(Protocols, EmpPulseStunsOnlyHostilesInRadius)
{
    auto Content = MakeProtocolTestContent();
    MatchSetup Setup = MakeTestSetup(205);

    SimWorld World;
    World.Initialize(Content.get(), Setup);
    World.SpawnBuilding(MakeContentId("building.sov.conyard"), 0, TileCoord(2, 2), true);
    World.SpawnBuilding(MakeContentId("building.sov.conyard"), 1, TileCoord(50, 50), true);

    const ContentId TankDef = MakeContentId("unit.sov.heavy_tank");
    const EntityId EnemyInRadius = World.SpawnUnit(TankDef, 1, Vec2(Fixed::FromInt(4000), Fixed::FromInt(4000)));
    const EntityId FriendInRadius = World.SpawnUnit(TankDef, 0, Vec2(Fixed::FromInt(4200), Fixed::FromInt(4000)));
    const EntityId EnemyOutside = World.SpawnUnit(TankDef, 1, Vec2(Fixed::FromInt(9000), Fixed::FromInt(9000)));

    ProtocolRuntime Protocols;
    ProtocolPowerDef Pulse;
    Pulse.Id = "test.emp_pulse";
    Pulse.Kind = ProtocolPowerKind::EmpPulse;
    Pulse.CooldownTicks = 100;
    Pulse.Radius = Fixed::FromInt(350);
    Pulse.StatusDurationTicks = 80;
    Protocols.RegisterProtocol(Pulse);
    UnlockTestPower(Protocols, "test.emp_pulse");

    RA4_EXPECT_EQ(Protocols.CastPower(0, "test.emp_pulse", Vec2(Fixed::FromInt(4000), Fixed::FromInt(4000)), World), true);

    const StatusComp* Stunned = World.GetStatus(EnemyInRadius);
    RA4_REQUIRE(Stunned != nullptr);
    RA4_EXPECT_EQ(Stunned->StunTicks, 80);
    RA4_EXPECT_EQ(Stunned->bCanAct(), false); // cannot fire while stunned

    const StatusComp* Spared = World.GetStatus(FriendInRadius);
    RA4_REQUIRE(Spared != nullptr);
    RA4_EXPECT_EQ(Spared->StunTicks, 0);
    RA4_EXPECT_EQ(Spared->bCanAct(), true); // bEnemiesOnly protects the caster's own side

    const StatusComp* Far = World.GetStatus(EnemyOutside);
    RA4_REQUIRE(Far != nullptr);
    RA4_EXPECT_EQ(Far->bCanAct(), true);
}

RA4_TEST(Protocols, SalvageBountyPaysCreditsOnCombatKill)
{
    auto Content = MakeProtocolTestContent();
    MatchSetup Setup = MakeTestSetup(206);

    SimWorld World;
    World.Initialize(Content.get(), Setup);
    World.SpawnBuilding(MakeContentId("building.sov.conyard"), 0, TileCoord(2, 2), true);
    World.SpawnBuilding(MakeContentId("building.sov.conyard"), 1, TileCoord(50, 50), true);

    const ContentId VictimDef = MakeContentId("unit.sov.heavy_tank"); // unarmed, Cost = 1200
    const ContentId GunnerDef = MakeContentId("unit.test.gunner");
    const EntityId Victim = World.SpawnUnit(VictimDef, 1, Vec2(Fixed::FromInt(4000), Fixed::FromInt(4000)));
    const EntityId Gunner =
        World.SpawnUnit(GunnerDef, 0, Vec2(Fixed::FromInt(4150), Fixed::FromInt(4000)));

    ProtocolRuntime Protocols;
    ProtocolPowerDef Bounty;
    Bounty.Id = "test.salvage_bounty";
    Bounty.Kind = ProtocolPowerKind::Passive;
    Bounty.CreditPercentPerKill = 25;
    Protocols.RegisterProtocol(Bounty);
    UnlockTestPower(Protocols, "test.salvage_bounty");

    const int32_t CreditsBefore = World.GetPlayer(0).Credits;

    // Feed events tick by tick WITHOUT clearing: the bounty resolves from the
    // EntityDestroyed payload, which only exists until the next ClearEvents.
    bool bSawCombatDamage = false;
    for (int32_t I = 0; I < 600 && World.IsAlive(Victim); ++I)
    {
        World.Tick(nullptr);
        for (const SimEvent& Event : World.GetEvents())
        {
            if (Event.Type == SimEventType::DamageApplied && Event.Entity == Victim &&
                Event.Other == Gunner)
            {
                bSawCombatDamage = true;
            }
        }
        Protocols.ProcessSimEvents(World.GetEvents(), World);
    }

    RA4_EXPECT(bSawCombatDamage);
    RA4_EXPECT_EQ(World.IsAlive(Victim), false);
    // 25% of the victim's Production.Cost (1200), paid once for one kill.
    RA4_EXPECT_EQ(World.GetPlayer(0).Credits - CreditsBefore, 300);
}
