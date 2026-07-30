// Copyright (c) Red Alert 4 project. HTN planner core types.
//
// The HTN layer is engine-free and deterministic: it operates on a snapshot
// worldstate built from AIWorldAssessment, plans a bounded sequence of
// primitive tasks, and emits the same Command objects a human player would.
// Nothing here touches the SimWorld directly; that is the AICommander's job.
#pragma once

#include <cstdint>

#ifndef RA4AI_API
#define RA4AI_API
#endif

namespace RA4
{
namespace AI
{

// Identifier for a task inside a domain. Indices are stable for the lifetime
// of the domain so plans stay valid across re-plans of the same snapshot.
using HTNTaskId = uint16_t;
using HTNMethodId = uint16_t;

constexpr HTNTaskId kInvalidHTNTask = 0xFFFFu;
constexpr HTNMethodId kInvalidHTNMethod = 0xFFFFu;

// Bounds on the planner search. These exist to keep decision-time bounded and
// deterministic: a runaway plan would stall the sim tick and a non-deterministic
// search would break replay. Both numbers are small because the Red Alert
// domain is shallow -- the longest useful plan is "build -> harvest -> tech ->
// army -> assault" which is five levels deep.
constexpr int32_t kHTNMaxDepth = 8;
constexpr int32_t kHTNMaxNodesVisited = 256;
constexpr int32_t kHTNMaxPlanLength = 16;

// Conservative upper bound on the number of properties carried in the worldstate.
// A flat array keeps worldstate copies cheap, which matters because the planner
// clones it on every decomposition step.
constexpr size_t kHTNWorldStateCapacity = 32;

// Worldstate property keys. Stored as int32_t values in a flat array. Boolean
// properties use 0/1 so the same comparison helpers work for every key without
// templating or branching on type.
enum class WSKey : uint8_t
{
    Credits = 0,
    PowerProduced,
    PowerConsumed,
    PowerPlants,
    Refineries,
    Harvesters,
    TargetHarvesters,
    ProductionBuildings,
    Defences,
    TargetDefences,
    ArmedUnits,
    AttackArmySize,
    MinimumAttackSize,
    HasConstructionYard,
    HasEnemyTarget,
    UnderAttack,
    CanProduceHarvester,
    AssaultActive,
    PendingBuildingPlacement,   // >0 if a finished structure is awaiting a tile
    Strategy,                    // current AIStrategy as int (for sticky decisions)
    BiasLoops,                   // re-plan counter; lets methods change preference over time

    Count
};

// Comparison operators used by method preconditions. The planner evaluates them
// against the worldstate snapshot; they are intentionally simple (no float
// comparisons, no compound predicates) so replays produce identical plans.
enum class WSCompare : uint8_t
{
    Equal = 0,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual
};

// One primitive action the HTN can emit. These map 1:1 to AICommander
// execution routines; the planner never touches Commands itself, it only tells
// the commander what to do, and the commander decides whether to do it (the
// sim may have already invalidated the plan).
enum class HTNAction : uint8_t
{
    None = 0,
    PlaceFinishedStructure,
    BuildPowerPlant,
    BuildRefinery,
    TrainHarvester,
    BuildBarracks,           // first infantry-tech producer
    BuildWarFactory,         // first vehicle-tech producer
    BuildDefence,
    TrainCombatUnit,
    AttackMoveArmy,
    RetreatWounded,
    NoOp,                    // plan terminator, emits nothing
};

const char* RA4AI_API ToString(HTNAction Action);
const char* RA4AI_API ToString(WSKey Key);

} // namespace AI
} // namespace RA4