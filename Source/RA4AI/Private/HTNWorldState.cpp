// Copyright (c) Red Alert 4 project.
#include "RA4AI/HTNWorldState.h"

#include <cstring>

namespace RA4
{
namespace AI
{

bool HTNWorldState::EvaluateCondition(WSKey Key, WSCompare Op, int32_t Operand) const
{
    const int32_t Value = Get(Key);
    switch (Op)
    {
        case WSCompare::Equal:        return Value == Operand;
        case WSCompare::NotEqual:     return Value != Operand;
        case WSCompare::Less:          return Value <  Operand;
        case WSCompare::LessEqual:     return Value <= Operand;
        case WSCompare::Greater:       return Value >  Operand;
        case WSCompare::GreaterEqual:  return Value >= Operand;
    }
    return false;
}

void HTNWorldState::ApplyEffect(WSKey Key, int32_t NewValue)
{
    Set(Key, NewValue);
}

void HTNWorldState::ApplyDelta(WSKey Key, int32_t Delta)
{
    Add(Key, Delta);
}

bool HTNWorldState::operator==(const HTNWorldState& Other) const
{
    return std::memcmp(Values, Other.Values, sizeof(Values)) == 0;
}

HTNWorldState MakeWorldState(const AIWorldAssessment& Assessment,
                             const AIConfig& Config,
                             AIStrategy ActiveStrategy,
                             int32_t BiasLoops)
{
    HTNWorldState WS;
    WS.Set(WSKey::Credits,                 Assessment.Credits);
    WS.Set(WSKey::PowerProduced,           Assessment.PowerProduced);
    WS.Set(WSKey::PowerConsumed,           Assessment.PowerConsumed);
    WS.Set(WSKey::PowerPlants,             Assessment.PowerPlants);
    WS.Set(WSKey::Refineries,              Assessment.Refineries);
    WS.Set(WSKey::Harvesters,              Assessment.Harvesters);
    WS.Set(WSKey::TargetHarvesters,        Config.TargetHarvesters);
    WS.Set(WSKey::ProductionBuildings,     Assessment.ProductionBuildings);
    WS.Set(WSKey::Defences,                Assessment.Defences);
    WS.Set(WSKey::TargetDefences,          Config.TargetDefences);
    WS.Set(WSKey::ArmedUnits,              Assessment.ArmedUnits);
    WS.Set(WSKey::AttackArmySize,           Config.AttackArmySize);
    WS.Set(WSKey::MinimumAttackSize,       Config.MinimumAttackSize);
    WS.Set(WSKey::HasConstructionYard,     Assessment.bHasConstructionYard ? 1 : 0);
    WS.Set(WSKey::HasEnemyTarget,           Assessment.bHasEnemyTarget ? 1 : 0);
    WS.Set(WSKey::UnderAttack,             Assessment.bUnderAttack ? 1 : 0);
    WS.Set(WSKey::CanProduceHarvester,      Assessment.bCanProduceHarvester ? 1 : 0);
    WS.Set(WSKey::AssaultActive,            Assessment.bAssaultActive ? 1 : 0);
    WS.Set(WSKey::PendingBuildingPlacement, 0);   // filled by AICommander when present
    WS.Set(WSKey::Strategy,                 static_cast<int32_t>(ActiveStrategy));
    WS.Set(WSKey::BiasLoops,                BiasLoops);
    return WS;
}

} // namespace AI
} // namespace RA4