// Copyright (c) Red Alert 4 project. Fast combat outcome prediction.
//
// Heuristic prediction from pre-computed stats: DPS, HP, armor, range advantage
// → win probability. All integer arithmetic, fully deterministic.

#include "RA4AI/BattlePredictor.h"

#include <algorithm>

#include "RA4Content/ContentDatabase.h"
#include "RA4Core/Ids.h"
#include "RA4Simulation/SimWorld.h"

namespace RA4
{
namespace AI
{

// ─── Private helpers ────────────────────────────────────────────────────────

int32_t BattlePredictor::ArmorEffectiveness(int32_t AttackerArmorClass,
                                            int32_t DefenderArmorClass)
{
    // Simple per-mille modifier. Higher armor class vs lower armor class = bonus.
    // Same class = 1000 (1.0x). Each class difference = ±15%.
    const int32_t Diff = AttackerArmorClass - DefenderArmorClass;
    // Clamp difference to [-3, +3] for bounded effect.
    const int32_t Clamped = std::max(-3, std::min(3, Diff));
    return 1000 + Clamped * 150;
}

BattleEstimate BattlePredictor::EstimateFromDpsAndHealth(const CombatantStats& A,
                                                         const CombatantStats& D)
{
    BattleEstimate Result;

    if (A.Count <= 0 || D.Count <= 0)
    {
        Result.WinProbability = A.Count > 0 ? 100 : 0;
        Result.EstimatedLosses = A.Count > 0 ? 0 : 100;
        Result.EnemyLosses = D.Count > 0 ? 0 : 100;
        Result.DurationTicks = 0;
        Result.Confidence = 0;
        return Result;
    }

    const int32_t TotalDpsA = A.Dps * A.Count;
    const int32_t TotalDpsD = D.Dps * D.Count;
    const int32_t TotalHealthA = A.Health * A.Count;
    const int32_t TotalHealthD = D.Health * D.Count;

    if (TotalDpsA + TotalDpsD == 0)
    {
        // No damage on either side — stalemate.
        Result.WinProbability = 50;
        Result.EstimatedLosses = 0;
        Result.EnemyLosses = 0;
        Result.DurationTicks = 0;
        Result.Confidence = 10;
        return Result;
    }

    // DPS ratio × 1000 for precision.
    const int32_t DpsRatio = (TotalDpsA * 1000) / (TotalDpsA + TotalDpsD);

    // Armor modifier: average attacker effectiveness vs defender.
    const int32_t ArmorMod = ArmorEffectiveness(A.ArmorValue, D.ArmorValue);
    const int32_t EffectiveDpsRatio = (DpsRatio * ArmorMod) / 1000;

    // Range advantage: if attacker outranges defender, +10% win probability.
    const int32_t RangeAdvantage = A.Range > D.Range ? 100 : (A.Range < D.Range ? -100 : 0);

    // Convert DPS ratio to win probability (0..100).
    int32_t WinProb = (EffectiveDpsRatio * 80) / 1000 + 10 + RangeAdvantage;
    WinProb = std::max(5, std::min(95, WinProb));

    // Estimated losses based on how long the fight lasts.
    // Time to kill attacker force: TotalHealthA / TotalDpsD
    // Time to kill defender force: TotalHealthD / TotalDpsA
    const int32_t TimeToKillA = TotalDpsD > 0 ? TotalHealthA / TotalDpsD : 999;
    const int32_t TimeToKillD = TotalDpsA > 0 ? TotalHealthD / TotalDpsA : 999;
    const int32_t FightDuration = std::min(TimeToKillA, TimeToKillD);

    // Loss percentage = DPS × duration / health × 100
    Result.EstimatedLosses = TotalHealthA > 0
        ? std::min(100, (TotalDpsD * FightDuration * 100) / TotalHealthA)
        : 100;
    Result.EnemyLosses = TotalHealthD > 0
        ? std::min(100, (TotalDpsA * FightDuration * 100) / TotalHealthD)
        : 100;
    Result.DurationTicks = FightDuration;
    Result.WinProbability = WinProb;
    Result.Confidence = 80;

    return Result;
}

// ─── Public API ─────────────────────────────────────────────────────────────

BattleEstimate BattlePredictor::PredictFromStats(const CombatantStats& Attacker,
                                                 const CombatantStats& Defender)
{
    return EstimateFromDpsAndHealth(Attacker, Defender);
}

CombatantStats BattlePredictor::GatherStats(const SimWorld& World,
                                            const std::vector<EntityId>& Entities)
{
    CombatantStats Stats;
    Stats.Count = static_cast<int32_t>(Entities.size());

    const ContentDatabase* Content = World.GetContent();
    if (!Content)
    {
        Stats.Count = 0;
        return Stats;
    }

    int32_t TotalHealth = 0;
    int32_t TotalDps = 0;
    int32_t TotalRange = 0;
    int32_t ArmorSum = 0;

    for (const EntityId& Id : Entities)
    {
        if (!World.IsAlive(Id))
        {
            Stats.Count--;
            continue;
        }

        const EntityCore* Core = World.GetCore(Id);
        const HealthComp* HP = World.GetHealth(Id);

        if (HP)
        {
            TotalHealth += HP->Current;
        }

        if (Core)
        {
            const EntityDef* Def = Content->FindEntity(Core->Def);
            if (Def)
            {
                ArmorSum += static_cast<int32_t>(Def->Armor);

                const WeaponDef* Weapon = Content->FindWeapon(Def->Weapon);
                if (Weapon)
                {
                    // DPS = Damage × BurstCount / CooldownTicks, scaled ×100.
                    const int32_t BurstDmg = Weapon->Damage * Weapon->BurstCount;
                    const int32_t Cdt = Weapon->CooldownTicks > 0 ? Weapon->CooldownTicks : 1;
                    TotalDps += (BurstDmg * 100) / Cdt;
                    TotalRange += Weapon->MaxRange.ToIntRound();
                    // Both flags belong INSIDE the null check: an unarmed unit in
                    // the list (e.g. a harvester swept into an assault) previously
                    // dereferenced Weapon here and crashed the whole match. Found
                    // by the self-play league on its first real run.
                    Stats.bHasAntiAir = Stats.bHasAntiAir || Weapon->bCanTargetAir;
                    Stats.bHasAntiArmor = Stats.bHasAntiArmor ||
                        Weapon->Warhead == WarheadClass::ArmorPiercing;
                }
            }
        }
    }

    Stats.Health = Stats.Count > 0 ? TotalHealth / Stats.Count : 0;
    Stats.Dps = Stats.Count > 0 ? TotalDps / Stats.Count : 0;
    Stats.Range = Stats.Count > 0 ? TotalRange / Stats.Count : 0;
    Stats.ArmorValue = Stats.Count > 0 ? ArmorSum / Stats.Count : 0;

    return Stats;
}

BattleEstimate BattlePredictor::PredictFromWorld(const SimWorld& World,
                                                 PlayerId Self, PlayerId Enemy,
                                                 const std::vector<EntityId>& Attackers,
                                                 const std::vector<EntityId>& Defenders)
{
    const CombatantStats AStats = GatherStats(World, Attackers);
    const CombatantStats DStats = GatherStats(World, Defenders);

    BattleEstimate Est = EstimateFromDpsAndHealth(AStats, DStats);

    // Lower confidence for heuristic prediction vs full sim.
    Est.Confidence = 60;

    return Est;
}

} // namespace AI
} // namespace RA4
