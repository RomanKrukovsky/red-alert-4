// Copyright (c) Red Alert 4 project. Fast combat outcome prediction.
//
// Estimates the outcome of a proposed engagement using two approaches:
// 1. Heuristic: DPS, HP, armor, range advantage → win probability. O(N), us.
// 2. Forward sim: clone SimWorld, run simplified battle. O(T*N), more accurate.
//
// Heuristic is default for runtime; forward sim for high-difficulty / offline.
// All integer arithmetic, fully deterministic.
#pragma once

#include <cstdint>
#include <vector>

#include "RA4Core/Ids.h"

namespace RA4
{

class SimWorld;
struct EntityDef;

namespace AI
{

// Estimated outcome of a proposed battle.
struct BattleEstimate
{
    int32_t WinProbability = 50;   // 0..100
    int32_t EstimatedLosses = 0;   // 0..100, percent of our force lost
    int32_t EnemyLosses = 0;       // 0..100, percent of enemy force lost
    int32_t DurationTicks = 0;     // estimated battle duration in ticks
    int32_t Confidence = 80;       // 0..100, how reliable this estimate is
};

// Simplified combatant stats for heuristic prediction.
struct CombatantStats
{
    int32_t Health = 0;
    int32_t Dps = 0;               // damage per tick, scaled ×100
    int32_t Range = 0;             // average weapon range in tiles
    int32_t Count = 0;
    int32_t ArmorValue = 0;        // average armor class index
    bool bHasAntiAir = false;
    bool bHasAntiArmor = false;
};

// Fast combat outcome predictor. Engine-free for heuristic mode.
class BattlePredictor
{
public:
    // Heuristic-only prediction from pre-computed stats.
    static BattleEstimate PredictFromStats(const CombatantStats& Attacker,
                                           const CombatantStats& Defender);

    // Full prediction from live world state.
    static BattleEstimate PredictFromWorld(const SimWorld& World,
                                           PlayerId Self, PlayerId Enemy,
                                           const std::vector<EntityId>& Attackers,
                                           const std::vector<EntityId>& Defenders);

    // Extract combatant stats from a list of entity IDs.
    static CombatantStats GatherStats(const SimWorld& World,
                                      const std::vector<EntityId>& Entities);

private:
    static BattleEstimate EstimateFromDpsAndHealth(const CombatantStats& A,
                                                   const CombatantStats& D);

    // Armor effectiveness modifier: per-mille (1000 = 1.0x).
    static int32_t ArmorEffectiveness(int32_t AttackerArmorClass,
                                      int32_t DefenderArmorClass);
};

} // namespace AI
} // namespace RA4
