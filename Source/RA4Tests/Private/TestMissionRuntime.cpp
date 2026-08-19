// Copyright (c) Red Alert 4 project. Tests that play campaign missions to a result.
//
// TestCampaign.cpp checks that the mission *database* has the right shape. These
// tests check that a mission is a mission: brought up in a real SimWorld, ticked, and
// driven to a win and to a loss for the reasons the designer wrote down.
//
// That distinction is the whole point of this file. A campaign of 38 rows that each
// name a map and a title passes every structural test there is, and none of it is
// playable.
#include "TestFramework.h"

#include "CampaignDatabase.h"
#include "MissionRuntime.h"
#include "RA4Content/ContentDatabase.h"

using namespace RA4;

namespace
{

/** More damage than anything in the game has hit points, so DebugDamage always kills.
    Note that destruction is deferred to the next SimWorld::Tick -- a test that damages
    something and then counts survivors without ticking still sees it alive. */
constexpr int32_t kLethal = 1000000;

/** RA4_EXPECT_EQ prints both operands with std::to_string, which has no overload for a
    scoped enum. Comparing the underlying values keeps the failure message useful. */
int32_t AsInt(MissionStatus Status)
{
    return int32_t(Status);
}

/** A two-sided mission that nobody can win by fighting: each player owns one building
    and neither can reach the other. Needed because SimWorld ends a match as soon as one
    player is left standing, and a lone-player test world satisfies that on tick one --
    so a mission tested in isolation would be reported Won by the winner path before its
    objective list was ever consulted. */
CampaignMissionDef MakeStandoff(const char* Id)
{
    CampaignMissionDef M;
    M.MissionId = Id;
    M.Setup.Seed = MakeContentId(Id).Value;
    M.Setup.MapWidth = 32;
    M.Setup.MapHeight = 32;
    M.Setup.Players[0].bActive = true;
    M.Setup.Players[0].Faction = FactionId::Soviet;
    M.Setup.Players[1].bActive = true;
    M.Setup.Players[1].Faction = FactionId::Alliance;
    M.Setup.Spawns.push_back(
        {MakeContentId("building.sov.construction_yard"), 0, TileCoord(4, 4), MissionSpawnKind::Building, 0});
    M.Setup.Spawns.push_back(
        {MakeContentId("building.all.construction_yard"), 1, TileCoord(26, 26), MissionSpawnKind::Building, 0});
    return M;
}

/** Brings a mission up exactly the way the game will: default content, the mission's
    own declared MatchSetup, then its declared spawns. */
struct MissionFixture
{
    ContentDatabase Content;
    SimWorld World;
    MissionRuntime Runtime;
    int32_t Placed = 0;

    void Open(const CampaignMissionDef& Mission, PlayerId LocalPlayer = 0)
    {
        BuildDefaultContent(Content);
        World.Initialize(&Content, BuildMissionMatchSetup(Mission));
        Placed = SpawnMissionEntities(World, Mission);
        Runtime.Begin(Mission, LocalPlayer);
    }

    /** Ticks with no commands until the mission ends or the budget runs out. Returns
        the status; a mission still InProgress means the budget ran out. */
    MissionStatus RunFor(int32_t Ticks)
    {
        for (int32_t I = 0; I < Ticks && !Runtime.IsFinished(); ++I)
        {
            World.Tick(nullptr);
            Runtime.Evaluate(World);
        }
        return Runtime.GetStatus();
    }
};

/** Kills every live entity a player owns, which is how these tests stand in for the
    player actually winning the fight. Resource nodes are left alone -- they are
    terrain, and MissionRuntime does not count them. */
int32_t WipePlayer(SimWorld& World, PlayerId Owner)
{
    int32_t Killed = 0;
    const std::vector<EntityCore>& Cores = World.GetAllCores();
    for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
    {
        const EntityCore& C = Cores[I];
        if (!C.bAlive || C.Owner != Owner || C.Kind == EntityKind::ResourceNode ||
            C.Kind == EntityKind::Projectile)
        {
            continue;
        }
        World.DebugDamage(World.MakeId(I), kLethal);
        ++Killed;
    }
    return Killed;
}

int32_t CountLive(const SimWorld& World, PlayerId Owner)
{
    int32_t Count = 0;
    const std::vector<EntityCore>& Cores = World.GetAllCores();
    for (size_t I = 0; I < Cores.size(); ++I)
    {
        const EntityCore& C = Cores[I];
        if (C.bAlive && C.Owner == Owner && C.Kind != EntityKind::ResourceNode &&
            C.Kind != EntityKind::Projectile)
        {
            ++Count;
        }
    }
    return Count;
}

/** Kills every live entity of one type a player owns, and nothing else. This is how a
    test satisfies a destroy-these objective without also satisfying the simulation's
    own elimination check -- SimWorld's victory system outranks the objective list, so
    wiping the enemy outright wins the mission before any objective is looked at, and
    the objective under test never gets evaluated. */
int32_t DestroyEveryOneOf(SimWorld& World, PlayerId Owner, ContentId Def)
{
    int32_t Killed = 0;
    const std::vector<EntityCore>& Cores = World.GetAllCores();
    for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
    {
        const EntityCore& C = Cores[I];
        if (C.bAlive && C.Owner == Owner && C.Def == Def)
        {
            World.DebugDamage(World.MakeId(I), kLethal);
            ++Killed;
        }
    }
    return Killed;
}

} // namespace

// --- The data itself --------------------------------------------------------

RA4_TEST(MissionRuntime, NoMissionShipsAnUnauthoredObjective)
{
    // The regression this file exists for. Every mission used to carry a single
    // placeholder objective with no condition attached; ObjectiveConditionType::None
    // is never satisfiable, so such a mission cannot be finished by playing it.
    CampaignDatabase Db;
    int32_t Missions = 0;
    int32_t Unauthored = 0;
    int32_t WithoutPrimary = 0;

    for (const CampaignChapterDef& Chapter : Db.GetChapters())
    {
        for (const CampaignMissionDef& Mission : Chapter.Missions)
        {
            ++Missions;
            bool bHasVisiblePrimary = false;
            for (const MissionObjective& Objective : Mission.Objectives)
            {
                if (Objective.Condition.Type == ObjectiveConditionType::None)
                {
                    ++Unauthored;
                }
                if (Objective.bIsPrimary && Objective.State != ObjectiveState::Hidden)
                {
                    bHasVisiblePrimary = true;
                }
            }
            if (!bHasVisiblePrimary)
            {
                ++WithoutPrimary;
            }
        }
    }

    RA4_EXPECT_EQ(Missions, 38);
    RA4_EXPECT_EQ(Unauthored, 0);
    RA4_EXPECT_EQ(WithoutPrimary, 0);
}

RA4_TEST(MissionRuntime, EveryMissionCanBeLost)
{
    // A mission with no failure condition and no simulation-declared defeat is one
    // the player cannot lose, which makes every objective in it optional.
    CampaignDatabase Db;
    int32_t WithoutFailure = 0;
    for (const CampaignChapterDef& Chapter : Db.GetChapters())
    {
        for (const CampaignMissionDef& Mission : Chapter.Missions)
        {
            if (Mission.FailureConditions.empty())
            {
                ++WithoutFailure;
            }
        }
    }
    RA4_EXPECT_EQ(WithoutFailure, 0);
}

RA4_TEST(MissionRuntime, EveryMissionDeclaresItsOwnMatch)
{
    // A mission that only names a .umap cannot be brought up headlessly, and the
    // campaign shipped for a long time in exactly that state.
    CampaignDatabase Db;
    int32_t WithoutSpawns = 0;
    int32_t WithoutPlayerSlot = 0;
    for (const CampaignChapterDef& Chapter : Db.GetChapters())
    {
        for (const CampaignMissionDef& Mission : Chapter.Missions)
        {
            if (Mission.Setup.Spawns.empty())
            {
                ++WithoutSpawns;
            }
            if (!Mission.Setup.Players[0].bActive)
            {
                ++WithoutPlayerSlot;
            }
        }
    }
    RA4_EXPECT_EQ(WithoutSpawns, 0);
    RA4_EXPECT_EQ(WithoutPlayerSlot, 0);
}

RA4_TEST(MissionRuntime, MissionSeedsAreDistinct)
{
    // Two missions sharing a seed play out the same way given the same commands.
    CampaignDatabase Db;
    std::vector<uint64_t> Seeds;
    for (const CampaignChapterDef& Chapter : Db.GetChapters())
    {
        for (const CampaignMissionDef& Mission : Chapter.Missions)
        {
            Seeds.push_back(Mission.Setup.Seed);
        }
    }
    std::sort(Seeds.begin(), Seeds.end());
    RA4_EXPECT(std::adjacent_find(Seeds.begin(), Seeds.end()) == Seeds.end());
}

// --- Bringing a mission up ---------------------------------------------------

RA4_TEST(MissionRuntime, SovietOpenerPlacesItsDeclaredForce)
{
    CampaignDatabase Db;
    const CampaignMissionDef* Mission = Db.FindMission("sov_mission_1");
    RA4_REQUIRE(Mission != nullptr);

    MissionFixture Fix;
    Fix.Open(*Mission);

    // Every spawn the mission declares resolves against default content. If a content
    // id is renamed out from under the campaign this is the test that says so.
    RA4_EXPECT_EQ(Fix.Placed, int32_t(Mission->Setup.Spawns.size()));
    RA4_EXPECT(CountLive(Fix.World, 0) > 0);
    RA4_EXPECT(CountLive(Fix.World, 1) > 0);
    RA4_EXPECT_EQ(AsInt(Fix.Runtime.GetStatus()), AsInt(MissionStatus::InProgress));
}

RA4_TEST(MissionRuntime, EveryMissionAcrossAllFourFactionsPlacesEverySpawn)
{
    // All four faction chapters are fully authored and playable against DefaultContent.
    CampaignDatabase Db;
    const FactionId Playable[4] = {
        FactionId::Soviet,
        FactionId::Alliance,
        FactionId::EasternCoalition,
        FactionId::ChronoLegion
    };

    for (FactionId Faction : Playable)
    {
        const CampaignChapterDef* Chapter = Db.FindChapter(Faction);
        RA4_REQUIRE(Chapter != nullptr);
        for (const CampaignMissionDef& Mission : Chapter->Missions)
        {
            MissionFixture Fix;
            Fix.Open(Mission);
            RA4_EXPECT_EQ(Fix.Placed, int32_t(Mission.Setup.Spawns.size()));
            RA4_EXPECT(CountLive(Fix.World, 0) > 0);
            RA4_EXPECT(CountLive(Fix.World, 1) > 0);
        }
    }
}

// --- Playing one to a result -------------------------------------------------

RA4_TEST(MissionRuntime, DestroyingTheEnemyYardWinsTheBaseBuildingMission)
{
    CampaignDatabase Db;
    const CampaignMissionDef* Mission = Db.FindMission("sov_mission_2");
    RA4_REQUIRE(Mission != nullptr);

    MissionFixture Fix;
    Fix.Open(*Mission);

    // The mission is not won on arrival, and not won just by ticking.
    Fix.RunFor(20);
    RA4_EXPECT_EQ(AsInt(Fix.Runtime.GetStatus()), AsInt(MissionStatus::InProgress));

    // Standing in for the player winning the fight. Only the yard the objective names
    // goes down -- the enemy garrison is left standing on purpose, so victory has to
    // come from the objective rather than from the simulation's elimination check.
    RA4_EXPECT(DestroyEveryOneOf(Fix.World, 1, MakeContentId("building.all.construction_yard")) > 0);
    // The build objective is satisfied by hand for the same reason.
    Fix.World.SpawnBuilding(MakeContentId("building.sov.ore_refinery"), 0, TileCoord(20, 20), true);

    RA4_EXPECT_EQ(AsInt(Fix.RunFor(40)), AsInt(MissionStatus::Won));
    RA4_EXPECT(CountLive(Fix.World, 1) > 0);

    const MissionObjective* Built = Fix.Runtime.FindObjective("obj_build_refinery");
    RA4_REQUIRE(Built != nullptr);
    RA4_EXPECT(Built->State == ObjectiveState::Completed);

    const MissionObjective* Cleared = Fix.Runtime.FindObjective("obj_destroy_allied_outpost");
    RA4_REQUIRE(Cleared != nullptr);
    RA4_EXPECT(Cleared->State == ObjectiveState::Completed);
}

RA4_TEST(MissionRuntime, LosingTheConstructionYardLosesTheBaseBuildingMission)
{
    CampaignDatabase Db;
    const CampaignMissionDef* Mission = Db.FindMission("sov_mission_2");
    RA4_REQUIRE(Mission != nullptr);

    MissionFixture Fix;
    Fix.Open(*Mission);
    Fix.RunFor(5);
    RA4_EXPECT_EQ(AsInt(Fix.Runtime.GetStatus()), AsInt(MissionStatus::InProgress));

    // The mission's second failure clause: no construction yard.
    const std::vector<EntityCore>& Cores = Fix.World.GetAllCores();
    const ContentId Yard = MakeContentId("building.sov.construction_yard");
    for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
    {
        if (Cores[I].bAlive && Cores[I].Owner == 0 && Cores[I].Def == Yard)
        {
            Fix.World.DebugDamage(Fix.World.MakeId(I), kLethal);
        }
    }

    RA4_EXPECT_EQ(AsInt(Fix.RunFor(10)), AsInt(MissionStatus::Lost));
    // The debrief needs to be able to name why, not just that.
    RA4_EXPECT(Fix.Runtime.GetTriggeredFailure() >= 0);

    // Objectives left open are reported failed rather than frozen mid-progress.
    const MissionObjective* Cleared = Fix.Runtime.FindObjective("obj_destroy_allied_outpost");
    RA4_REQUIRE(Cleared != nullptr);
    RA4_EXPECT(Cleared->State == ObjectiveState::Failed);
}

RA4_TEST(MissionRuntime, LosingEverythingLosesAnyMission)
{
    CampaignDatabase Db;
    const CampaignMissionDef* Mission = Db.FindMission("sov_mission_1");
    RA4_REQUIRE(Mission != nullptr);

    MissionFixture Fix;
    Fix.Open(*Mission);
    RA4_EXPECT(WipePlayer(Fix.World, 0) > 0);
    RA4_EXPECT_EQ(AsInt(Fix.RunFor(10)), AsInt(MissionStatus::Lost));
}

RA4_TEST(MissionRuntime, ReachingTheObjectiveTileCompletesAReachObjective)
{
    CampaignDatabase Db;
    const CampaignMissionDef* Mission = Db.FindMission("sov_mission_5");
    RA4_REQUIRE(Mission != nullptr);

    MissionFixture Fix;
    Fix.Open(*Mission);
    Fix.RunFor(5);
    RA4_EXPECT_EQ(AsInt(Fix.Runtime.GetStatus()), AsInt(MissionStatus::InProgress));

    // Rather than drive the MCV across the map, put one where the mission wants it.
    // The condition under test is the arrival check, not the pathfinder.
    const MissionObjective* Escort = Fix.Runtime.FindObjective("obj_escort_mcv");
    RA4_REQUIRE(Escort != nullptr);
    const TileCoord Target = Escort->Condition.TargetTile;
    Fix.World.SpawnUnit(MakeContentId("unit.sov.mcv"), 0, Fix.World.GetMap().TileCenterToWorld(Target));

    RA4_EXPECT_EQ(AsInt(Fix.RunFor(10)), AsInt(MissionStatus::Won));
}

RA4_TEST(MissionRuntime, LosingTheEscortedMcvLosesTheEscortMission)
{
    CampaignDatabase Db;
    const CampaignMissionDef* Mission = Db.FindMission("sov_mission_5");
    RA4_REQUIRE(Mission != nullptr);

    MissionFixture Fix;
    Fix.Open(*Mission);

    const ContentId Mcv = MakeContentId("unit.sov.mcv");
    const std::vector<EntityCore>& Cores = Fix.World.GetAllCores();
    int32_t Destroyed = 0;
    for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
    {
        if (Cores[I].bAlive && Cores[I].Owner == 0 && Cores[I].Def == Mcv)
        {
            Fix.World.DebugDamage(Fix.World.MakeId(I), kLethal);
            ++Destroyed;
        }
    }
    RA4_EXPECT_EQ(Destroyed, 1);

    // The escort still has its tanks, so this is not the be-eliminated clause firing.
    RA4_EXPECT(CountLive(Fix.World, 0) > 0);
    RA4_EXPECT_EQ(AsInt(Fix.RunFor(10)), AsInt(MissionStatus::Lost));
}

RA4_TEST(MissionRuntime, SurviveObjectiveCompletesOnlyAfterItsDeadline)
{
    CampaignDatabase Db;
    const CampaignMissionDef* Mission = Db.FindMission("sov_mission_4");
    RA4_REQUIRE(Mission != nullptr);

    const MissionObjective* Hold = nullptr;
    for (const MissionObjective& Objective : Mission->Objectives)
    {
        if (Objective.Condition.Type == ObjectiveConditionType::SurviveTicks)
        {
            Hold = &Objective;
        }
    }
    RA4_REQUIRE(Hold != nullptr);
    const int32_t Deadline = Hold->Condition.Amount;
    RA4_REQUIRE(Deadline > 0);

    MissionFixture Fix;
    Fix.Open(*Mission);

    // One tick short is still short. This is the check that a survive objective is a
    // deadline and not a formality.
    Fix.RunFor(Deadline - 1);
    RA4_EXPECT_EQ(AsInt(Fix.Runtime.GetStatus()), AsInt(MissionStatus::InProgress));

    RA4_EXPECT_EQ(AsInt(Fix.RunFor(5)), AsInt(MissionStatus::Won));
}

RA4_TEST(MissionRuntime, ObjectiveCompletionLatches)
{
    // "Build a refinery" stays done after the refinery is shelled. Without latching a
    // mission's result would depend on which tick the last objective was checked on.
    CampaignDatabase Db;
    const CampaignMissionDef* Mission = Db.FindMission("sov_mission_2");
    RA4_REQUIRE(Mission != nullptr);

    MissionFixture Fix;
    Fix.Open(*Mission);
    const EntityId Extra =
        Fix.World.SpawnBuilding(MakeContentId("building.sov.ore_refinery"), 0, TileCoord(20, 20), true);
    RA4_REQUIRE(Extra.IsValid());

    Fix.RunFor(3);
    const MissionObjective* Built = Fix.Runtime.FindObjective("obj_build_refinery");
    RA4_REQUIRE(Built != nullptr);
    RA4_EXPECT(Built->State == ObjectiveState::Completed);

    Fix.World.DebugDamage(Extra, kLethal);
    Fix.RunFor(5);

    Built = Fix.Runtime.FindObjective("obj_build_refinery");
    RA4_REQUIRE(Built != nullptr);
    RA4_EXPECT(Built->State == ObjectiveState::Completed);
}

RA4_TEST(MissionRuntime, HiddenObjectivesDoNotGateVictoryUntilRevealed)
{
    // sov_mission_3 wins on infiltration; the extraction objective ships Hidden and is
    // revealed by the mission script. A hidden objective must neither be evaluated nor
    // required, or every staged mission would be unwinnable.
    CampaignDatabase Db;
    const CampaignMissionDef* Mission = Db.FindMission("sov_mission_3");
    RA4_REQUIRE(Mission != nullptr);

    MissionFixture Fix;
    Fix.Open(*Mission);

    const MissionObjective* Hidden = Fix.Runtime.FindObjective("obj_extract");
    RA4_REQUIRE(Hidden != nullptr);
    RA4_EXPECT(Hidden->State == ObjectiveState::Hidden);

    const MissionObjective* Infiltrate = Fix.Runtime.FindObjective("obj_infiltrate_lab");
    RA4_REQUIRE(Infiltrate != nullptr);
    Fix.World.SpawnUnit(MakeContentId("unit.sov.conscript"), 0,
                        Fix.World.GetMap().TileCenterToWorld(Infiltrate->Condition.TargetTile));

    // Won with the second stage still hidden.
    RA4_EXPECT_EQ(AsInt(Fix.RunFor(10)), AsInt(MissionStatus::Won));
}

RA4_TEST(MissionRuntime, RevealedObjectiveBecomesRequired)
{
    CampaignDatabase Db;
    const CampaignMissionDef* Mission = Db.FindMission("sov_mission_3");
    RA4_REQUIRE(Mission != nullptr);

    MissionFixture Fix;
    Fix.Open(*Mission);
    RA4_EXPECT(Fix.Runtime.RevealObjective("obj_extract"));
    // Revealing something that is already active is not a state change.
    RA4_EXPECT(!Fix.Runtime.RevealObjective("obj_extract"));

    const MissionObjective* Infiltrate = Fix.Runtime.FindObjective("obj_infiltrate_lab");
    RA4_REQUIRE(Infiltrate != nullptr);
    const EntityId Agent = Fix.World.SpawnUnit(
        MakeContentId("unit.sov.conscript"), 0,
        Fix.World.GetMap().TileCenterToWorld(Infiltrate->Condition.TargetTile));
    RA4_REQUIRE(Agent.IsValid());

    // Infiltration completes, but the revealed extraction now holds the mission open.
    Fix.RunFor(10);
    RA4_EXPECT_EQ(AsInt(Fix.Runtime.GetStatus()), AsInt(MissionStatus::InProgress));

    const MissionObjective* Done = Fix.Runtime.FindObjective("obj_infiltrate_lab");
    RA4_REQUIRE(Done != nullptr);
    RA4_EXPECT(Done->State == ObjectiveState::Completed);
}

RA4_TEST(MissionRuntime, TransitionsAreReportedOnceEach)
{
    // The HUD announces objectives off this list rather than diffing state every
    // tick, so a transition repeating would repeat the announcement.
    CampaignDatabase Db;
    const CampaignMissionDef* Mission = Db.FindMission("sov_mission_2");
    RA4_REQUIRE(Mission != nullptr);

    MissionFixture Fix;
    Fix.Open(*Mission);
    Fix.World.SpawnBuilding(MakeContentId("building.sov.ore_refinery"), 0, TileCoord(20, 20), true);
    Fix.RunFor(20);

    int32_t BuiltTransitions = 0;
    for (const ObjectiveTransition& T : Fix.Runtime.GetTransitions())
    {
        if (T.ObjectiveId == "obj_build_refinery" && T.NewState == ObjectiveState::Completed)
        {
            ++BuiltTransitions;
        }
    }
    RA4_EXPECT_EQ(BuiltTransitions, 1);

    Fix.Runtime.ClearTransitions();
    RA4_EXPECT(Fix.Runtime.GetTransitions().empty());
}

RA4_TEST(MissionRuntime, SecondaryObjectivesDoNotGateVictory)
{
    // sov_mission_2's stockpile objective is optional. A mission won with it
    // outstanding is the whole meaning of "secondary".
    CampaignDatabase Db;
    const CampaignMissionDef* Mission = Db.FindMission("sov_mission_2");
    RA4_REQUIRE(Mission != nullptr);

    MissionFixture Fix;
    Fix.Open(*Mission);
    WipePlayer(Fix.World, 1);
    Fix.World.SpawnBuilding(MakeContentId("building.sov.ore_refinery"), 0, TileCoord(20, 20), true);
    RA4_EXPECT_EQ(AsInt(Fix.RunFor(40)), AsInt(MissionStatus::Won));

    const MissionObjective* Optional = Fix.Runtime.FindObjective("obj_stockpile");
    RA4_REQUIRE(Optional != nullptr);
    RA4_EXPECT(!Optional->bIsPrimary);
    RA4_EXPECT(Optional->State != ObjectiveState::Completed);
}

RA4_TEST(MissionRuntime, ReplayingAMissionProducesTheSameResult)
{
    // The reason objectives are pure predicates over simulation state: two runs of the
    // same mission with the same inputs must agree on what happened and on when.
    CampaignDatabase Db;
    const CampaignMissionDef* Mission = Db.FindMission("sov_mission_2");
    RA4_REQUIRE(Mission != nullptr);

    std::vector<ObjectiveTransition> Runs[2];
    MissionStatus Results[2] = {MissionStatus::NotStarted, MissionStatus::NotStarted};

    for (int32_t Run = 0; Run < 2; ++Run)
    {
        MissionFixture Fix;
        Fix.Open(*Mission);
        Fix.RunFor(10);
        WipePlayer(Fix.World, 1);
        Fix.World.SpawnBuilding(MakeContentId("building.sov.ore_refinery"), 0, TileCoord(20, 20), true);
        Results[Run] = Fix.RunFor(40);
        Runs[Run] = Fix.Runtime.GetTransitions();
    }

    RA4_EXPECT(Results[0] == Results[1]);
    RA4_EXPECT_EQ(AsInt(Results[0]), AsInt(MissionStatus::Won));
    RA4_REQUIRE(Runs[0].size() == Runs[1].size());
    for (size_t I = 0; I < Runs[0].size(); ++I)
    {
        RA4_EXPECT(Runs[0][I].ObjectiveId == Runs[1][I].ObjectiveId);
        RA4_EXPECT(Runs[0][I].NewState == Runs[1][I].NewState);
        RA4_EXPECT_EQ(Runs[0][I].Tick, Runs[1][I].Tick);
    }
}

RA4_TEST(MissionRuntime, AMissionWithoutObjectivesIsNotWon)
{
    // Guards the guard. If AllPrimariesComplete() returned true on an empty list,
    // every unauthored mission would auto-win and NoMissionShipsAnUnauthoredObjective
    // would be the only thing standing between the campaign and a silent regression.
    MissionFixture Fix;
    Fix.Open(MakeStandoff("test_empty"));
    RA4_EXPECT_EQ(AsInt(Fix.RunFor(60)), AsInt(MissionStatus::InProgress));
}

RA4_TEST(MissionRuntime, AnUnauthoredObjectiveIsNeverSatisfied)
{
    CampaignMissionDef Placeholder = MakeStandoff("test_placeholder");

    MissionObjective Obj;
    Obj.Id = "obj_primary";
    Obj.State = ObjectiveState::Active;
    // Condition left at ObjectiveConditionType::None -- exactly what the campaign
    // database used to ship for 28 of its 38 missions.
    Placeholder.Objectives.push_back(Obj);

    MissionFixture Fix;
    Fix.Open(Placeholder);
    RA4_EXPECT_EQ(AsInt(Fix.RunFor(200)), AsInt(MissionStatus::InProgress));

    const MissionObjective* Never = Fix.Runtime.FindObjective("obj_primary");
    RA4_REQUIRE(Never != nullptr);
    RA4_EXPECT(Never->State == ObjectiveState::Active);
}
