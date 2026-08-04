// Copyright (c) Red Alert 4 project. AI Director subsystem implementations.
//
// Each Director evaluates one domain (economy, scouting, defence, offence) and
// produces scored recommendations. Directors are pure logic — no engine
// dependency, fully deterministic, testable in isolation.
#include "RA4AI/AIDirectors.h"

#include <algorithm>
#include <string>

namespace RA4
{
namespace AI
{

// ---------------------------------------------------------------------------
// ToString
// ---------------------------------------------------------------------------

const char* ToString(DirectorRecommendation Rec)
{
    switch (Rec)
    {
    case DirectorRecommendation::None:              return "None";
    case DirectorRecommendation::BuildHarvester:    return "BuildHarvester";
    case DirectorRecommendation::BuildRefinery:     return "BuildRefinery";
    case DirectorRecommendation::BuildPowerPlant:   return "BuildPowerPlant";
    case DirectorRecommendation::ExpandBase:        return "ExpandBase";
    case DirectorRecommendation::ScoutArea:         return "ScoutArea";
    case DirectorRecommendation::ScoutEnemyBase:    return "ScoutEnemyBase";
    case DirectorRecommendation::BuildDefence:      return "BuildDefence";
    case DirectorRecommendation::RecallArmy:        return "RecallArmy";
    case DirectorRecommendation::FortifyPosition:   return "FortifyPosition";
    case DirectorRecommendation::AttackTarget:      return "AttackTarget";
    case DirectorRecommendation::HarassEconomy:     return "HarassEconomy";
    case DirectorRecommendation::RegroupArmy:       return "RegroupArmy";
    case DirectorRecommendation::Count:             return "Count";
    }
    return "Unknown";
}

// ---------------------------------------------------------------------------
// DirectorBundle
// ---------------------------------------------------------------------------

const DirectorRec* DirectorBundle::FindBest() const
{
    const DirectorRec* Best = nullptr;

    auto Check = [&Best](const std::vector<DirectorRec>& Recs)
    {
        for (const DirectorRec& R : Recs)
        {
            if (Best == nullptr || R.Score > Best->Score)
            {
                Best = &R;
            }
        }
    };

    Check(EconomyRecs);
    Check(ScoutingRecs);
    Check(DefenseRecs);
    Check(OffenseRecs);
    return Best;
}

const DirectorRec* DirectorBundle::FindBestOfType(DirectorRecommendation Type) const
{
    const DirectorRec* Best = nullptr;

    auto Check = [&Best, Type](const std::vector<DirectorRec>& Recs)
    {
        for (const DirectorRec& R : Recs)
        {
            if (R.Type == Type && (Best == nullptr || R.Score > Best->Score))
            {
                Best = &R;
            }
        }
    };

    Check(EconomyRecs);
    Check(ScoutingRecs);
    Check(DefenseRecs);
    Check(OffenseRecs);
    return Best;
}

// ---------------------------------------------------------------------------
// EconomyDirector
// ---------------------------------------------------------------------------

int32_t EconomyDirector::EvaluateHarvesterNeed(const DirectorContext& Ctx) const
{
    if (Ctx.Assessment == nullptr) return 0;
    const int32_t Deficit = Ctx.TargetHarvesters - Ctx.Assessment->Harvesters;
    if (Deficit <= 0) return 0;
    return 300 + Deficit * 200;
}

int32_t EconomyDirector::EvaluateRefineryNeed(const DirectorContext& Ctx) const
{
    if (Ctx.Assessment == nullptr) return 0;
    if (Ctx.Assessment->Refineries == 0 && Ctx.Assessment->bCanProduceHarvester)
    {
        return 800;
    }
    if (Ctx.Assessment->Refineries > 0 && Ctx.Assessment->Harvesters >= Ctx.Assessment->Refineries * 2)
    {
        return 400;
    }
    return 0;
}

int32_t EconomyDirector::EvaluatePowerNeed(const DirectorContext& Ctx) const
{
    if (Ctx.Assessment == nullptr) return 0;
    if (Ctx.Assessment->PowerConsumed > 0 && Ctx.Assessment->PowerProduced < Ctx.Assessment->PowerConsumed)
    {
        const int32_t Ratio = (Ctx.Assessment->PowerProduced * 100) / Ctx.Assessment->PowerConsumed;
        if (Ratio < 50) return 700;
        return 450;
    }
    return 0;
}

int32_t EconomyDirector::EvaluateExpansionNeed(const DirectorContext& Ctx) const
{
    if (Ctx.Assessment == nullptr) return 0;
    if (!Ctx.Assessment->bHasConstructionYard) return 0;
    if (Ctx.Assessment->Credits > Ctx.CreditReserve * 3 && Ctx.Assessment->Refineries >= 2)
    {
        return 300;
    }
    return 0;
}

std::vector<DirectorRec> EconomyDirector::Evaluate(const DirectorContext& Ctx) const
{
    std::vector<DirectorRec> Recs;

    const int32_t HarvesterScore = EvaluateHarvesterNeed(Ctx);
    if (HarvesterScore > 0)
    {
        Recs.push_back({DirectorRecommendation::BuildHarvester, HarvesterScore, {}, "harvester deficit"});
    }

    const int32_t RefineryScore = EvaluateRefineryNeed(Ctx);
    if (RefineryScore > 0)
    {
        Recs.push_back({DirectorRecommendation::BuildRefinery, RefineryScore, {}, "refinery needed"});
    }

    const int32_t PowerScore = EvaluatePowerNeed(Ctx);
    if (PowerScore > 0)
    {
        Recs.push_back({DirectorRecommendation::BuildPowerPlant, PowerScore, {}, "power shortage"});
    }

    const int32_t ExpansionScore = EvaluateExpansionNeed(Ctx);
    if (ExpansionScore > 0)
    {
        Recs.push_back({DirectorRecommendation::ExpandBase, ExpansionScore, {}, "economy expansion"});
    }

    return Recs;
}

// ---------------------------------------------------------------------------
// ScoutingDirector
// ---------------------------------------------------------------------------

int32_t ScoutingDirector::EvaluateStaleIntel(const DirectorContext& Ctx) const
{
    if (Ctx.KnownEnemies == nullptr || Ctx.KnownEnemies->empty()) return 700;

    TickIndex MostRecent = 0;
    for (const EnemyMemory& Mem : *Ctx.KnownEnemies)
    {
        if (Mem.LastSeenTick > MostRecent) MostRecent = Mem.LastSeenTick;
    }

    const TickIndex Age = Ctx.CurrentTick > MostRecent ? Ctx.CurrentTick - MostRecent : 0;
    if (Age > 300) return 600;
    if (Age > 100) return 300;
    return 0;
}

int32_t ScoutingDirector::EvaluateUnknownSectors(const DirectorContext& Ctx) const
{
    if (Ctx.Assessment == nullptr) return 0;
    if (!Ctx.Assessment->bHasConstructionYard) return 0;
    if (Ctx.KnownEnemies != nullptr && !Ctx.KnownEnemies->empty()) return 0;
    return 500;
}

std::vector<DirectorRec> ScoutingDirector::Evaluate(const DirectorContext& Ctx) const
{
    std::vector<DirectorRec> Recs;

    const int32_t StaleScore = EvaluateStaleIntel(Ctx);
    if (StaleScore > 0)
    {
        Recs.push_back({DirectorRecommendation::ScoutArea, StaleScore, {}, "stale intelligence"});
    }

    const int32_t UnknownScore = EvaluateUnknownSectors(Ctx);
    if (UnknownScore > 0)
    {
        Recs.push_back({DirectorRecommendation::ScoutEnemyBase, UnknownScore, {}, "no enemy contact"});
    }

    return Recs;
}

// ---------------------------------------------------------------------------
// DefenseDirector
// ---------------------------------------------------------------------------

int32_t DefenseDirector::EvaluateBaseThreat(const DirectorContext& Ctx) const
{
    if (Ctx.Assessment == nullptr) return 0;
    if (!Ctx.Assessment->bUnderAttack) return 0;
    return 500;
}

int32_t DefenseDirector::EvaluateDefenceNeed(const DirectorContext& Ctx) const
{
    if (Ctx.Assessment == nullptr) return 0;
    const int32_t Deficit = Ctx.TargetDefences - Ctx.Assessment->Defences;
    if (Deficit <= 0) return 0;
    return 100 + Deficit * 150;
}

int32_t DefenseDirector::EvaluateRecallNeed(const DirectorContext& Ctx) const
{
    if (Ctx.Assessment == nullptr) return 0;
    if (Ctx.Assessment->bUnderAttack && Ctx.Assessment->Defences == 0)
    {
        return 400;
    }
    return 0;
}

std::vector<DirectorRec> DefenseDirector::Evaluate(const DirectorContext& Ctx) const
{
    std::vector<DirectorRec> Recs;

    const int32_t ThreatScore = EvaluateBaseThreat(Ctx);
    const int32_t DefenceScore = EvaluateDefenceNeed(Ctx);
    if (DefenceScore > 0 || ThreatScore > 0)
    {
        Recs.push_back({DirectorRecommendation::BuildDefence,
                        DefenceScore + ThreatScore, {}, "defence deficit or under attack"});
    }

    const int32_t RecallScore = EvaluateRecallNeed(Ctx);
    if (RecallScore > 0)
    {
        Recs.push_back({DirectorRecommendation::RecallArmy, RecallScore, {}, "base undefended under attack"});
    }

    return Recs;
}

// ---------------------------------------------------------------------------
// OffenseDirector
// ---------------------------------------------------------------------------

int32_t OffenseDirector::EvaluateTargetValue(const DirectorContext& Ctx) const
{
    if (Ctx.Assessment == nullptr) return 0;
    if (!Ctx.Assessment->bHasEnemyTarget) return 0;
    if (Ctx.Values != nullptr && Ctx.Values->FindHighestValueTarget().IsValid())
    {
        return 600;
    }
    return 200;
}

int32_t OffenseDirector::EvaluateHarassOpportunity(const DirectorContext& Ctx) const
{
    if (Ctx.Assessment == nullptr) return 0;
    if (Ctx.Assessment->ArmedUnits < Ctx.MinimumAttackSize) return 0;
    if (Ctx.Assessment->bHasEnemyTarget && Ctx.Assessment->Harvesters > 0)
    {
        return 300;
    }
    return 0;
}

int32_t OffenseDirector::EvaluateArmyReadiness(const DirectorContext& Ctx) const
{
    if (Ctx.Assessment == nullptr) return 0;
    if (Ctx.Assessment->ArmedUnits < Ctx.AttackArmySize) return 0;
    if (!Ctx.Assessment->bHasEnemyTarget) return 0;
    return 700;
}

std::vector<DirectorRec> OffenseDirector::Evaluate(const DirectorContext& Ctx) const
{
    std::vector<DirectorRec> Recs;

    const int32_t AttackScore = EvaluateArmyReadiness(Ctx);
    if (AttackScore > 0)
    {
        Recs.push_back({DirectorRecommendation::AttackTarget, AttackScore, {}, "army ready, target acquired"});
    }

    const int32_t HarassScore = EvaluateHarassOpportunity(Ctx);
    if (HarassScore > 0)
    {
        Recs.push_back({DirectorRecommendation::HarassEconomy, HarassScore, {}, "harass opportunity"});
    }

    const int32_t TargetScore = EvaluateTargetValue(Ctx);
    if (TargetScore > 0 && AttackScore == 0)
    {
        Recs.push_back({DirectorRecommendation::RegroupArmy, TargetScore, {}, "target visible but army not ready"});
    }

    return Recs;
}

} // namespace AI
} // namespace RA4
