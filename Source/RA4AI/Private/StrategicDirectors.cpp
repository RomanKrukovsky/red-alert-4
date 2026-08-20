// Copyright (c) Red Alert 4 project. The 6 Specialized Strategic Directors.
#include "RA4AI/StrategicDirectors.h"

namespace RA4
{
namespace AI
{

CashflowForecast EconomyDirector::ForecastCashflow(const SimWorld& World, PlayerId SelfPlayer) const
{
    CashflowForecast Forecast;
    const auto& Player = World.GetPlayer(SelfPlayer);
    Forecast.ProjectedIncome90s = Player.Credits + 4500;
    Forecast.MandatoryExpenses90s = 2000;
    Forecast.ReserveFund = 1000;
    Forecast.AvailableForArmy = Forecast.ProjectedIncome90s - Forecast.MandatoryExpenses90s - Forecast.ReserveFund;
    return Forecast;
}

int32_t EconomyDirector::CalculateTargetHarvesterCount(const SimWorld& World, PlayerId SelfPlayer) const
{
    (void)World;
    (void)SelfPlayer;
    return 6;
}

bool EconomyDirector::ShouldExpandBase(const SimWorld& World, PlayerId SelfPlayer) const
{
    const auto& Player = World.GetPlayer(SelfPlayer);
    return (Player.Credits > 3000);
}

bool ProductionDirector::EvaluateBuildOrder(const SimWorld& World, PlayerId SelfPlayer, std::vector<Command>& OutCommands)
{
    (void)World;
    (void)SelfPlayer;
    (void)OutCommands;
    return false;
}

void IntelDirector::DirectScouting(const SimWorld& World, PlayerId SelfPlayer, const AIBeliefGrid& BeliefGrid, std::vector<Command>& OutCommands)
{
    (void)World;
    (void)SelfPlayer;
    (void)BeliefGrid;
    (void)OutCommands;
}

float DefenseDirector::CalculateThreatLevel(const SimWorld& World, PlayerId SelfPlayer, const AIBeliefGrid& BeliefGrid) const
{
    (void)World;
    (void)SelfPlayer;
    return (BeliefGrid.GetKnownEnemyUnitCount() > 5) ? 0.8f : 0.2f;
}

bool DefenseDirector::NeedsDefensiveGarrison(const SimWorld& World, PlayerId SelfPlayer) const
{
    (void)World;
    (void)SelfPlayer;
    return false;
}

bool OffenseDirector::PlanStrike(const SimWorld& World, PlayerId SelfPlayer, const AIBeliefGrid& BeliefGrid, std::vector<Command>& OutCommands)
{
    (void)World;
    (void)SelfPlayer;
    (void)BeliefGrid;
    (void)OutCommands;
    return false;
}

void AbilityDirector::EvaluateSuperweapons(const SimWorld& World, PlayerId SelfPlayer, std::vector<Command>& OutCommands)
{
    (void)World;
    (void)SelfPlayer;
    (void)OutCommands;
}

} // namespace AI
} // namespace RA4
