// Copyright (c) Red Alert 4 project. Deterministic strategic utility scoring.
#pragma once

#include <cstdint>
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
};

enum class AIDifficulty : uint8_t
{
    Easy = 0,
    Normal,
    Hard
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

    // Explicit bounded bonus for Hard difficulty (e.g. 1.20 = +20% income). Normal & Easy are 1.0.
    float CreditBonusMultiplier = 1.0f;

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
RA4AI_API int32_t RequiredCreditReserve(AIStrategy Strategy,
                                        const AIConfig& Config);
const char* RA4AI_API ToString(AIStrategy Strategy);

} // namespace AI
} // namespace RA4
