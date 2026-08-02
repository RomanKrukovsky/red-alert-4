// Copyright (c) Red Alert 4 project. Tests for the computer opponent.
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4AI/AICommander.h"
#include "RA4AI/AIDebugOverlay.h"
#include "RA4AI/AIDoctrine.h"
#include "RA4AI/AIWorldView.h"
#include "RA4AI/ArmyGroup.h"
#include "RA4AI/TacticalOperation.h"
#include "RA4Core/SimConfig.h"


#include <algorithm>
#include <cstdio>
#include <string>

using namespace RA4;
using namespace RA4::AI;
using namespace RA4Test;

namespace
{

// Drives a match where one or both sides are played by the AI. This is the harness
// the acceptance test uses: if it cannot finish a match, the AI does not work.
struct AIMatch
{
    ContentDatabase Content;
    SimWorld World;
    AICommander Commanders[2];
    bool bCommanderActive[2] = {false, false};
    int32_t PeakBuildings[2] = {0, 0};
    int32_t PeakUnits[2] = {0, 0};

    explicit AIMatch(uint64_t Seed = 20260728)
    {
        BuildDefaultContent(Content);
        World.Initialize(&Content, MakeTestSetup(Seed));
        // Both sides start with a headquarters and ore, exactly as the skirmish
        // bootstrap seeds a real match.
        World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
        World.SpawnBuilding(Ids::AllConYard, 1, TileCoord(48, 48), true);
        for (int32_t X = 0; X < 3; ++X)
        {
            for (int32_t Y = 0; Y < 3; ++Y)
            {
                World.SpawnResourceNode(Ids::OreField, TileCoord(6 + X, 15 + Y), 4000);
                World.SpawnResourceNode(Ids::OreField, TileCoord(53 + X, 43 + Y), 4000);
            }
        }
        World.ClearEvents();
    }

    void Enable(PlayerId Player, AIProfile Profile, uint64_t Seed = 20260728)
    {
        Commanders[Player].Initialize(Player, Profile, Seed);
        bCommanderActive[Player] = true;
    }

    void Run(int32_t Ticks)
    {
        for (int32_t I = 0; I < Ticks && World.GetPhase() == MatchPhase::Running; ++I)
        {
            CommandFrame Frame;
            Frame.Tick = World.GetTick();
            for (PlayerId P = 0; P < 2; ++P)
            {
                if (bCommanderActive[P])
                {
                    Commanders[P].Tick(World, Frame.Commands);
                }
            }
            // The commanders consume events from the previous simulation tick.
            // Clear them only after every commander has observed them.
            World.ClearEvents();
            World.Tick(Frame.Commands.empty() ? nullptr : &Frame);
            for (PlayerId P = 0; P < 2; ++P)
            {
                PeakBuildings[P] = std::max(PeakBuildings[P], CountBuildings(P));
                PeakUnits[P] = std::max(PeakUnits[P], CountUnits(P));
            }
        }
    }

    int32_t CountBuildings(PlayerId Player) const { return CountEntities(World, Player, EntityKind::Building); }
    int32_t CountUnits(PlayerId Player) const { return CountEntities(World, Player, EntityKind::Unit); }

    int32_t CountArmed(PlayerId Player) const
    {
        int32_t Count = 0;
        const std::vector<EntityCore>& Cores = World.GetAllCores();
        for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
        {
            if (!Cores[I].bAlive || Cores[I].Owner != Player || Cores[I].Kind != EntityKind::Unit)
            {
                continue;
            }
            const EntityDef* Def = Content.FindEntity(Cores[I].Def);
            if (Def != nullptr && Def->Weapon.IsValid() && !Def->Unit.bIsHarvester && !Def->Unit.bIsBuilder)
            {
                ++Count;
            }
        }
        return Count;
    }
};

} // namespace

// ---------------------------------------------------------------------------
// Economy and build order
// ---------------------------------------------------------------------------

RA4_TEST(AI, BuildsPowerBeforeItStalls)
{
    AIMatch M;
    M.Enable(0, AIProfile::Balanced);
    M.Run(SecondsToTicks(60));

    // A power plant is the first thing a construction yard can build and everything
    // else throttles without it.
    RA4_EXPECT(CountEntitiesOfType(M.World, 0, Ids::SovPower) > 0);
    RA4_EXPECT(M.World.GetPlayer(0).PowerProduced > 0);
}

RA4_TEST(AI, BuildsRefineryAndActuallyEarns)
{
    AIMatch M;
    M.Enable(0, AIProfile::Economic);
    M.Run(SecondsToTicks(180));

    RA4_EXPECT(CountEntitiesOfType(M.World, 0, Ids::SovRefinery) > 0);
    // The refinery ships a harvester, so income proves the whole loop ran: build,
    // place, complete, gather, deliver.
    RA4_EXPECT(M.World.GetPlayer(0).TotalHarvested > 0);
}

RA4_TEST(AI, BuildsProductionBuildingsAndTrainsAnArmy)
{
    AIMatch M;
    M.Enable(0, AIProfile::Balanced);
    M.Run(SecondsToTicks(240));

    RA4_EXPECT(CountEntitiesOfType(M.World, 0, Ids::SovBarracks) > 0);
    RA4_EXPECT(M.CountArmed(0) > 0);
    std::printf("         balanced @240s: buildings=%d units=%d armed=%d harvested=%d\n",
                M.CountBuildings(0), M.CountUnits(0), M.CountArmed(0),
                M.World.GetPlayer(0).TotalHarvested);
}

RA4_TEST(AI, DoesNotBankruptItselfOnOneDecisionTick)
{
    AIMatch M;
    M.Enable(0, AIProfile::Balanced);
    M.Run(SecondsToTicks(30));

    // One construction decision per cycle: the treasury must never go negative and
    // must not be emptied in a single burst.
    RA4_EXPECT(M.World.GetPlayer(0).Credits >= 0);
}

RA4_TEST(AI, QueuesOneOfAKindNotSix)
{
    AIMatch M;
    M.Enable(0, AIProfile::Balanced);
    M.Run(SecondsToTicks(45));

    // Without the in-flight check the commander re-queues a refinery every decision
    // tick until the money runs out.
    RA4_EXPECT(CountEntitiesOfType(M.World, 0, Ids::SovRefinery) <= 2);
}

// ---------------------------------------------------------------------------
// Profiles
// ---------------------------------------------------------------------------

RA4_TEST(AI, ProfilesProduceDifferentConfigurations)
{
    const AIConfig Aggressive = MakeProfileConfig(AIProfile::Aggressive);
    const AIConfig Defensive = MakeProfileConfig(AIProfile::Defensive);
    const AIConfig Economic = MakeProfileConfig(AIProfile::Economic);

    // Aggressive commits earlier and with less; defensive waits for a bigger force.
    RA4_EXPECT(Aggressive.AttackArmySize < Defensive.AttackArmySize);
    RA4_EXPECT(Aggressive.TargetDefences < Defensive.TargetDefences);
    RA4_EXPECT(Economic.TargetHarvesters > Aggressive.TargetHarvesters);
    RA4_EXPECT(Economic.CreditReserve > Aggressive.CreditReserve);
}

RA4_TEST(AI, ExposesAdaptiveProfile)
{
    RA4_EXPECT(std::string(ToString(AIProfile::Adaptive)) == "Adaptive");
}

RA4_TEST(AI, UtilitySelectsEconomyForNewBase)
{
    AIWorldAssessment A;
    A.Credits = 10000;
    A.PowerProduced = 100;
    A.bHasConstructionYard = true;

    const std::vector<AIStrategyScore> Scores =
        ScoreStrategies(A, MakeProfileConfig(AIProfile::Balanced));

    RA4_EXPECT(FindWinningStrategy(Scores) == AIStrategy::ExpandEconomy);
}

RA4_TEST(AI, UtilitySelectsFortifyUnderAttack)
{
    AIWorldAssessment A;
    A.PowerProduced = 100;
    A.PowerPlants = 1;
    A.bHasConstructionYard = true;
    A.Refineries = 1;
    A.Harvesters = 3;
    A.ProductionBuildings = 2;
    A.bUnderAttack = true;

    const std::vector<AIStrategyScore> Scores =
        ScoreStrategies(A, MakeProfileConfig(AIProfile::Balanced));

    RA4_EXPECT(FindWinningStrategy(Scores) == AIStrategy::Fortify);
}

RA4_TEST(AI, UtilitySelectsAssaultWithReadyArmy)
{
    AIWorldAssessment A;
    A.PowerProduced = 100;
    A.PowerPlants = 1;
    A.bHasConstructionYard = true;
    A.Refineries = 1;
    A.Harvesters = 3;
    A.ProductionBuildings = 2;
    A.ArmedUnits = 8;
    A.bHasEnemyTarget = true;

    const std::vector<AIStrategyScore> Scores =
        ScoreStrategies(A, MakeProfileConfig(AIProfile::Balanced));

    RA4_EXPECT(FindWinningStrategy(Scores) == AIStrategy::Assault);
}

RA4_TEST(AI, UtilitySelectsTechWhenEconomyIsReady)
{
    AIWorldAssessment A;
    A.PowerProduced = 100;
    A.PowerPlants = 1;
    A.bHasConstructionYard = true;
    A.Refineries = 1;
    A.Harvesters = 3;

    const std::vector<AIStrategyScore> Scores =
        ScoreStrategies(A, MakeProfileConfig(AIProfile::Balanced));

    RA4_EXPECT(FindWinningStrategy(Scores) == AIStrategy::TechUp);
}

RA4_TEST(AI, UtilitySelectsArmyWhenProductionIsReady)
{
    AIWorldAssessment A;
    A.PowerProduced = 100;
    A.PowerPlants = 1;
    A.bHasConstructionYard = true;
    A.Refineries = 1;
    A.Harvesters = 3;
    A.ProductionBuildings = 2;
    A.Defences = 2;
    A.ArmedUnits = 1;
    A.bHasEnemyTarget = true;

    const std::vector<AIStrategyScore> Scores =
        ScoreStrategies(A, MakeProfileConfig(AIProfile::Balanced));

    RA4_EXPECT(FindWinningStrategy(Scores) == AIStrategy::AssembleArmy);
}

RA4_TEST(AI, UtilitySelectsRecoveryAfterEconomyLoss)
{
    AIWorldAssessment A;
    A.bHasConstructionYard = true;
    A.TotalHarvested = 2000;
    A.Harvesters = 1;

    const std::vector<AIStrategyScore> Scores =
        ScoreStrategies(A, MakeProfileConfig(AIProfile::Balanced));

    RA4_EXPECT(FindWinningStrategy(Scores) == AIStrategy::Recover);
}

RA4_TEST(AI, StrategyHysteresisKeepsCurrentChoiceNearATie)
{
    AIConfig Config;
    Config.StrategySwitchMargin = 100;
    const std::vector<AIStrategyScore> Scores = {
        {AIStrategy::ExpandEconomy, 600, "economy"},
        {AIStrategy::TechUp, 650, "tech"},
    };

    RA4_EXPECT(SelectStrategy(Scores, AIStrategy::ExpandEconomy, true, Config) ==
               AIStrategy::ExpandEconomy);
}

RA4_TEST(AI, EmergencyStrategyOverridesHysteresis)
{
    AIConfig Config;
    Config.StrategySwitchMargin = 200;
    Config.EmergencyStrategyScore = 900;
    const std::vector<AIStrategyScore> Scores = {
        {AIStrategy::AssembleArmy, 850, "army"},
        {AIStrategy::Fortify, 1000, "under attack"},
    };

    RA4_EXPECT(SelectStrategy(Scores, AIStrategy::AssembleArmy, true, Config) ==
               AIStrategy::Fortify);
}

RA4_TEST(AI, RecoveryMaySpendTheCreditReserve)
{
    AIConfig Config;
    Config.CreditReserve = 400;

    RA4_EXPECT_EQ(RequiredCreditReserve(AIStrategy::AssembleArmy, Config), 400);
    RA4_EXPECT_EQ(RequiredCreditReserve(AIStrategy::ExpandEconomy, Config), 400);
    RA4_EXPECT_EQ(RequiredCreditReserve(AIStrategy::Recover, Config), 0);
}

RA4_TEST(AI, AggressiveProfileValuesAssaultMoreThanDefensive)
{
    AIWorldAssessment A;
    A.Refineries = 1;
    A.Harvesters = 2;
    A.ProductionBuildings = 2;
    A.ArmedUnits = 6;
    A.bHasEnemyTarget = true;

    const auto AggressiveScores =
        ScoreStrategies(A, MakeProfileConfig(AIProfile::Aggressive));
    const auto DefensiveScores =
        ScoreStrategies(A, MakeProfileConfig(AIProfile::Defensive));

    RA4_EXPECT(FindStrategyScore(AggressiveScores, AIStrategy::Assault) >
               FindStrategyScore(DefensiveScores, AIStrategy::Assault));
}

RA4_TEST(AI, EconomicProfileOutEarnsAggressive)
{
    AIMatch Eco;
    Eco.Enable(0, AIProfile::Economic);
    Eco.Run(SecondsToTicks(200));

    AIMatch Agg;
    Agg.Enable(0, AIProfile::Aggressive);
    Agg.Run(SecondsToTicks(200));

    std::printf("         economic harvested=%d, aggressive harvested=%d\n",
                Eco.World.GetPlayer(0).TotalHarvested, Agg.World.GetPlayer(0).TotalHarvested);
    // The profile has to change behaviour, not just a struct field.
    RA4_EXPECT(Eco.World.GetPlayer(0).TotalHarvested >= Agg.World.GetPlayer(0).TotalHarvested);
}

// ---------------------------------------------------------------------------
// Fighting
// ---------------------------------------------------------------------------

RA4_TEST(AI, AttacksOnceTheArmyReachesStrength)
{
    AIMatch M;
    M.Enable(0, AIProfile::Aggressive);
    M.Run(SecondsToTicks(300));

    // The enemy headquarters must have taken damage, which only happens if the AI
    // built an army, walked it across the map and engaged.
    const EntityId EnemyYard = FindFirstOfType(M.World, 1, Ids::AllConYard);
    bool bEngaged = false;
    if (!EnemyYard.IsValid())
    {
        bEngaged = true;   // already destroyed
    }
    else if (const HealthComp* Health = M.World.GetHealth(EnemyYard))
    {
        bEngaged = Health->Current < Health->Max;
    }
    std::printf("         aggressive @300s: armed=%d, enemy HQ engaged=%d\n", M.CountArmed(0), bEngaged ? 1 : 0);
    RA4_EXPECT(bEngaged);
}

RA4_TEST(AI, RecentDamageTriggersDefenceStrategy)
{
    AIMatch M;
    M.Enable(0, AIProfile::Aggressive);
    M.Enable(1, AIProfile::Balanced, /*Seed*/ 987654321);
    M.Commanders[1].SetDecisionLogLimit(512);
    M.Run(SecondsToTicks(300));

    bool bSawFortify = false;
    for (const AIDecision& Decision : M.Commanders[1].GetDecisionLog())
    {
        if (Decision.Strategy == AIStrategy::Fortify)
        {
            bSawFortify = true;
            break;
        }
    }
    RA4_EXPECT(bSawFortify);
}

RA4_TEST(AI, DecisionLogExplainsWhatItDid)
{
    AIMatch M;
    M.Enable(0, AIProfile::Balanced);
    M.Run(SecondsToTicks(90));

    const std::vector<AIDecision>& Log = M.Commanders[0].GetDecisionLog();
    RA4_REQUIRE(!Log.empty());
    for (const AIDecision& D : Log)
    {
        // Every entry must carry a human-readable reason, or the log is useless for
        // diagnosing an AI that appears to be idle.
        RA4_EXPECT(!D.Reason.empty());
    }
    std::printf("         first decisions: %s / %s\n", Log[0].Reason.c_str(),
                Log.size() > 1 ? Log[1].Reason.c_str() : "-");
}

RA4_TEST(AI, CommanderReportsSelectedStrategyAndScore)
{
    AIMatch M;
    M.Enable(0, AIProfile::Balanced);
    M.Run(SecondsToTicks(5));

    const std::vector<AIDecision>& Log = M.Commanders[0].GetDecisionLog();
    RA4_REQUIRE(!Log.empty());
    RA4_EXPECT(Log.back().Strategy == M.Commanders[0].GetActiveStrategy());
    RA4_EXPECT(Log.back().StrategyScore >= 0);
    RA4_EXPECT(Log.back().StrategyScore <= 1000);
    RA4_EXPECT(!Log.back().Reason.empty());
}

// ---------------------------------------------------------------------------
// Acceptance
// ---------------------------------------------------------------------------

RA4_TEST(AI, TwoCommandersPlayAMatchToCompletion)
{
    // The acceptance test for the whole module: two AIs, one map, no human input.
    // If this finishes with a winner, a player can sit down against one of them.
    AIMatch M;
    M.Enable(0, AIProfile::Aggressive);
    M.Enable(1, AIProfile::Balanced, /*Seed*/ 987654321);
    M.Run(SecondsToTicks(900));

    std::printf("         AI vs AI: tick=%u phase=%d winner=%d | P0 b=%d u=%d harv=%d | P1 b=%d u=%d harv=%d\n",
                M.World.GetTick(), int32_t(M.World.GetPhase()), int32_t(M.World.GetWinner()),
                M.CountBuildings(0), M.CountUnits(0), M.World.GetPlayer(0).TotalHarvested,
                M.CountBuildings(1), M.CountUnits(1), M.World.GetPlayer(1).TotalHarvested);

    // Both sides must have actually played: built a base and earned money.
    RA4_EXPECT(M.PeakBuildings[0] > 1);
    RA4_EXPECT(M.PeakBuildings[1] > 1);
    RA4_EXPECT(M.World.GetPlayer(0).TotalHarvested > 0);
    RA4_EXPECT(M.World.GetPlayer(1).TotalHarvested > 0);
    // And someone must have shot at someone.
    RA4_EXPECT(M.World.GetPlayer(0).UnitsLost + M.World.GetPlayer(1).UnitsLost +
                   M.World.GetPlayer(0).BuildingsLost + M.World.GetPlayer(1).BuildingsLost >
               0);
}

RA4_TEST(AI, FiveSkirmishScenariosFinishWithAWinner)
{
    struct Scenario
    {
        uint64_t MatchSeed;
        uint64_t PlayerZeroSeed;
        uint64_t PlayerOneSeed;
        AIProfile PlayerZeroProfile;
        AIProfile PlayerOneProfile;
    };

    const Scenario Scenarios[] = {
        {20260730, 101, 201, AIProfile::Aggressive, AIProfile::Balanced},
        {20260731, 102, 202, AIProfile::Balanced, AIProfile::Aggressive},
        {20260732, 103, 203, AIProfile::Defensive, AIProfile::Aggressive},
        {20260733, 104, 204, AIProfile::Aggressive, AIProfile::Defensive},
        {20260734, 105, 205, AIProfile::Adaptive, AIProfile::Economic},
    };

    for (int32_t Index = 0; Index < int32_t(sizeof(Scenarios) / sizeof(Scenarios[0])); ++Index)
    {
        const Scenario& S = Scenarios[Index];
        AIMatch M(S.MatchSeed);
        M.Enable(0, S.PlayerZeroProfile, S.PlayerZeroSeed);
        M.Enable(1, S.PlayerOneProfile, S.PlayerOneSeed);
        M.Run(SecondsToTicks(900));

        std::printf("         scenario %d: tick=%u phase=%d winner=%d losses=%d\n",
                    Index + 1, M.World.GetTick(), int32_t(M.World.GetPhase()),
                    int32_t(M.World.GetWinner()),
                    M.World.GetPlayer(0).UnitsLost + M.World.GetPlayer(1).UnitsLost +
                        M.World.GetPlayer(0).BuildingsLost + M.World.GetPlayer(1).BuildingsLost);

        if (Index == 4)
        {
            const TacticalOperation& Op = M.Commanders[0].GetActiveOperation();
            std::printf("         scenario 5 P0 op: id=%u state=%s assigned=%zu target=(%d,%d)\n",
                        Op.OperationId, ToString(Op.State),
                        Op.AssignedUnits.size(), Op.TargetLocation.X, Op.TargetLocation.Y);
            std::printf("         scenario 5 P0 final: armed=%d buildings=%d bActive=%d bDefeated=%d\n",
                        M.CountArmed(0), M.CountBuildings(0),
                        int32_t(M.World.GetPlayer(0).bActive), int32_t(M.World.GetPlayer(0).bDefeated));
            std::printf("         scenario 5 P1 final: armed=%d buildings=%d bActive=%d bDefeated=%d\n",
                        M.CountArmed(1), M.CountBuildings(1),
                        int32_t(M.World.GetPlayer(1).bActive), int32_t(M.World.GetPlayer(1).bDefeated));
        }

        RA4_EXPECT(M.World.GetPhase() == MatchPhase::Finished);
        RA4_EXPECT(M.World.GetWinner() == 0 || M.World.GetWinner() == 1);
        RA4_EXPECT(M.PeakBuildings[0] > 1);
        RA4_EXPECT(M.PeakBuildings[1] > 1);
        RA4_EXPECT(M.World.GetPlayer(0).TotalHarvested > 0);
        RA4_EXPECT(M.World.GetPlayer(1).TotalHarvested > 0);
    }
}

RA4_TEST(AI, IsDeterministic)
{
    // Same seed, same commanders, same result -- otherwise replays and lockstep
    // multiplayer break the moment an AI is in the match.
    AIMatch A(4242);
    A.Enable(0, AIProfile::Balanced);
    A.Enable(1, AIProfile::Defensive, 555);
    A.Run(SecondsToTicks(120));

    AIMatch B(4242);
    B.Enable(0, AIProfile::Balanced);
    B.Enable(1, AIProfile::Defensive, 555);
    B.Run(SecondsToTicks(120));

    RA4_EXPECT(A.World.ComputeStateChecksum() == B.World.ComputeStateChecksum());
    RA4_EXPECT_EQ(A.World.GetPlayer(0).TotalHarvested, B.World.GetPlayer(0).TotalHarvested);

    const std::vector<AIDecision>& LogA = A.Commanders[0].GetDecisionLog();
    const std::vector<AIDecision>& LogB = B.Commanders[0].GetDecisionLog();
    RA4_REQUIRE(LogA.size() == LogB.size());
    for (size_t I = 0; I < LogA.size(); ++I)
    {
        RA4_EXPECT(LogA[I].Tick == LogB[I].Tick);
        RA4_EXPECT(LogA[I].Strategy == LogB[I].Strategy);
        RA4_EXPECT_EQ(LogA[I].StrategyScore, LogB[I].StrategyScore);
        RA4_EXPECT(LogA[I].Command == LogB[I].Command);
        RA4_EXPECT(LogA[I].Reason == LogB[I].Reason);
    }
}

RA4_TEST(AI, StopsIssuingOrdersOnceDefeated)
{
    AIMatch M;
    M.Enable(0, AIProfile::Balanced);
    M.Run(SecondsToTicks(20));

    Command Surrender;
    Surrender.Type = CommandType::Surrender;
    Surrender.Issuer = 0;
    M.World.ApplyCommand(Surrender);
    M.Run(5);

    std::vector<Command> Commands;
    M.Commanders[0].Tick(M.World, Commands);
    // A defeated or finished match must not keep generating traffic.
    RA4_EXPECT(Commands.empty());
}

RA4_TEST(AI, SimWorldViewTracksEnemyMemoryAndDecaysConfidence)
{
    AIMatch M;
    SimWorldView View(M.World, 0);

    // Add a remembered enemy entity whose last seen tick is fresh
    EnemyMemory FreshMem;
    FreshMem.Entity = EntityId(1, 0);
    FreshMem.Position = TileCoord(48, 48);
    FreshMem.LastSeenTick = M.World.GetTick();
    FreshMem.Confidence = Fixed::FromInt(1);

    View.AddKnownEnemyMemory(FreshMem);
    const auto& Enemies = View.GetKnownEnemies();
    RA4_REQUIRE(!Enemies.empty());
    RA4_EXPECT(Enemies[0].Confidence.Raw == Fixed::FromInt(1).Raw);

    // Advance ticks without re-spotting under fog
    M.Run(50);

    // Decay is exercised directly rather than through UpdateMemory: there is no fog
    // grid yet, so the observation pass sees every enemy every tick and would reset
    // confidence back to 1 before the ageing step could be measured.
    SimWorldView PastView(M.World, 0);
    PastView.AddKnownEnemyMemory(FreshMem);
    PastView.DecayMemories(/*MemoryRetentionTicks*/ 100);

    const auto& PastEnemies = PastView.GetKnownEnemies();
    RA4_REQUIRE(!PastEnemies.empty());
    RA4_EXPECT(PastEnemies[0].Confidence.Raw < Fixed::FromInt(1).Raw);
    RA4_EXPECT(PastEnemies[0].Confidence.Raw >= Fixed::FromRatio(10, 100).Raw);
}

RA4_TEST(AI, TacticalOperationLifecycleStateTransitions)
{
    TacticalOperation Op;
    Op.OperationId = 1;
    Op.TargetLocation = TileCoord(48, 48);

    RA4_EXPECT(Op.State == OperationState::Proposed);
    RA4_EXPECT(std::string(ToString(Op.State)) == "Proposed");

    Op.TransitionTo(OperationState::Gathering, 100);
    RA4_EXPECT(Op.State == OperationState::Gathering);
    RA4_EXPECT_EQ(Op.LastStateChangeTick, 100);

    Op.TransitionTo(OperationState::Advancing, 200);
    RA4_EXPECT(Op.State == OperationState::Advancing);
    RA4_EXPECT(std::string(ToString(Op.State)) == "Advancing");

    Op.TransitionTo(OperationState::Engaging, 250);
    RA4_EXPECT(Op.State == OperationState::Engaging);

    Op.TransitionTo(OperationState::Completed, 300);
    RA4_EXPECT(Op.State == OperationState::Completed);
}

RA4_TEST(AI, CommanderTracksActiveOperationLifecycleInMatch)
{
    AIMatch M;
    M.Enable(0, AIProfile::Aggressive);
    M.Run(SecondsToTicks(300));

    const TacticalOperation& Op = M.Commanders[0].GetActiveOperation();
    // Aggressive commander built an army and pushed: operation must have been created and advanced
    RA4_EXPECT(Op.OperationId > 0);
    RA4_EXPECT(Op.State == OperationState::Staging || Op.State == OperationState::Advancing || Op.State == OperationState::Engaging || Op.State == OperationState::Completed);
}

RA4_TEST(AI, SquadsGatherBeforeAdvancing)
{
    AIMatch M;
    M.Enable(0, AIProfile::Balanced);
    M.Run(SecondsToTicks(300));

    const TacticalOperation& Op = M.Commanders[0].GetActiveOperation();
    const AIConfig Config = M.Commanders[0].GetConfig();

    // If the commander committed an assault, it must have formed a squad at least
    // as large as its configured minimum attack size.  Committing with only one unit
    // is streaming, but every profile has a non-trivial minimum commit threshold.
    if (Op.OperationId > 0)
    {
        RA4_EXPECT(int32_t(Op.AssignedUnits.size()) >= Config.MinimumAttackSize);
    }
}

RA4_TEST(AI, SquadAssignmentIsDeterministic)
{
    // Two identical matches must assign exactly the same entities to the active
    // operation in the same order.
    AIMatch A(123456);
    A.Enable(0, AIProfile::Balanced);
    A.Run(SecondsToTicks(240));

    AIMatch B(123456);
    B.Enable(0, AIProfile::Balanced);
    B.Run(SecondsToTicks(240));

    const TacticalOperation& OpA = A.Commanders[0].GetActiveOperation();
    const TacticalOperation& OpB = B.Commanders[0].GetActiveOperation();

    RA4_EXPECT(OpA.OperationId == OpB.OperationId);
    RA4_EXPECT(OpA.State == OpB.State);
    RA4_REQUIRE(OpA.AssignedUnits.size() == OpB.AssignedUnits.size());
    for (size_t I = 0; I < OpA.AssignedUnits.size(); ++I)
    {
        RA4_EXPECT(OpA.AssignedUnits[I] == OpB.AssignedUnits[I]);
    }
}

RA4_TEST(AI, WoundedUnitRetreatsToBase)
{
    AIMatch M;
    M.Enable(0, AIProfile::Aggressive);

    // Spawn an armed unit for Player 0 away from base and damage its health below 25%
    const EntityId TankId = M.World.SpawnUnit(Ids::SovConscript, 0, Vec2(Fixed::FromInt(5000), Fixed::FromInt(5000)));
    RA4_REQUIRE(TankId.IsValid());

    // Apply damage to bring health below 25%
    const HealthComp* Health = M.World.GetHealth(TankId);
    RA4_REQUIRE(Health != nullptr);
    const_cast<HealthComp*>(Health)->Current = Health->Max / 5; // 20% health remaining

    M.Run(SecondsToTicks(10));

    // Commander decision log should record wounded unit retreat order
    bool bSawRetreat = false;
    for (const AIDecision& D : M.Commanders[0].GetDecisionLog())
    {
        if (D.Command == CommandType::Move && D.Reason.find("wounded") != std::string::npos)
        {
            bSawRetreat = true;
            break;
        }
    }
    RA4_EXPECT(bSawRetreat);
}

// ---------------------------------------------------------------------------
// Fog-limited knowledge model
// ---------------------------------------------------------------------------

RA4_TEST(AIKnowledge, MemoryIndexesTheFogGridInTilesNotWorldUnits)
{
    // Regression guard for a real defect: UpdateMemory used to pass raw world
    // coordinates (e.g. 2300) straight into FFogOfWarGrid::GetVisibility, which is
    // indexed in TILES and bounded by the map's 64x64 grid. Every lookup fell out of
    // bounds and returned NeverSeen, so with fog active the AI remembered nothing at
    // all -- including enemies standing in plain sight.
    AIMatch M;
    // One simulation tick so SystemFogOfWar stamps visibility for the starting bases.
    M.Run(1);

    RA4_REQUIRE(M.World.GetFogGrid() != nullptr);

    // Player 0's construction yard sits at tile (10,10) and sees around itself, so
    // an enemy parked next to it must be observed.
    const EntityId CloseEnemy =
        M.World.SpawnUnit(Ids::AllRifleman, 1, M.World.GetMap().TileCenterToWorld(TileCoord(11, 11)));
    RA4_REQUIRE(CloseEnemy.IsValid());
    M.Run(1);

    SimWorldView View(M.World, 0);
    View.UpdateMemory(/*MemoryRetentionTicks*/ 600);

    bool bRemembered = false;
    for (const EnemyMemory& Mem : View.GetKnownEnemies())
    {
        if (Mem.Entity == CloseEnemy)
        {
            bRemembered = true;
        }
    }
    RA4_EXPECT(bRemembered);
}

RA4_TEST(AIKnowledge, EnemiesOutsideVisionAreNotObserved)
{
    // The other half of the contract: seeing something nearby must not imply seeing
    // the whole map. The enemy base at (48,48) is far outside player 0's vision.
    AIMatch M;
    M.Run(1);
    RA4_REQUIRE(M.World.GetFogGrid() != nullptr);

    SimWorldView View(M.World, 0);
    View.UpdateMemory(/*MemoryRetentionTicks*/ 600);

    for (const EnemyMemory& Mem : View.GetKnownEnemies())
    {
        const TileCoord Tile = Mem.Position;
        // Nothing from the far corner of the map may appear in memory.
        RA4_EXPECT(!(Tile.X > 40 && Tile.Y > 40));
    }
}

RA4_TEST(AI, EntityRoleDerivationWorksForUnitsAndBuildings)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);

    const EntityDef* Conscript = Content.FindEntity(Ids::SovConscript);
    RA4_REQUIRE(Conscript != nullptr);
    RA4_EXPECT(HasRole(Conscript->Roles, EntityRole::Combat));
    RA4_EXPECT(HasRole(Conscript->Roles, EntityRole::Scout));

    const EntityDef* HeavyTank = Content.FindEntity(Ids::SovHeavyTank);
    RA4_REQUIRE(HeavyTank != nullptr);
    RA4_EXPECT(HasRole(HeavyTank->Roles, EntityRole::Combat));
    RA4_EXPECT(HasRole(HeavyTank->Roles, EntityRole::AntiArmor));

    const EntityDef* Rifleman = Content.FindEntity(Ids::AllRifleman);
    RA4_REQUIRE(Rifleman != nullptr);
    RA4_EXPECT(HasRole(Rifleman->Roles, EntityRole::Combat));

    const EntityDef* PowerPlant = Content.FindEntity(Ids::SovPower);
    RA4_REQUIRE(PowerPlant != nullptr);
    RA4_EXPECT(HasRole(PowerPlant->Roles, EntityRole::Power));
    RA4_EXPECT(HasRole(PowerPlant->Roles, EntityRole::BaseBuilding));

    const EntityDef* Refinery = Content.FindEntity(Ids::SovRefinery);
    RA4_REQUIRE(Refinery != nullptr);
    RA4_EXPECT(HasRole(Refinery->Roles, EntityRole::Refinery));
    RA4_EXPECT(HasRole(Refinery->Roles, EntityRole::BaseBuilding));

    const EntityDef* Harvester = Content.FindEntity(Ids::SovHarvester);
    RA4_REQUIRE(Harvester != nullptr);
    RA4_EXPECT(HasRole(Harvester->Roles, EntityRole::Harvester));
}

RA4_TEST(AI, DispatchesScoutWhenNoTargetsAreKnown)
{
    AIMatch M;
    M.Enable(0, AIProfile::Balanced);
    M.Commanders[0].SetDecisionLogLimit(512);

    // Run until AI trains a combat unit and dispatches it on a scouting move order
    M.Run(SecondsToTicks(240));

    bool bSawScoutMove = false;
    for (const AIDecision& D : M.Commanders[0].GetDecisionLog())
    {
        if (D.Command == CommandType::Move && D.Reason.find("scouting") != std::string::npos)
        {
            bSawScoutMove = true;
            break;
        }
    }
    RA4_EXPECT(bSawScoutMove);
}

RA4_TEST(AI, ArmyGroupManagerLifecycle)
{
    ArmyGroupManager Mgr;
    uint32_t G1 = Mgr.AllocateGroupId();
    ArmyGroup* Group = Mgr.CreateGroup(G1, GroupRole::MainAssault, "Alpha Vanguard");
    RA4_REQUIRE(Group != nullptr);
    RA4_EXPECT_EQ(Group->GroupId, G1);
    RA4_EXPECT_EQ(static_cast<uint8_t>(Group->Role), static_cast<uint8_t>(GroupRole::MainAssault));


    Group->Members.push_back(EntityId(1, 1));
    Group->Members.push_back(EntityId(2, 1));
    Group->Stance = GroupStance::Aggressive;
    Group->FormationShape = GroupFormationShape::Wedge;

    const ArmyGroup* Found = Mgr.FindGroup(G1);
    RA4_REQUIRE(Found != nullptr);
    RA4_EXPECT_EQ(Found->Members.size(), 2);
    RA4_EXPECT_EQ(static_cast<uint8_t>(Found->Stance), static_cast<uint8_t>(GroupStance::Aggressive));
    RA4_EXPECT_EQ(static_cast<uint8_t>(Found->FormationShape), static_cast<uint8_t>(GroupFormationShape::Wedge));

    Mgr.RemoveGroup(G1);
    RA4_EXPECT(Mgr.FindGroup(G1) == nullptr);
}

RA4_TEST(AI, FactionDoctrinesSovietAndAlliance)
{
    FactionDoctrineDef Soviet = AIDoctrineRegistry::GetDoctrineForFaction(FactionId::Soviet, AIProfile::Balanced);
    FactionDoctrineDef Alliance = AIDoctrineRegistry::GetDoctrineForFaction(FactionId::Alliance, AIProfile::Balanced);

    RA4_EXPECT_EQ(static_cast<uint8_t>(Soviet.Faction), static_cast<uint8_t>(FactionId::Soviet));
    RA4_EXPECT_EQ(static_cast<uint8_t>(Soviet.Type), static_cast<uint8_t>(AIDoctrineType::SovietArmoredPush));
    RA4_EXPECT_EQ(static_cast<uint8_t>(Alliance.Faction), static_cast<uint8_t>(FactionId::Alliance));
    RA4_EXPECT_EQ(static_cast<uint8_t>(Alliance.Type), static_cast<uint8_t>(AIDoctrineType::AllianceMobilePrecision));


    // Soviet values armored push & heavier losses tolerance
    RA4_EXPECT(Soviet.Personality.AcceptableLossesPercent > Alliance.Personality.AcceptableLossesPercent);
    // Alliance values scouting and caution
    RA4_EXPECT(Alliance.Personality.ScoutPriority > Soviet.Personality.ScoutPriority);
}

RA4_TEST(AI, AIDebugOverlaySnapshotCreation)
{
    std::vector<ArmyGroup> Groups;
    ArmyGroup G;
    G.GroupId = 1;
    G.Name = "Iron Column";
    G.Role = GroupRole::MainAssault;
    G.Members.push_back(EntityId(10, 1));
    Groups.push_back(G);

    std::vector<std::string> Logs = {"Built Power Plant", "Dispatched Scout"};

    AIDebugOverlaySnapshot Snap = AIDebugLogger::CreateSnapshot(
        0, "General Sokolov", "Soviet Armored Push", AIStrategy::Assault, 85,
        "Pushing enemy HQ", 5000, 100, 40, 3, 90, Groups, Logs
    );

    RA4_EXPECT(Snap.Player == 0);
    RA4_EXPECT(Snap.CommanderName == "General Sokolov");
    RA4_EXPECT(Snap.ActiveGroups.size() == 1);
    RA4_EXPECT(Snap.ActiveGroups[0].Name == "Iron Column");
    RA4_EXPECT(Snap.RecentDecisions.size() == 2);
}

RA4_TEST(AI, DifficultyProfilesConfig)
{
    AIConfig EasyCfg = MakeProfileConfig(AIProfile::Balanced, AIDifficulty::Easy);
    AIConfig NormalCfg = MakeProfileConfig(AIProfile::Balanced, AIDifficulty::Normal);
    AIConfig HardCfg = MakeProfileConfig(AIProfile::Balanced, AIDifficulty::Hard);

    RA4_EXPECT(EasyCfg.DecisionIntervalTicks == 20);
    RA4_EXPECT(NormalCfg.DecisionIntervalTicks == 10);
    RA4_EXPECT(HardCfg.DecisionIntervalTicks == 5);

    RA4_EXPECT(EasyCfg.CreditBonusMultiplier == 1.0f);
    RA4_EXPECT(NormalCfg.CreditBonusMultiplier == 1.0f);
    RA4_EXPECT(HardCfg.CreditBonusMultiplier == 1.20f);
}

RA4_TEST(AI, FogOfWarStrictCompliance)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);
    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(20260731));

    // Player 0 (AI) at (10, 10), Player 1 (Enemy) at (55, 55) far out in fog
    World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    World.SpawnBuilding(Ids::AllConYard, 1, TileCoord(55, 55), true);

    SimWorldView Knowledge(World, 0);
    Knowledge.UpdateMemory(600);

    // AI's known enemies list MUST be empty because enemy is outside vision radius
    RA4_EXPECT(Knowledge.GetKnownEnemies().empty());
}

RA4_TEST(AI, NoCheatResources)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);
    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(20260731));

    World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    AICommander AI;
    AI.Initialize(0, AIProfile::Balanced, 20260731);

    const int32_t InitialCredits = World.GetPlayer(0).Credits;

    // Run 100 ticks without harvesting
    for (int32_t T = 0; T < 100; ++T)
    {
        CommandFrame Frame;
        Frame.Tick = World.GetTick();
        AI.Tick(World, Frame.Commands);
        World.ClearEvents();
        World.Tick(Frame.Commands.empty() ? nullptr : &Frame);
    }

    // Credits must equal initial credits minus legitimate spending (0 free credits added)
    const int32_t CurrentCredits = World.GetPlayer(0).Credits;
    RA4_EXPECT(CurrentCredits <= InitialCredits);
}

RA4_TEST(AI, MassSimulationsBenchmark)
{
    // Run mass AI vs AI matches with fixed seeds across profiles and difficulties
    const AIProfile Profiles[4] = {AIProfile::Aggressive, AIProfile::Balanced, AIProfile::Economic, AIProfile::Defensive};
    int32_t CompletedMatches = 0;
    int32_t TotalTicksExecuted = 0;

    for (int32_t SeedIdx = 0; SeedIdx < 8; ++SeedIdx)
    {
        const uint64_t Seed = 20260731u + static_cast<uint64_t>(SeedIdx) * 12345u;
        AIMatch Match(Seed);
        Match.Enable(0, Profiles[SeedIdx % 4], Seed);
        Match.Enable(1, Profiles[(SeedIdx + 1) % 4], Seed + 1);

        Match.Run(10000);
        const int32_t Winner = Match.World.GetWinner();
        if (Winner != 0 && Winner != 1 && Winner != -1)
        {
            std::printf("  stall seed %d tick=%u p0 armed=%d b=%d op=%s | p1 armed=%d b=%d op=%s profiles=%d/%d\n",
                        SeedIdx, Match.World.GetTick(),
                        Match.CountArmed(0), Match.CountBuildings(0),
                        ToString(Match.Commanders[0].GetActiveOperation().State),
                        Match.CountArmed(1), Match.CountBuildings(1),
                        ToString(Match.Commanders[1].GetActiveOperation().State),
                        SeedIdx % 4, (SeedIdx + 1) % 4);
        }
        RA4_EXPECT(Winner == 0 || Winner == 1 || Winner == -1);

        CompletedMatches++;
        TotalTicksExecuted += Match.World.GetTick();
    }

    RA4_EXPECT_EQ(CompletedMatches, 8);
    RA4_EXPECT(TotalTicksExecuted > 0);
}

RA4_TEST(AI, HardDifficultyIssuesMoreDecisionsThanEasy)
{
    // Same profile, same seed: only the difficulty config differs. With a 4x tighter
    // decision interval the Hard commander must log strictly more decisions.
    const int32_t Ticks = SecondsToTicks(120);

    AIMatch Easy;
    Easy.Enable(0, AIProfile::Balanced);
    Easy.Commanders[0].SetConfig(MakeProfileConfig(AIProfile::Balanced, AIDifficulty::Easy));
    Easy.Commanders[0].SetDecisionLogLimit(2048);
    Easy.Run(Ticks);

    AIMatch Hard;
    Hard.Enable(0, AIProfile::Balanced);
    Hard.Commanders[0].SetConfig(MakeProfileConfig(AIProfile::Balanced, AIDifficulty::Hard));
    Hard.Commanders[0].SetDecisionLogLimit(2048);
    Hard.Run(Ticks);

    std::printf("         decisions @120s: easy=%zu hard=%zu\n",
                Easy.Commanders[0].GetDecisionLog().size(),
                Hard.Commanders[0].GetDecisionLog().size());
    RA4_EXPECT(Hard.Commanders[0].GetDecisionLog().size() >
               Easy.Commanders[0].GetDecisionLog().size());
}

RA4_TEST(AI, DoctrineRaisesSovietHarvesterTarget)
{
    // Player 0 is Soviet in the AIMatch harness, so the commander resolves the
    // Soviet doctrine on its first tick (TargetHarvesterCount = 4).
    AIMatch M;
    M.Enable(0, AIProfile::Balanced);
    M.Run(2);

    const FactionDoctrineDef& Doctrine = M.Commanders[0].GetDoctrineForTesting();
    RA4_EXPECT(Doctrine.TargetHarvesterCount > M.Commanders[0].GetConfig().TargetHarvesters);
    RA4_EXPECT(Doctrine.MinimumAssaultArmySize > 0);
}

RA4_TEST(AI, DoctrineLoadsLazilyOnFirstTick)
{
    // Before the world exists the doctrine must stay at defaults: Initialize never
    // touches it, only Tick does.
    AICommander Commander;
    Commander.Initialize(0, AIProfile::Balanced, 123);
    RA4_EXPECT(Commander.GetDoctrineForTesting().TargetHarvesterCount == 3); // struct default

    ContentDatabase Content;
    BuildDefaultContent(Content);
    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(20260731));
    std::vector<Command> Commands;
    Commander.Tick(World, Commands);
    RA4_EXPECT(Commander.GetDoctrineForTesting().TargetHarvesterCount == 4); // Soviet doctrine
}
