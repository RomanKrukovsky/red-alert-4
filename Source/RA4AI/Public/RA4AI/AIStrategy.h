// Copyright (c) Red Alert 4 project. Deterministic strategic utility scoring.
#pragma once

#include <cstdint>
#include "RA4Core/Ids.h"
#include <vector>

#ifndef RA4AI_API
#define RA4AI_API
#endif

namespace RA4
{
namespace AI
{

enum class AIProfile : uint8_t
{
    Adaptive = 0,
    Balanced = Adaptive,
    Aggressive,
    Defensive,
    Economic,

    // Extended personalities. Each is a distinct *shape* of play rather than a
    // difficulty setting: they trade economy, timing and force composition against
    // each other, so a player who beats one should not automatically beat the next.
    Rush,            // earliest possible pressure, minimal economy, accepts losses
    Turtle,          // deep defence, late but overwhelming push
    AirSuperiority,  // techs toward air, keeps a reserve, avoids early ground fights
    Guerrilla,       // constant small raids on economy, avoids pitched battles
};

enum class AIDifficulty : uint8_t
{
    Easy = 0,
    Normal,
    Hard,
    // Expert scales judgement, not entitlements: it reacts sooner, re-observes more
    // often, and demands a favourable battle forecast before committing. It receives
    // no income bonus and no extra vision -- per the design rule that difficulty must
    // change how well the AI plays, never what it is allowed to know or take.
    Expert
};

struct AIConfig
{
    AIDifficulty Difficulty = AIDifficulty::Normal;
    int32_t DecisionIntervalTicks = 10;

    // How often the commander re-observes the world through its fog-limited view, and
    // how long an unrefreshed sighting survives. Sampling is decoupled from decision
    // making on purpose: observation must be frequent enough not to miss a unit
    // crossing a vision cone, while full strategy scoring stays comparatively rare.
    int32_t MemoryUpdateIntervalTicks = 5;
    int32_t MemoryRetentionTicks = 600;   // 30 s at 20 Hz

    int32_t TargetHarvesters = 3;
    int32_t AttackArmySize = 6;
    int32_t MinimumAttackSize = 3;
    int32_t TargetDefences = 2;
    int32_t CreditReserve = 300;

    int32_t StrategySwitchMargin = 100;
    int32_t EmergencyStrategyScore = 900;
    int32_t UnderAttackMemoryTicks = 100;

    int32_t EconomyWeight = 100;
    int32_t TechWeight = 100;
    int32_t DefenceWeight = 100;
    int32_t ArmyWeight = 100;
    int32_t AssaultWeight = 100;
    int32_t RecoveryWeight = 100;
};

enum class AIStrategy : uint8_t
{
    Opening = 0,
    ExpandEconomy,
    Expansion,
    TechUp,
    Fortify,
    AssembleArmy,
    Assault,
    Recover,
    FinalAssault,
};

struct AIWorldAssessment
{
    int32_t Credits = 0;
    int32_t PowerProduced = 0;
    int32_t PowerConsumed = 0;
    int32_t TotalHarvested = 0;
    int32_t PowerPlants = 0;
    int32_t Refineries = 0;
    int32_t Harvesters = 0;
    int32_t ProductionBuildings = 0;
    int32_t Defences = 0;
    int32_t ArmedUnits = 0;
    bool bHasConstructionYard = false;
    bool bHasEnemyTarget = false;
    bool bCanProduceHarvester = false;
    bool bUnderAttack = false;
    bool bAssaultActive = false;
};

struct AIStrategyScore
{
    AIStrategy Strategy = AIStrategy::ExpandEconomy;
    int32_t Score = 0;
    const char* Reason = "";
};

RA4AI_API AIConfig MakeProfileConfig(AIProfile Profile, AIDifficulty Difficulty = AIDifficulty::Normal);
const char* RA4AI_API ToString(AIProfile Profile);
const char* RA4AI_API ToString(AIDifficulty Difficulty);

RA4AI_API std::vector<AIStrategyScore> ScoreStrategies(
    const AIWorldAssessment& Assessment, const AIConfig& Config);
RA4AI_API AIStrategy FindWinningStrategy(const std::vector<AIStrategyScore>& Scores);
RA4AI_API AIStrategy SelectStrategy(const std::vector<AIStrategyScore>& Scores,
                                    AIStrategy CurrentStrategy,
                                    bool bHasCurrentStrategy,
                                    const AIConfig& Config);
RA4AI_API int32_t FindStrategyScore(const std::vector<AIStrategyScore>& Scores,
                                    AIStrategy Strategy);
/// Commitment test for a squad still gathering below its minimum size. Returns
/// true when the squad is at strength, or when its roster has been stable below
/// minimum for long enough that reinforcements are not coming and attacking with
/// what exists beats idling.
RA4AI_API bool ShouldCommitStaleGather(size_t AliveCount, int32_t MinCommitUnits,
                                       TickIndex SinceLastRosterChangeTicks);
RA4AI_API int32_t RequiredCreditReserve(AIStrategy Strategy,
                                        const AIConfig& Config);
const char* RA4AI_API ToString(AIStrategy Strategy);

} // namespace AI
} // namespace RA4
