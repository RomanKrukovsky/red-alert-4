// Copyright (c) Red Alert 4 project. The 6 Specialized Strategic Directors.
#pragma once

#include <cstdint>
#include <vector>
#include "RA4AI/AIBeliefGrid.h"
#include "RA4AI/ForwardCombatSim.h"
#include "RA4Core/Command.h"
#include "RA4Core/Ids.h"
#include "RA4Simulation/SimWorld.h"

#ifndef RA4AI_API
#define RA4AI_API
#endif

namespace RA4
{
namespace AI
{

struct CashflowForecast
{
    int32_t ProjectedIncome90s = 0;
    int32_t MandatoryExpenses90s = 0;
    int32_t ReserveFund = 0;
    int32_t AvailableForArmy = 0;
};

class RA4AI_API EconomyDirector
{
public:
    CashflowForecast ForecastCashflow(const SimWorld& World, PlayerId SelfPlayer) const;
    int32_t CalculateTargetHarvesterCount(const SimWorld& World, PlayerId SelfPlayer) const;
    bool ShouldExpandBase(const SimWorld& World, PlayerId SelfPlayer) const;
};

class RA4AI_API ProductionDirector
{
public:
    bool EvaluateBuildOrder(const SimWorld& World, PlayerId SelfPlayer, std::vector<Command>& OutCommands);
};

class RA4AI_API IntelDirector
{
public:
    void DirectScouting(const SimWorld& World, PlayerId SelfPlayer, const AIBeliefGrid& BeliefGrid, std::vector<Command>& OutCommands);
};

class RA4AI_API DefenseDirector
{
public:
    float CalculateThreatLevel(const SimWorld& World, PlayerId SelfPlayer, const AIBeliefGrid& BeliefGrid) const;
    bool NeedsDefensiveGarrison(const SimWorld& World, PlayerId SelfPlayer) const;
};

class RA4AI_API OffenseDirector
{
public:
    bool PlanStrike(const SimWorld& World, PlayerId SelfPlayer, const AIBeliefGrid& BeliefGrid, std::vector<Command>& OutCommands);
};

class RA4AI_API AbilityDirector
{
public:
    void EvaluateSuperweapons(const SimWorld& World, PlayerId SelfPlayer, std::vector<Command>& OutCommands);
};

} // namespace AI
} // namespace RA4
