// Copyright (c) Red Alert 4 project. Specialised AI Director subsystems.
//
// Each Director evaluates one domain (economy, scouting, defence, offence) and
// produces scored recommendations.  Directors are pure logic — no engine
// dependency, fully deterministic, testable in isolation.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "RA4AI/AIStrategy.h"
#include "RA4AI/AIWorldView.h"
#include "RA4AI/ThreatMap.h"
#include "RA4AI/ValueMap.h"
#include "RA4Core/Vector.h"

#ifndef RA4AI_API
#define RA4AI_API
#endif

namespace RA4
{
namespace AI
{

enum class DirectorRecommendation : uint8_t
{
    None = 0,
    BuildHarvester,
    BuildRefinery,
    BuildPowerPlant,
    ExpandBase,
    ScoutArea,
    ScoutEnemyBase,
    BuildDefence,
    RecallArmy,
    FortifyPosition,
    AttackTarget,
    HarassEconomy,
    RegroupArmy,
    Count,
};

const char* RA4AI_API ToString(DirectorRecommendation Rec);

struct DirectorRec
{
    DirectorRecommendation Type = DirectorRecommendation::None;
    int32_t Score = 0;
    TileCoord TargetTile;
    std::string Reason;
};

struct DirectorContext
{
    const AIWorldAssessment* Assessment = nullptr;
    const ThreatMap* Threats = nullptr;
    const ValueMap* Values = nullptr;
    const std::vector<EnemyMemory>* KnownEnemies = nullptr;
    TickIndex CurrentTick = 0;
    TileCoord OwnBaseTile{-1, -1};
    int32_t TargetHarvesters = 3;
    int32_t CreditReserve = 300;
    int32_t TargetDefences = 2;
    int32_t AttackArmySize = 6;
    int32_t MinimumAttackSize = 3;
    int32_t MapWidth = 0;
    int32_t MapHeight = 0;
};

struct DirectorBundle
{
    std::vector<DirectorRec> EconomyRecs;
    std::vector<DirectorRec> ScoutingRecs;
    std::vector<DirectorRec> DefenseRecs;
    std::vector<DirectorRec> OffenseRecs;

    const DirectorRec* FindBest() const;
    const DirectorRec* FindBestOfType(DirectorRecommendation Type) const;
};

class RA4AI_API EconomyDirector
{
public:
    std::vector<DirectorRec> Evaluate(const DirectorContext& Ctx) const;
private:
    int32_t EvaluateHarvesterNeed(const DirectorContext& Ctx) const;
    int32_t EvaluateRefineryNeed(const DirectorContext& Ctx) const;
    int32_t EvaluatePowerNeed(const DirectorContext& Ctx) const;
    int32_t EvaluateExpansionNeed(const DirectorContext& Ctx) const;
};

class RA4AI_API ScoutingDirector
{
public:
    std::vector<DirectorRec> Evaluate(const DirectorContext& Ctx) const;
private:
    int32_t EvaluateStaleIntel(const DirectorContext& Ctx) const;
    int32_t EvaluateUnknownSectors(const DirectorContext& Ctx) const;
};

class RA4AI_API DefenseDirector
{
public:
    std::vector<DirectorRec> Evaluate(const DirectorContext& Ctx) const;
private:
    int32_t EvaluateBaseThreat(const DirectorContext& Ctx) const;
    int32_t EvaluateDefenceNeed(const DirectorContext& Ctx) const;
    int32_t EvaluateRecallNeed(const DirectorContext& Ctx) const;
};

class RA4AI_API OffenseDirector
{
public:
    std::vector<DirectorRec> Evaluate(const DirectorContext& Ctx) const;
private:
    int32_t EvaluateTargetValue(const DirectorContext& Ctx) const;
    int32_t EvaluateHarassOpportunity(const DirectorContext& Ctx) const;
    int32_t EvaluateArmyReadiness(const DirectorContext& Ctx) const;
};

} // namespace AI
} // namespace RA4
