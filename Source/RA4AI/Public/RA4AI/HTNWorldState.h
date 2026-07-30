// Copyright (c) Red Alert 4 project. HTN worldstate: flat array + snapshot helper.
//
// Every planner branch clones the worldstate, so it has to be a fixed-size POD
// with cheap copies. Building it from AIWorldAssessment keeps the planner
// ignorant of SimWorld and keeps the snapshot deterministic: the same assessment
// always yields the same worldstate, and the same worldstate always yields the
// same plan under the same search bounds.
#pragma once

#include <cstdint>

#include "RA4AI/AIStrategy.h"
#include "RA4AI/HTNTypes.h"

#ifndef RA4AI_API
#define RA4AI_API
#endif

namespace RA4
{
namespace AI
{

// Fixed-size property bag indexed by WSKey. Sized to the number of keys plus a
// small slack_margin rather than kHTNWorldStateCapacity so adding a new key
// surfaces as a compile error here rather than as a silent buffer overflow.
struct RA4AI_API HTNWorldState
{
    int32_t Values[static_cast<size_t>(WSKey::Count)] = {};

    int32_t Get(WSKey Key) const { return Values[static_cast<size_t>(Key)]; }
    void Set(WSKey Key, int32_t Value) { Values[static_cast<size_t>(Key)] = Value; }
    void Add(WSKey Key, int32_t Delta) { Values[static_cast<size_t>(Key)] += Delta; }

    bool EvaluateCondition(WSKey Key, WSCompare Op, int32_t Operand) const;
    void ApplyEffect(WSKey Key, int32_t NewValue);
    void ApplyDelta(WSKey Key, int32_t Delta);

    bool operator==(const HTNWorldState& Other) const;
    bool operator!=(const HTNWorldState& Other) const { return !(*this == Other); }
};

// Build a worldstate snapshot from the same assessment the utility AI uses.
// Adding a new property is a one-line change here and a one-line change to WSKey.
RA4AI_API HTNWorldState MakeWorldState(const AIWorldAssessment& Assessment,
                                       const AIConfig& Config,
                                       AIStrategy ActiveStrategy,
                                       int32_t BiasLoops);

} // namespace AI
} // namespace RA4