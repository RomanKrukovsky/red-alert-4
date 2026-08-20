// Copyright (c) Red Alert 4 project. The computer opponent.
//
// Engine-free by design: the AI reads SimWorld through the IAIWorldView read-only interface
// and emits the same Command objects a human produces, which then pass the same server validation.
// It cannot cheat by construction -- there is no path from here into simulation state except through ApplyCommand.
//
// That also makes it testable: an AI-versus-AI match runs headless in milliseconds,
// so "does the opponent actually play" is a regression test rather than an opinion.
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "RA4AI/AIDirectors.h"
#include "RA4AI/AIDoctrine.h"
#include "RA4AI/AIStrategy.h"
#include "RA4AI/AIWorldView.h"
#include "RA4AI/BattlePredictor.h"
#include "RA4AI/OpponentModel.h"
#include "RA4AI/ThreatMap.h"
#include "RA4AI/TacticalOperation.h"
#include "RA4AI/ValueMap.h"
#include "RA4Core/Command.h"
#include "RA4Core/Ids.h"
#include "RA4Core/Random.h"
#include "RA4Simulation/SimWorld.h"

#ifndef RA4AI_API
#define RA4AI_API
#endif

namespace RA4
{
namespace AI
{

// One decision, recorded for diagnostics. The brief requires the AI be able to
// explain what it chose and why; storing the reason costs nothing and turns "the AI
// did nothing for a minute" from a guess into a readable trace.
struct AIDecision
{
    TickIndex Tick = 0;
    CommandType Command = CommandType::None;
    ContentId Content;
    AIStrategy Strategy = AIStrategy::ExpandEconomy;
    int32_t StrategyScore = 0;
    AIStrategy PreviousStrategy = AIStrategy::ExpandEconomy;
    std::string Reason;
};

class RA4AI_API AICommander
{
public:
    void Initialize(PlayerId InPlayer, AIProfile InProfile, uint64_t Seed);
    void Reset();

    void SetConfig(const AIConfig& InConfig) { Config = InConfig; }
    const AIConfig& GetConfig() const { return Config; }
    AIProfile GetProfile() const { return Profile; }
    AIStrategy GetActiveStrategy() const { return ActiveStrategy; }
    const TacticalOperation& GetActiveOperation() const { return ActiveOperation; }

    // Call once per simulation tick. Appends any commands for this tick; the caller
    // merges them into the CommandFrame alongside player and other AI commands.
    void Tick(const SimWorld& World, std::vector<Command>& OutCommands);

    const std::vector<AIDecision>& GetDecisionLog() const { return DecisionLog; }
    void SetDecisionLogLimit(size_t Limit) { DecisionLogLimit = Limit; }

    // Test hook: the doctrine/personality resolved from the commander's faction.
    // Invalid until the first Tick, when the world is ready.
    const FactionDoctrineDef& GetDoctrineForTesting() const { return Doctrine; }

    // Spatial awareness maps, updated on decision ticks.
    const ThreatMap& GetThreatMap() const { return Threats; }
    const ValueMap& GetValueMap() const { return Values; }

    // Behavioural profile of each observed opponent, accumulated from the SimEvents
    // this commander was entitled to see. Not a fog bypass: it summarises what was
    // done to us over time, never where the enemy currently is.
    const OpponentModel& GetOpponentModel() const { return Opponents; }

    // Last set of scored director recommendations. Exposed so the debug overlay can
    // explain the choice and tests can assert the directors were actually consulted.
    const DirectorBundle& GetDirectorRecs() const { return Directors; }

    // Most recent pre-engagement forecast. Only meaningful once HasBattleForecast().
    const BattleEstimate& GetBattleForecast() const { return BattleForecast; }
    bool HasBattleForecast() const { return bHasBattleForecast; }

private:
    // --- decision steps, in priority order -------------------------------
    bool TryPlaceFinishedStructure(const SimWorld& World, std::vector<Command>& Out);
    bool TryBuildEconomy(const SimWorld& World, std::vector<Command>& Out);
    bool TryBuildTech(const SimWorld& World, std::vector<Command>& Out);
    bool TryBuildDefence(const SimWorld& World, std::vector<Command>& Out);
    bool TryTrainArmy(const SimWorld& World, std::vector<Command>& Out);
    void CommandArmy(const SimWorld& World, std::vector<Command>& Out);

    // Squad-level operation lifecycle: assigns idle combat units in deterministic
    // order, stages them near base, then issues a coordinated AttackMove on the
    // last known enemy position.  All state is serialisable and driven by the
    // same per-unit CommandType commands a human issues.
    void ReconcileSquad(const SimWorld& World);
    void ComputeStagingPoint(const SimWorld& World);
    bool AllSquadAtStaging(const SimWorld& World) const;
    bool AnySquadNearTarget(const SimWorld& World, const TileCoord& TargetTile) const;
    void IssueSquadAttackMove(const SimWorld& World, const Vec2& Destination,
                              std::vector<Command>& Out);
    void IssueSquadRetreat(const SimWorld& World, std::vector<Command>& Out);
    void PruneRetreatedUnits(const SimWorld& World);

    bool ExecuteStrategy(AIStrategy Strategy, const SimWorld& World,
                         std::vector<Command>& Out);

    // Doctrine resolution: happens once, lazily on the first Tick, because the
    // commander's faction lives in the SimWorld and is only valid once a match is
    // set up. Until then every doctrine-dependent decision falls back to Config.
    void LoadDoctrine(const SimWorld& World);
    int32_t EffectiveTargetHarvesters() const;
    int32_t EffectiveAssaultArmySize() const;
    bool TryHarassRaid(const SimWorld& World, int32_t ArmySize, std::vector<Command>& Out);

    // --- world queries ----------------------------------------------------
    ContentId FindStructure(const SimWorld& World, bool (*Predicate)(const EntityDef&)) const;
    ContentId FindCombatUnit(const SimWorld& World) const;
    ContentId FindHarvesterUnit(const SimWorld& World) const;
    ContentId FindDefenceStructure(const SimWorld& World) const;

    int32_t CountOwned(const SimWorld& World, bool (*Predicate)(const EntityDef&)) const;
    int32_t CountOwnedUnits(const SimWorld& World, bool bCombatOnly) const;
    int32_t CountIdleCombatUnits(const SimWorld& World) const;
    int32_t CountQueued(const SimWorld& World, ContentId Content) const;
    EntityId FindOwnConstructionYard(const SimWorld& World) const;

    // What the commander believes about the enemy, drawn only from its fog-limited
    // memory. Returns false when nothing is currently believed to exist.
    struct KnownTarget
    {
        EntityId Entity;
        TileCoord Tile;          // last observed position, not the live one
        EntityKind Kind = EntityKind::Unit;
    };
    bool FindKnownEnemyTarget(KnownTarget& Out) const;

    // Sends a single unit to look for the enemy. Without this a fog-limited commander
    // never discovers anything and therefore never attacks -- scouting is what makes
    // the honest knowledge model playable rather than passive.
    bool TryScout(const SimWorld& World, std::vector<Command>& Out);
    TileCoord NextScoutWaypoint(const SimWorld& World) const;
    TileCoord GetAndAdvanceScoutWaypoint(const SimWorld& World);

    // Re-observes the world through the fog-limited view. Must be called before any
    // decision that depends on enemy information.
    void UpdateKnowledge(const SimWorld& World);

    // Runs the four directors for this decision tick and stores the bundle.
    void EvaluateDirectors(const SimWorld& World, const AIWorldAssessment& Assessment);

    // Estimates the outcome of committing the assigned squad against TargetTile,
    // using only remembered enemies. True when a forecast was produced.
    bool ForecastAssault(const SimWorld& World, const TileCoord& TargetTile);

    // Minimum win probability required before committing an assault.
    int32_t AssaultCommitThreshold() const;
    bool IsUnderAttack(const SimWorld& World) const;
    AIWorldAssessment BuildAssessment(const SimWorld& World) const;

    bool FindPlacementTile(const SimWorld& World, ContentId Structure, TileCoord& OutTile) const;

    bool QueueProduction(const SimWorld& World, ContentId Content, const char* Reason,
                          std::vector<Command>& Out);
    void Log(TickIndex Tick, CommandType Type, ContentId Content, const char* Reason);
    void LogIdleStrategyDecision(TickIndex Tick);

    PlayerId Player = 1;
    AIProfile Profile = AIProfile::Balanced;
    AIConfig Config;
    Random Rng;

    // Resolved on the first Tick; start invalid so that Doctrine/Personality copies
    // only happen after a matter tick for this commander.
    FactionDoctrineDef Doctrine;
    AIPersonality Personality;
    bool bDoctrineLoaded = false;
    TickIndex LastHarassRaidTick = 0;
    bool bHasHarassRaid = false;

    int32_t TicksSinceDecision = 0;
    AIStrategy ActiveStrategy = AIStrategy::ExpandEconomy;
    AIStrategy PreviousStrategyForDecision = AIStrategy::ExpandEconomy;
    int32_t ActiveStrategyScore = 0;
    bool bHasActiveStrategy = false;
    TickIndex LastUnderAttackTick = 0;
    bool bHasSeenAttack = false;

    TacticalOperation ActiveOperation;

    // The commander's ONLY legitimate source of enemy information. Held across ticks
    // because memory of what was seen is the whole point; rebuilt if the world object
    // itself changes (a new match reusing the same commander).
    std::unique_ptr<SimWorldView> Knowledge;
    const SimWorld* KnowledgeWorld = nullptr;
    int32_t TicksSinceMemoryUpdate = 0;

    EntityId ScoutUnit;
    int32_t ScoutWaypointIndex = 0;
    TickIndex LastScoutOrderTick = 0;
    bool bHasScoutOrder = false;

private:
    std::vector<AIDecision> DecisionLog;
    size_t DecisionLogLimit = 64;

    // Spatial awareness: threat from enemy forces and strategic value of positions.
    ThreatMap Threats;
    ValueMap Values;

    // Opponent modelling, director layer and battle forecasting. All advisory: they
    // bias scores and thresholds only, and still emit ordinary validated Commands.
    OpponentModel Opponents;
    DirectorBundle Directors;
    EconomyDirector EconomyDir;
    ScoutingDirector ScoutingDir;
    DefenseDirector DefenseDir;
    OffenseDirector OffenseDir;

    BattleEstimate BattleForecast;
    bool bHasBattleForecast = false;
};

} // namespace AI
} // namespace RA4
