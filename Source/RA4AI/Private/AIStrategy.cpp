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
        case AIProfile::Rush: return "Rush";
        case AIProfile::Turtle: return "Turtle";
        case AIProfile::AirSuperiority: return "AirSuperiority";
        case AIProfile::Guerrilla: return "Guerrilla";
    }
    return "Invalid";
}

const char* ToString(AIDifficulty Difficulty)
{
    switch (Difficulty)
    {
        case AIDifficulty::Easy: return "Easy";
        case AIDifficulty::Normal: return "Normal";
        case AIDifficulty::Hard: return "Hard";
        case AIDifficulty::Expert: return "Expert";
    }
    return "Normal";
}

AIConfig MakeProfileConfig(AIProfile Profile, AIDifficulty Difficulty)
{
    AIConfig Config;
    Config.Difficulty = Difficulty;

    // Difficulty tuning
    switch (Difficulty)
    {
        case AIDifficulty::Easy:
            Config.DecisionIntervalTicks = 20;
            Config.MemoryUpdateIntervalTicks = 10;
            break;
        case AIDifficulty::Normal:
            Config.DecisionIntervalTicks = 10;
            Config.MemoryUpdateIntervalTicks = 5;
            break;
        case AIDifficulty::Hard:
            // Hard earns its advantage: faster reaction and observation only. It gets
            // no income bonus -- difficulty must change how well the AI plays, never
            // what it is handed. (The old +20% multiplier was also dead code: nothing
            // ever read it, so removing it changes no behaviour, only the intent.)
            Config.DecisionIntervalTicks = 5;
            Config.MemoryUpdateIntervalTicks = 2;
            break;
        case AIDifficulty::Expert:
            // Fastest reaction and observation of any tier, and deliberately NO
            // credit bonus: Expert must win by playing better, not by being fed.
            Config.DecisionIntervalTicks = 3;
            Config.MemoryUpdateIntervalTicks = 1;
            // Longer memory: an Expert commander keeps acting on older sightings
            // instead of forgetting a base it scouted a minute ago.
            Config.MemoryRetentionTicks = 900;
            // Less thrash between strategies, so plans are carried through.
            Config.StrategySwitchMargin = 140;
            break;
    }

    // Profile tuning
    switch (Profile)
    {
        case AIProfile::Aggressive:
            // Tuning note (league pass 2, 560 matches per variant): raising the
            // commit floor was tried to stop unit pairs trickling into static
            // defence, and MEASURABLY BACKFIRED at both floor 4 (a regression
            // scenario stopped finishing at all) and floor 3 (win rate 40% -> 34%,
            // first-blood rate 42% -> 21%, Defensive and Turtle rows went to 0%).
            // Waiting for a third unit forfeits the early-pressure timing that IS
            // this profile's identity: damage per game rose but arrived after
            // walls existed. Kept at 2 deliberately -- the trickle is the cost of
            // the timing, and fixing Aggressive-vs-Turtle belongs to the armor
            // matrix / anti-building tools, not to slower openings.
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
        case AIProfile::Rush:
            // All-in early aggression: the smallest viable economy, the smallest
            // committing force, and almost no reserve. Loses to anything that
            // survives the opening, which is the intended counterplay.
            //
            // League pass 1 (560 matches, factions alternated): 36% win rate,
            // the weakest profile. Root cause was not the aggression but the
            // economy: one harvester cannot fund a second wave, so a single
            // failed opening ended the match. Two harvesters keeps the identity
            // (still the smallest economy of any profile) while making the rush
            // repeatable instead of a coin flip.
            Config.TargetHarvesters = 2;
            Config.AttackArmySize = 3;
            Config.MinimumAttackSize = 2;
            Config.TargetDefences = 0;
            Config.CreditReserve = 50;
            Config.AssaultWeight = 150;
            Config.ArmyWeight = 130;
            Config.EconomyWeight = 60;
            Config.DefenceWeight = 40;
            Config.TechWeight = 50;
            // Commits and stays committed: re-deciding mid-rush wastes the timing.
            Config.StrategySwitchMargin = 40;
            break;
        case AIProfile::Turtle:
            // Trades all early initiative for defence and tech, then attacks with a
            // much larger force than any other profile fields.
            //
            // League pass 1: 72% win rate, the strongest profile. With the current
            // content set six turrets are effectively uncrackable, so Turtle never
            // paid a price for surrendering the initiative. Five keeps it the most
            // fortified profile (Defensive holds four -- the profile invariant
            // TurtleIsTheMostDefensive caught an attempt to tie them) while
            // leaving attackers a real, expensive way through.
            Config.TargetHarvesters = 4;
            Config.AttackArmySize = 14;
            Config.MinimumAttackSize = 8;
            Config.TargetDefences = 5;
            Config.CreditReserve = 600;
            Config.DefenceWeight = 160;
            Config.TechWeight = 125;
            Config.EconomyWeight = 110;
            Config.AssaultWeight = 60;
            Config.ArmyWeight = 95;
            Config.RecoveryWeight = 130;
            // Very reluctant to abandon a fortified posture.
            Config.StrategySwitchMargin = 180;
            break;
        case AIProfile::AirSuperiority:
            // Prioritises tech to reach air, keeps a healthy bank to afford it, and
            // declines early ground engagements it would lose.
            Config.TargetHarvesters = 4;
            Config.AttackArmySize = 8;
            Config.MinimumAttackSize = 4;
            Config.TargetDefences = 2;
            Config.CreditReserve = 700;
            Config.TechWeight = 165;
            Config.EconomyWeight = 115;
            Config.ArmyWeight = 100;
            Config.AssaultWeight = 85;
            Config.DefenceWeight = 105;
            Config.StrategySwitchMargin = 130;
            break;
        case AIProfile::Guerrilla:
            // Never masses for a decisive battle: many small raids, a low commit
            // threshold, and a deliberately thin defensive line.
            //
            // League pass 2 telemetry: 42% win rate with the lowest share of
            // damage landing on buildings (25%) -- raids that harass but never
            // finish. Raid size 3 keeps the many-small-raids identity (still the
            // smallest committing force alongside Rush's opening) while giving a
            // raid enough punch to actually kill what it catches.
            Config.TargetHarvesters = 3;
            Config.AttackArmySize = 4;
            Config.MinimumAttackSize = 3;
            Config.TargetDefences = 1;
            Config.CreditReserve = 200;
            Config.AssaultWeight = 120;
            Config.ArmyWeight = 108;
            Config.EconomyWeight = 100;
            Config.DefenceWeight = 75;
            Config.RecoveryWeight = 115;
            // Switches targets readily -- that mobility is the whole identity.
            Config.StrategySwitchMargin = 50;
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
        case AIStrategy::Opening: return "Opening";
        case AIStrategy::ExpandEconomy: return "ExpandEconomy";
        case AIStrategy::Expansion: return "Expansion";
        case AIStrategy::TechUp: return "TechUp";
        case AIStrategy::Fortify: return "Fortify";
        case AIStrategy::AssembleArmy: return "AssembleArmy";
        case AIStrategy::Assault: return "Assault";
        case AIStrategy::Recover: return "Recover";
        case AIStrategy::FinalAssault: return "FinalAssault";
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

    const int32_t OpeningScore =
        (!Assessment.bHasConstructionYard) ? 950 : 0;

    const int32_t ExpansionScore =
        (Assessment.Refineries >= 1 && Assessment.Harvesters >= 3 && Assessment.Credits >= 2000) ? 500 : 0;

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

    const int32_t FinalAssaultScore =
        (Assessment.bHasEnemyTarget && Assessment.ArmedUnits >= Config.AttackArmySize * 2) ? 920 : 0;

    const int32_t RecoveryScore =
        Assessment.TotalHarvested > 0 &&
                (Assessment.Refineries == 0 || Assessment.PowerPlants == 0)
            ? 980
            : 0;

    return {
        {AIStrategy::Opening,
         ApplyWeight(OpeningScore, Config.EconomyWeight),
         "opening build sequence"},
        {AIStrategy::ExpandEconomy,
         ApplyWeight(EconomyScore, Config.EconomyWeight),
         "economy needs investment"},
        {AIStrategy::Expansion,
         ApplyWeight(ExpansionScore, Config.EconomyWeight),
         "expanding to secondary ore node"},
        {AIStrategy::TechUp,
         ApplyWeight(TechScore, Config.TechWeight),
         "production technology is missing"},
        {AIStrategy::Fortify,
         ApplyWeight(DefenceScore, Config.DefenceWeight),
         Assessment.bUnderAttack ? "base is under attack"
                                 : "defences are below target"},
        {AIStrategy::AssembleArmy,
         ApplyWeight(ArmyScore, Config.ArmyWeight),
         "assembling assault force"},
        {AIStrategy::Assault,
         ApplyWeight(AssaultScore, Config.AssaultWeight),
         "launching assault on known enemy"},
        {AIStrategy::Recover,
         ApplyWeight(RecoveryScore, Config.RecoveryWeight),
         "established economy lost critical infrastructure"},
        {AIStrategy::FinalAssault,
         ApplyWeight(FinalAssaultScore, Config.AssaultWeight),
         "overwhelming force pushing for victory"}
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
