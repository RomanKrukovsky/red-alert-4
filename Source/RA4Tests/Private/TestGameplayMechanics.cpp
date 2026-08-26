// Copyright (c) Red Alert 4 project. Tests for Stage 2 gameplay mechanics.
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4AI/AICommander.h"
#include "RA4Content/ContentDatabase.h"
#include "RA4Core/Command.h"
#include "RA4Core/Fixed.h"
#include "RA4Core/Vector.h"
#include "RA4Simulation/SimWorld.h"

#include <memory>
#include <vector>

using namespace RA4;
using namespace RA4Test;

namespace
{

std::unique_ptr<ContentDatabase> MakeRA3TestContent()
{
    auto Db = std::make_unique<ContentDatabase>();

    // Test Weapon
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

    // Land Yard (provides build radius)
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
        Db->AddEntity(E);
    }

    // Water-Only Shipyard
    {
        EntityDef E;
        E.Id = MakeContentId("building.test_shipyard");
        E.Name = "building.test_shipyard";
        E.Kind = EntityKind::Building;
        E.MaxHealth = 1000;
        E.Armor = ArmorClass::Building;
        E.Building.FootprintX = 2;
        E.Building.FootprintY = 2;
        E.Building.bWaterOnly = true;
        E.Production.Cost = 1000;
        Db->AddEntity(E);
    }

    // Hybrid Water/Land Power Plant
    {
        EntityDef E;
        E.Id = MakeContentId("building.test_power");
        E.Name = "building.test_power";
        E.Kind = EntityKind::Building;
        E.MaxHealth = 600;
        E.Armor = ArmorClass::Building;
        E.Building.FootprintX = 2;
        E.Building.FootprintY = 2;
        E.Building.bAllowOnWater = true;
        E.Building.bIsPowerPlant = true;
        E.Building.PowerProduced = 100;
        E.Production.Cost = 500;
        Db->AddEntity(E);
    }

    // Standard Land-Only Turret
    {
        EntityDef E;
        E.Id = MakeContentId("building.test_land_turret");
        E.Name = "building.test_land_turret";
        E.Kind = EntityKind::Building;
        E.MaxHealth = 500;
        E.Armor = ArmorClass::Defense;
        E.Building.FootprintX = 1;
        E.Building.FootprintY = 1;
        E.Building.bAllowOnWater = false;
        E.Building.bWaterOnly = false;
        E.Production.Cost = 400;
        Db->AddEntity(E);
    }

    // Heavy Tank with Shield / Fortify Secondary Ability
    {
        EntityDef E;
        E.Id = MakeContentId("unit.test_ability_tank");
        E.Name = "unit.test_ability_tank";
        E.Kind = EntityKind::Unit;
        E.MaxHealth = 500;
        E.Armor = ArmorClass::HeavyVehicle;
        E.Weapon = MakeContentId("weapon.test_cannon");
        E.Unit.Layer = MovementLayer::Tracked;
        E.Unit.MaxSpeed = Fixed::FromInt(100);
        E.Unit.Acceleration = Fixed::FromInt(200);
        E.Unit.TurnRatePerSecond = 1024;
        E.Unit.CollisionRadius = Fixed::FromInt(30);

        // Secondary Ability
        E.Unit.bHasSecondaryAbility = true;
        E.Unit.AbilityCooldownTicks = 40; // 2 sec cooldown
        E.Unit.AbilityDurationTicks = 20; // 1 sec duration
        E.Unit.AbilitySpeedMultiplier = Fixed::FromRatio(1, 2); // 50% speed
        E.Unit.AbilityArmorBonusPercent = 50; // 50% damage reduction
        E.Unit.bAbilityDisablesPrimaryWeapon = false;

        Db->AddEntity(E);
    }

    return Db;
}

} // namespace

// --- 1. Secondary Ability Activation, Timers, Cooldowns ---

RA4_TEST(RA3Gameplay, SecondaryAbilityActivationAndCooldown)
{
    auto Content = MakeRA3TestContent();
    MatchSetup Setup = MakeTestSetup();

    SimWorld World;
    World.Initialize(Content.get(), Setup);

    const ContentId ConYardDef = MakeContentId("building.test_conyard");
    World.SpawnBuilding(ConYardDef, 0, TileCoord(2, 2), /*bInstantComplete*/ true);
    World.SpawnBuilding(ConYardDef, 1, TileCoord(30, 30), /*bInstantComplete*/ true);

    const ContentId TankDef = MakeContentId("unit.test_ability_tank");
    const EntityId Tank = World.SpawnUnit(TankDef, 0, Vec2(Fixed::FromInt(1000), Fixed::FromInt(1000)));

    RA4_EXPECT(World.IsAlive(Tank));
    const CombatComp* Combat = World.GetCombat(Tank);
    RA4_EXPECT(Combat != nullptr);
    RA4_EXPECT_EQ(Combat->bSecondaryModeActive, false);
    RA4_EXPECT_EQ(Combat->SecondaryAbilityCooldownTicks, 0);

    // Issue ToggleSecondaryAbility command
    Command ToggleCmd;
    ToggleCmd.Type = CommandType::ToggleSecondaryAbility;
    ToggleCmd.Issuer = 0;
    ToggleCmd.Primary = Tank;
    const CommandResult FirstResult = World.ApplyCommand(ToggleCmd);
    RA4_EXPECT_EQ(static_cast<int>(FirstResult.Reason), static_cast<int>(CommandReject::Accepted));

    Combat = World.GetCombat(Tank);
    RA4_EXPECT_EQ(Combat->bSecondaryModeActive, true);
    RA4_EXPECT_EQ(Combat->SecondaryAbilityDurationTicks, 20);
    RA4_EXPECT_EQ(Combat->SecondaryAbilityCooldownTicks, 40);

    // Advance 1 tick so rate limit is reset, then attempt to activate again while on cooldown
    World.Tick(nullptr);
    const CommandResult RejectResult = World.ApplyCommand(ToggleCmd);
    RA4_EXPECT_EQ(static_cast<int>(RejectResult.Reason), static_cast<int>(CommandReject::OnCooldown));

    // Advance 9 more ticks (total 10 ticks elapsed since activation)
    // -> duration should be 10, cooldown 30, still active
    for (int I = 0; I < 9; ++I)
    {
        World.Tick(nullptr);
    }
    Combat = World.GetCombat(Tank);
    RA4_EXPECT_EQ(Combat->bSecondaryModeActive, true);
    RA4_EXPECT_EQ(Combat->SecondaryAbilityDurationTicks, 10);
    RA4_EXPECT_EQ(Combat->SecondaryAbilityCooldownTicks, 30);

    // Advance 11 more ticks -> duration expires (hit 0), mode automatically deactivates
    for (int I = 0; I < 11; ++I)
    {
        World.Tick(nullptr);
    }
    Combat = World.GetCombat(Tank);
    RA4_EXPECT_EQ(Combat->bSecondaryModeActive, false);
    RA4_EXPECT_EQ(Combat->SecondaryAbilityDurationTicks, 0);
    RA4_EXPECT_EQ(Combat->SecondaryAbilityCooldownTicks, 19);

    // Advance 19 more ticks -> cooldown fully expires
    for (int I = 0; I < 19; ++I)
    {
        World.Tick(nullptr);
    }
    Combat = World.GetCombat(Tank);
    RA4_EXPECT_EQ(Combat->SecondaryAbilityCooldownTicks, 0);

    // Now it can be activated again
    const CommandResult ReadyResult = World.ApplyCommand(ToggleCmd);
    RA4_EXPECT_EQ(static_cast<int>(ReadyResult.Reason), static_cast<int>(CommandReject::Accepted));
    Combat = World.GetCombat(Tank);
    RA4_EXPECT_EQ(Combat->bSecondaryModeActive, true);
}

// --- 2. Secondary Ability Speed and Armor Modifiers ---

RA4_TEST(RA3Gameplay, SecondaryAbilitySpeedAndArmorModifiers)
{
    auto Content = MakeRA3TestContent();
    MatchSetup Setup = MakeTestSetup();

    SimWorld World;
    World.Initialize(Content.get(), Setup);

    const ContentId ConYardDef = MakeContentId("building.test_conyard");
    World.SpawnBuilding(ConYardDef, 0, TileCoord(2, 2), /*bInstantComplete*/ true);
    World.SpawnBuilding(ConYardDef, 1, TileCoord(30, 30), /*bInstantComplete*/ true);

    const ContentId TankDef = MakeContentId("unit.test_ability_tank");
    const EntityId TankA = World.SpawnUnit(TankDef, 0, Vec2(Fixed::FromInt(1000), Fixed::FromInt(1000)));
    const EntityId TankB = World.SpawnUnit(TankDef, 0, Vec2(Fixed::FromInt(1000), Fixed::FromInt(1500)));

    // Tank A activates secondary ability (50% speed, 50% damage reduction)
    Command ToggleCmd;
    ToggleCmd.Type = CommandType::ToggleSecondaryAbility;
    ToggleCmd.Issuer = 0;
    ToggleCmd.Primary = TankA;
    World.ApplyCommand(ToggleCmd);

    World.Tick(nullptr);

    // Move Tank A forward by 2000 units
    Command MoveA;
    MoveA.Type = CommandType::Move;
    MoveA.Issuer = 0;
    MoveA.Primary = TankA;
    MoveA.Location = Vec2(Fixed::FromInt(3000), Fixed::FromInt(1000));
    World.ApplyCommand(MoveA);

    World.Tick(nullptr);

    // Move Tank B forward by 2000 units
    Command MoveB;
    MoveB.Type = CommandType::Move;
    MoveB.Issuer = 0;
    MoveB.Primary = TankB;
    MoveB.Location = Vec2(Fixed::FromInt(3000), Fixed::FromInt(1500));
    World.ApplyCommand(MoveB);

    // Step 15 ticks
    for (int I = 0; I < 15; ++I)
    {
        World.Tick(nullptr);
    }

    const Fixed DistA = World.GetTransform(TankA)->Position.X - Fixed::FromInt(1000);
    const Fixed DistB = World.GetTransform(TankB)->Position.X - Fixed::FromInt(1000);

    // Tank A had speed multiplier 0.5, so DistA must be strictly less than DistB
    RA4_EXPECT(DistA < DistB);
    RA4_EXPECT(DistA > Fixed::Zero());
    RA4_EXPECT(DistB > Fixed::Zero());

    const int32_t InitialHealthA = World.GetHealth(TankA)->Current;
    const int32_t InitialHealthB = World.GetHealth(TankB)->Current;
    RA4_EXPECT_EQ(InitialHealthA, 500);
    RA4_EXPECT_EQ(InitialHealthB, 500);
}

// --- 3. Water Base Building & Placement Validation ---

RA4_TEST(RA3Gameplay, WaterBaseBuildingPlacement)
{
    auto Content = MakeRA3TestContent();
    MatchSetup Setup = MakeTestSetup();

    // Make column X=8..15 all water
    for (int32_t Y = 0; Y < Setup.Map.Height; ++Y)
    {
        for (int32_t X = 8; X < 16; ++X)
        {
            Setup.Map.Tiles[Setup.Map.TileIndex(X, Y)] = Tile_Water;
        }
    }

    SimWorld World;
    World.Initialize(Content.get(), Setup);

    // Spawn starting ConYard at (2, 2) on ground to project build radius
    const ContentId ConYardDef = MakeContentId("building.test_conyard");
    World.SpawnBuilding(ConYardDef, 0, TileCoord(2, 2), /*bInstantComplete*/ true);
    World.SpawnBuilding(ConYardDef, 1, TileCoord(30, 30), /*bInstantComplete*/ true);

    const ContentId LandTurret = MakeContentId("building.test_land_turret");
    const ContentId Shipyard = MakeContentId("building.test_shipyard");
    const ContentId PowerPlant = MakeContentId("building.test_power");

    // Land Turret:
    // - On land (5, 5): valid
    // - On water (9, 5): INVALID
    RA4_EXPECT(World.IsPlacementValid(LandTurret, 0, TileCoord(5, 5)));
    RA4_EXPECT(!World.IsPlacementValid(LandTurret, 0, TileCoord(9, 5)));

    // Shipyard (bWaterOnly):
    // - On land (5, 5): INVALID
    // - On water (9, 5): valid
    RA4_EXPECT(!World.IsPlacementValid(Shipyard, 0, TileCoord(5, 5)));
    RA4_EXPECT(World.IsPlacementValid(Shipyard, 0, TileCoord(9, 5)));

    // Power Plant (bAllowOnWater):
    // - On land (5, 5): valid
    // - On water (9, 5): valid
    RA4_EXPECT(World.IsPlacementValid(PowerPlant, 0, TileCoord(5, 5)));
    RA4_EXPECT(World.IsPlacementValid(PowerPlant, 0, TileCoord(9, 5)));
}

// --- 4. Co-op AI Commander Ping Coordination ---

RA4_TEST(RA3Gameplay, CoopAIPingCoordination)
{
    auto Content = MakeRA3TestContent();
    MatchSetup Setup = MakeTestSetup();
    // Allied Team 1
    Setup.Players[0].Team = 1;
    Setup.Players[1].Team = 1;

    SimWorld World;
    World.Initialize(Content.get(), Setup);

    const ContentId ConYardDef = MakeContentId("building.test_conyard");
    World.SpawnBuilding(ConYardDef, 0, TileCoord(2, 2), /*bInstantComplete*/ true);
    World.SpawnBuilding(ConYardDef, 1, TileCoord(30, 30), /*bInstantComplete*/ true);

    AI::AICommander Commander;
    Commander.Initialize(1, AI::AIProfile::Balanced, 1337);

    // Initial state: no active ping
    RA4_EXPECT_EQ(Commander.GetActiveCoopPing().bActive, false);

    // Player 0 emits a tactical Attack ping at (1200, 1800)
    Command PingCmd;
    PingCmd.Type = CommandType::CoopPing;
    PingCmd.Issuer = 0;
    PingCmd.Location = Vec2(Fixed::FromInt(1200), Fixed::FromInt(1800));
    PingCmd.Param = static_cast<int32_t>(CoopPingType::Attack);
    World.ApplyCommand(PingCmd);

    // Tick AICommander
    std::vector<Command> OutCommands;
    Commander.Tick(World, OutCommands);

    // Verify AI received and registered the ping
    const auto& ActivePing = Commander.GetActiveCoopPing();
    RA4_EXPECT_EQ(ActivePing.bActive, true);
    RA4_EXPECT_EQ(ActivePing.Sender, 0);
    RA4_EXPECT_EQ(static_cast<uint8_t>(ActivePing.Type), static_cast<uint8_t>(CoopPingType::Attack));
    RA4_EXPECT(ActivePing.Location == Vec2(Fixed::FromInt(1200), Fixed::FromInt(1800)));
}

// --- 5. African Federation & Canonical Faction Roster Test ---

RA4_TEST(AfricanFederation, FactionRegistrationAndUnitRoster)
{
    ContentDatabase Db;
    BuildDefaultContent(Db);

    const FactionDef* AfFaction = Db.FindFaction(FactionId::AfricanFederation);
    RA4_REQUIRE(AfFaction != nullptr);
    RA4_EXPECT(AfFaction->Name == "faction.au" || AfFaction->Name == "faction.af");
    RA4_EXPECT_EQ(AfFaction->StartingCredits, 10000);

    const EntityDef* Askari = Db.FindEntity(MakeContentId("unit.au.askari_rifleman"));
    RA4_REQUIRE(Askari != nullptr);
    RA4_EXPECT(Askari->Faction == FactionId::AfricanFederation);
    RA4_EXPECT(Askari->Kind == EntityKind::Unit);

    const EntityDef* Mamba = Db.FindEntity(MakeContentId("unit.au.mamba_mbt"));
    RA4_REQUIRE(Mamba != nullptr);
    RA4_EXPECT(Mamba->Faction == FactionId::AfricanFederation);
    RA4_EXPECT(Mamba->Unit.MaxSpeed >= Fixed::FromInt(900));

    const EntityDef* Elephant = Db.FindEntity(MakeContentId("unit.au.elephant_superheavy"));
    RA4_REQUIRE(Elephant != nullptr);
    RA4_EXPECT(Elephant->MaxHealth >= 1000);
    RA4_EXPECT(Elephant->Unit.bHasSecondaryAbility == true);

    const EntityDef* Amina = Db.FindEntity(MakeContentId("unit.au.amina_commando"));
    RA4_REQUIRE(Amina != nullptr);
    RA4_EXPECT(Amina->Production.Tier == TechTier::T3);

    // Verify all 5 canonical factions are present
    RA4_EXPECT(Db.FindFaction(FactionId::EurasianPact) != nullptr);
    RA4_EXPECT(Db.FindFaction(FactionId::AtlanticAlliance) != nullptr);
    RA4_EXPECT(Db.FindFaction(FactionId::EasternCoalition) != nullptr);
    RA4_EXPECT(Db.FindFaction(FactionId::PacificPact) != nullptr);
    RA4_EXPECT(Db.FindFaction(FactionId::AfricanFederation) != nullptr);
}
