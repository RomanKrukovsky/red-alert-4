// Copyright (c) Red Alert 4 project. HTN plan result.
//
// A plan is a deterministic, ordered list of primitive actions. The planner
// also stores the worldstate snapshot it planned against so the commander can
// cheaply detect "world has changed enough to re-plan" without recomputing the
// full plan.
#pragma once

#include <cstdint>
#include <vector>

#include "RA4AI/HTNTypes.h"
#include "RA4AI/HTNWorldState.h"
#include "RA4Core/SimConfig.h"

#ifndef RA4AI_API
#define RA4AI_API
#endif

namespace RA4
{
namespace AI
{

struct RA4AI_API HTNPlan
{
    std::vector<HTNAction> Actions;
    std::vector<const char*> ReasonPerStep;       // task name per step, for trace
    HTNWorldState PlannedAgainst;
    uint32_t ComputedAtTick = 0;
    int32_t TotalPlanCost = 0;
    int32_t BiasLoopsAtPlanTime = 0;

    bool IsValid() const { return !Actions.empty(); }
    size_t Length() const { return Actions.size(); }
    void Clear() { Actions.clear(); ReasonPerStep.clear(); TotalPlanCost = 0; }
};

// Status the planner can return. Anything other than Success means the caller
// must fall back to its existing utility logic; it never falls back silently.
enum class HTNPlanStatus : uint8_t
{
    Success = 0,
    FailedNoPlan,
    FailedDepthExceeded,
    FailedNodeBudgetExceeded,
    FailedEmptyDomain
};

const char* RA4AI_API ToString(HTNPlanStatus Status);

} // namespace AI
} // namespace RA4