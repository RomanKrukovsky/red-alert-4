// Copyright (c) Red Alert 4 project.
#include "RA4AI/AIStrategy.h"

#include <algorithm>

namespace RA4
{
namespace AI
{

namespace
{

int32_t ClampScore(int32_t Score)
{
    return std::clamp(Score, 0, 1000);
}

int32_t ApplyWeight(int32_t Score, int32_t Weight)
{
    return ClampScore((Score * Weight) / 100);
}

} // namespace

const char* ToString(AIProfile Profile)
{
    switch (Profile)
    {
        case AIProfile::Adaptive: return "Adaptive";
        case AIProfile::Aggressive: return "Aggressive";
        case AIProfile::Defensive: return "Defensive";
        case AIProfile::Economic: return "Economic";
    }
    return "Invalid";
}

AIConfig MakeProfileConfig(AIProfile Profile)
{
    AIConfig Config;
    switch (Profile)
    {
        case AIProfile::Aggressive:
            Config.TargetHarvesters = 2;
            Config.AttackArmySize = 4;
            Config.MinimumAttackSize = 2;
            Config.TargetDefences = 0;
            Config.CreditReserve = 100;
            Config.AssaultWeight = 125;
            Config.ArmyWeight = 115;
            Config.EconomyWeight = 85;
            Config.DefenceWeight = 70;
            Config.StrategySwitchMargin = 60;
            break;
        case AIProfile::Defensive:
            Config.TargetHarvesters = 3;
            Config.AttackArmySize = 10;
            Config.MinimumAttackSize = 6;
            Config.TargetDefences = 4;
            Config.DefenceWeight = 130;
            Config.ArmyWeight = 110;
            Config.AssaultWeight = 80;
            Config.StrategySwitchMargin = 120;
            break;
        case AIProfile::Economic:
            Config.TargetHarvesters = 5;
            Config.AttackArmySize = 6;
            Config.MinimumAttackSize = 3;
            Config.TargetDefences = 1;
            Config.CreditReserve = 800;
            Config.EconomyWeight = 130;
            Config.TechWeight = 110;
            Config.AssaultWeight = 90;
            Config.RecoveryWeight = 120;
            Config.StrategySwitchMargin = 120;
            break;
        case AIProfile::Adaptive:
            break;
    }
    return Config;
}

const char* ToString(AIStrategy Strategy)
{
    switch (Strategy)
    {
        case AIStrategy::ExpandEconomy: return "ExpandEconomy";
        case AIStrategy::TechUp: return "TechUp";
        case AIStrategy::Fortify: return "Fortify";
        case AIStrategy::AssembleArmy: return "AssembleArmy";
        case AIStrategy::Assault: return "Assault";
        case AIStrategy::Recover: return "Recover";
    }
    return "Invalid";
}

std::vector<AIStrategyScore> ScoreStrategies(
    const AIWorldAssessment& Assessment, const AIConfig& Config)
{
    const int32_t PowerScore =
        Assessment.PowerProduced <= Assessment.PowerConsumed ? 900 : 0;
    const int32_t RefineryScore = Assessment.Refineries == 0 ? 850 : 0;
    const int32_t HarvesterDeficit =
        std::max(0, Config.TargetHarvesters - Assessment.Harvesters);
    const int32_t HarvesterScore =
        HarvesterDeficit > 0 && Assessment.bCanProduceHarvester
            ? 600 + 50 * HarvesterDeficit
            : 0;
    const int32_t EconomyScore =
        std::max({PowerScore, RefineryScore, HarvesterScore});

    const int32_t MinimumReadyHarvesters =
        std::min(Config.TargetHarvesters, 1);
    const int32_t TechScore =
        Assessment.Refineries > 0 &&
                Assessment.Harvesters >= MinimumReadyHarvesters
            ? (Assessment.ProductionBuildings < 2 ? 700 : 250)
            : 0;

    const int32_t DefenceDeficit =
        std::max(0, Config.TargetDefences - Assessment.Defences);
    const int32_t DefenceScore = Assessment.bUnderAttack
                                     ? 1000
                                     : (DefenceDeficit > 0
                                            ? 300 + 50 * DefenceDeficit
                                            : 0);

    const int32_t ArmyDeficit =
        std::max(0, Config.AttackArmySize - Assessment.ArmedUnits);
    const int32_t ArmyScore =
        Assessment.ProductionBuildings > 0 && ArmyDeficit > 0
            ? 550 + 25 * ArmyDeficit
            : 100;

    int32_t AssaultScore = 0;
    if (Assessment.bHasEnemyTarget &&
        Assessment.ArmedUnits >= Config.AttackArmySize)
    {
        AssaultScore =
            750 + 20 * (Assessment.ArmedUnits - Config.AttackArmySize);
    }
    else if (Assessment.bAssaultActive &&
             Assessment.ArmedUnits >= Config.MinimumAttackSize)
    {
        AssaultScore = 700;
    }

    const int32_t RecoveryScore =
        Assessment.TotalHarvested > 0 &&
                (Assessment.Refineries == 0 || Assessment.PowerPlants == 0)
            ? 950
            : 0;

    return {
        {AIStrategy::ExpandEconomy,
         ApplyWeight(EconomyScore, Config.EconomyWeight),
         "economy needs investment"},
        {AIStrategy::TechUp,
         ApplyWeight(TechScore, Config.TechWeight),
         "production technology is missing"},
        {AIStrategy::Fortify,
         ApplyWeight(DefenceScore, Config.DefenceWeight),
         Assessment.bUnderAttack ? "base is under attack"
                                 : "defences are below target"},
        {AIStrategy::AssembleArmy,
         ApplyWeight(ArmyScore, Config.ArmyWeight),
         "army is below attack strength"},
        {AIStrategy::Assault,
         ApplyWeight(AssaultScore, Config.AssaultWeight),
         "army is ready to assault"},
        {AIStrategy::Recover,
         ApplyWeight(RecoveryScore, Config.RecoveryWeight),
         "established economy lost critical infrastructure"},
    };
}

AIStrategy FindWinningStrategy(const std::vector<AIStrategyScore>& Scores)
{
    AIStrategy Winner = AIStrategy::ExpandEconomy;
    int32_t WinningScore = -1;
    for (const AIStrategyScore& Candidate : Scores)
    {
        if (Candidate.Score > WinningScore)
        {
            Winner = Candidate.Strategy;
            WinningScore = Candidate.Score;
        }
    }
    return Winner;
}

int32_t FindStrategyScore(const std::vector<AIStrategyScore>& Scores,
                          AIStrategy Strategy)
{
    for (const AIStrategyScore& Candidate : Scores)
    {
        if (Candidate.Strategy == Strategy)
        {
            return Candidate.Score;
        }
    }
    return 0;
}

AIStrategy SelectStrategy(const std::vector<AIStrategyScore>& Scores,
                          AIStrategy CurrentStrategy,
                          bool bHasCurrentStrategy,
                          const AIConfig& Config)
{
    if (Scores.empty())
    {
        return CurrentStrategy;
    }

    const AIStrategy Winner = FindWinningStrategy(Scores);
    if (!bHasCurrentStrategy)
    {
        return Winner;
    }

    const int32_t WinnerScore = FindStrategyScore(Scores, Winner);
    if ((Winner == AIStrategy::Fortify || Winner == AIStrategy::Recover) &&
        WinnerScore >= Config.EmergencyStrategyScore)
    {
        return Winner;
    }

    const int32_t CurrentScore = FindStrategyScore(Scores, CurrentStrategy);
    if (WinnerScore < CurrentScore + Config.StrategySwitchMargin)
    {
        return CurrentStrategy;
    }
    return Winner;
}

int32_t RequiredCreditReserve(AIStrategy Strategy, const AIConfig& Config)
{
    return Strategy == AIStrategy::Recover ? 0 : Config.CreditReserve;
}

} // namespace AI
} // namespace RA4
