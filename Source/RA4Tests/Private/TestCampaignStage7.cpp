// Copyright (c) Red Alert 4 project. Tests for Stage 7 (Scripted Campaign Triggers, Actions & Cinematic Runtime).
#include "TestFramework.h"
#include "TestHelpers.h"

#include "CampaignDatabase.h"
#include "CampaignScriptTypes.h"
#include "MissionRuntime.h"
#include "MissionScriptRuntime.h"
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

std::unique_ptr<ContentDatabase> MakeCampaignTestContent()
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
        E.Building.bProvidesBuildRadius = true;
        E.Building.BuildRadius = Fixed::FromInt(2000);
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

// --- 1. Timed Reinforcements & Cinematic Transmissions ---

RA4_TEST(CampaignStage7, TimedReinforcementsAndTransmissions)
{
    auto Content = MakeCampaignTestContent();
    MatchSetup Setup = MakeTestSetup(101);

    SimWorld World;
    World.Initialize(Content.get(), Setup);

    const ContentId ConYard = MakeContentId("building.sov.conyard");
    World.SpawnBuilding(ConYard, 0, TileCoord(2, 2), true);
    World.SpawnBuilding(ConYard, 1, TileCoord(50, 50), true);

    MissionRuntime ObjRuntime;
    CampaignMissionDef Mission;
    Mission.MissionId = "sov_01_scripted";
    ObjRuntime.Begin(Mission, 0);

    MissionScriptRuntime Script;
    Script.Initialize(&ObjRuntime);

    // Trigger: at tick 20, spawn 2 tanks and play cinematic transmission
    MissionTrigger TimedTrig;
    TimedTrig.Id = "trig_reinforcements";
    TimedTrig.Condition = TriggerConditionType::TickReached;
    TimedTrig.TriggerTick = 20;

    ScriptTriggerAction SpawnAct;
    SpawnAct.Type = TriggerActionType::SpawnReinforcements;
    const ContentId TankDef = MakeContentId("unit.sov.heavy_tank");
    SpawnAct.Reinforcements.push_back({TankDef, Vec2(Fixed::FromInt(1000), Fixed::FromInt(1000)), 0});
    SpawnAct.Reinforcements.push_back({TankDef, Vec2(Fixed::FromInt(1100), Fixed::FromInt(1000)), 0});

    ScriptTriggerAction CommsAct;
    CommsAct.Type = TriggerActionType::PlayCinematicTransmission;
    CommsAct.SpeakerName = "General Krukov";
    CommsAct.DialogueTextKey = "dialogue.sov.reinforcements_arrived";
    CommsAct.PortraitVideoId = "video.portrait.krukov";
    CommsAct.DurationTicks = 50;

    TimedTrig.Actions.push_back(SpawnAct);
    TimedTrig.Actions.push_back(CommsAct);
    Script.AddTrigger(TimedTrig);

    // Simulate 10 ticks: trigger must not have fired yet
    for (int I = 0; I < 10; ++I)
    {
        World.Tick(nullptr);
        Script.Tick(World);
    }

    RA4_EXPECT_EQ(Script.GetTriggers()[0].bFired, false);
    RA4_EXPECT(Script.GetActiveTransmission() == nullptr);

    // Simulate to tick 25: trigger must fire, 2 units spawned, transmission active
    for (int I = 0; I < 15; ++I)
    {
        World.Tick(nullptr);
        Script.Tick(World);
    }

    RA4_EXPECT_EQ(Script.GetTriggers()[0].bFired, true);
    const auto* Trans = Script.GetActiveTransmission();
    RA4_REQUIRE(Trans != nullptr);
    RA4_EXPECT(Trans->Speaker == "General Krukov");
    RA4_EXPECT(Trans->Text == "dialogue.sov.reinforcements_arrived");

    // Simulate to tick 80: transmission completes and records to history
    for (int I = 0; I < 55; ++I)
    {
        World.Tick(nullptr);
        Script.Tick(World);
    }

    RA4_EXPECT(Script.GetActiveTransmission() == nullptr);
    RA4_REQUIRE(Script.GetTransmissionHistory().size() == 1u);
    RA4_EXPECT(Script.GetTransmissionHistory()[0].Speaker == "General Krukov");
}

// --- 2. Area Trigger & Dynamic Objective Reveal ---

RA4_TEST(CampaignStage7, AreaTriggerAndObjectiveReveal)
{
    auto Content = MakeCampaignTestContent();
    MatchSetup Setup = MakeTestSetup(102);

    SimWorld World;
    World.Initialize(Content.get(), Setup);

    const ContentId ConYard = MakeContentId("building.sov.conyard");
    World.SpawnBuilding(ConYard, 0, TileCoord(2, 2), true);
    World.SpawnBuilding(ConYard, 1, TileCoord(50, 50), true);

    const ContentId TankDef = MakeContentId("unit.sov.heavy_tank");
    World.SpawnUnit(TankDef, 0, World.GetMap().TileCenterToWorld(TileCoord(19, 19)));

    MissionRuntime ObjRuntime;
    CampaignMissionDef Mission;
    Mission.MissionId = "sov_02_infiltration";

    // Primary objective 1: Reach Location
    MissionObjective Obj1;
    Obj1.Id = "obj_reach_alpha";
    Obj1.bIsPrimary = true;
    Obj1.State = ObjectiveState::Active;
    Obj1.Condition.Type = ObjectiveConditionType::ReachLocation;
    Obj1.Condition.Subject = 0;
    Obj1.Condition.TargetTile = TileCoord(20, 20);
    Obj1.Condition.RadiusTiles = 4;
    Mission.Objectives.push_back(Obj1);

    // Primary objective 2: Hidden until triggered
    MissionObjective Obj2;
    Obj2.Id = "obj_destroy_base";
    Obj2.bIsPrimary = true;
    Obj2.State = ObjectiveState::Hidden;
    Obj2.Condition.Type = ObjectiveConditionType::EntityCountAtMost;
    Obj2.Condition.Subject = 1;
    Obj2.Condition.Amount = 0;
    Mission.Objectives.push_back(Obj2);

    ObjRuntime.Begin(Mission, 0);

    MissionScriptRuntime Script;
    Script.Initialize(&ObjRuntime);

    // Trigger when player reaches Tile(20, 20)
    MissionTrigger AreaTrig;
    AreaTrig.Id = "trig_reach_alpha";
    AreaTrig.Condition = TriggerConditionType::AreaEntered;
    AreaTrig.ConditionPlayer = 0;
    AreaTrig.AreaCenter = TileCoord(20, 20);
    AreaTrig.AreaRadiusTiles = 4;

    ScriptTriggerAction RevealAct;
    RevealAct.Type = TriggerActionType::RevealObjective;
    RevealAct.TargetObjectiveId = "obj_destroy_base";
    AreaTrig.Actions.push_back(RevealAct);
    Script.AddTrigger(AreaTrig);

    // Evaluate world tick
    World.Tick(nullptr);
    ObjRuntime.Evaluate(World);
    Script.Tick(World);

    // Phase 1 should complete and Phase 2 should be revealed
    const auto* EvaluatedObj1 = ObjRuntime.FindObjective("obj_reach_alpha");
    RA4_REQUIRE(EvaluatedObj1 != nullptr);
    RA4_EXPECT_EQ(static_cast<uint8_t>(EvaluatedObj1->State), static_cast<uint8_t>(ObjectiveState::Completed));

    const auto* EvaluatedObj2 = ObjRuntime.FindObjective("obj_destroy_base");
    RA4_REQUIRE(EvaluatedObj2 != nullptr);
    RA4_EXPECT_EQ(static_cast<uint8_t>(EvaluatedObj2->State), static_cast<uint8_t>(ObjectiveState::Active));
}

// --- 3. Boss Destruction & Economy Threshold Triggers ---

RA4_TEST(CampaignStage7, EntityDestroyedAndCreditsTriggers)
{
    auto Content = MakeCampaignTestContent();
    MatchSetup Setup = MakeTestSetup(103);

    SimWorld World;
    World.Initialize(Content.get(), Setup);

    const ContentId ConYard = MakeContentId("building.sov.conyard");
    World.SpawnBuilding(ConYard, 0, TileCoord(2, 2), true);
    World.SpawnBuilding(ConYard, 1, TileCoord(50, 50), true);

    const ContentId TankDef = MakeContentId("unit.sov.heavy_tank");
    const EntityId BossUnit = World.SpawnUnit(TankDef, 1, Vec2(Fixed::FromInt(2000), Fixed::FromInt(2000)));

    MissionScriptRuntime Script;
    Script.Initialize(nullptr);

    // Trigger 1: Boss destroyed
    MissionTrigger DestroyTrig;
    DestroyTrig.Id = "trig_boss_destroyed";
    DestroyTrig.Condition = TriggerConditionType::EntityDestroyed;
    DestroyTrig.TargetEntity = BossUnit;

    ScriptTriggerAction CommsAct;
    CommsAct.Type = TriggerActionType::PlayCinematicTransmission;
    CommsAct.SpeakerName = "Commander Shinzo";
    CommsAct.DialogueTextKey = "dialogue.emp.boss_destroyed";
    CommsAct.DurationTicks = 30;
    DestroyTrig.Actions.push_back(CommsAct);
    Script.AddTrigger(DestroyTrig);

    // Trigger 2: Credits threshold (starting is 10000, set threshold to 15000)
    MissionTrigger CreditsTrig;
    CreditsTrig.Id = "trig_rich";
    CreditsTrig.Condition = TriggerConditionType::CreditsThreshold;
    CreditsTrig.ConditionPlayer = 0;
    CreditsTrig.HealthPercent = 15000; // Threshold = 15000 credits

    ScriptTriggerAction RewardAct;
    RewardAct.Type = TriggerActionType::SpawnReinforcements;
    RewardAct.Reinforcements.push_back({TankDef, Vec2(Fixed::FromInt(500), Fixed::FromInt(500)), 0});
    CreditsTrig.Actions.push_back(RewardAct);
    Script.AddTrigger(CreditsTrig);

    World.Tick(nullptr);
    Script.Tick(World);
    RA4_EXPECT_EQ(Script.GetTriggers()[0].bFired, false);
    RA4_EXPECT_EQ(Script.GetTriggers()[1].bFired, false);

    // Destroy boss and grant credits
    const_cast<EntityCore*>(World.GetCore(BossUnit))->bAlive = false;
    World.AddCredits(0, 6000);

    World.Tick(nullptr);
    Script.Tick(World);

    RA4_EXPECT_EQ(Script.GetTriggers()[0].bFired, true);
    RA4_EXPECT_EQ(Script.GetTriggers()[1].bFired, true);
    RA4_REQUIRE(Script.GetActiveTransmission() != nullptr);
    RA4_EXPECT(Script.GetActiveTransmission()->Speaker == "Commander Shinzo");
}
