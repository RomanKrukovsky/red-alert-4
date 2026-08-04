// Copyright (c) Red Alert 4 project. Fast Forward Combat Simulator.
#pragma once

#include <cstdint>
#include <vector>
#include "RA4Core/Ids.h"
#include "RA4Core/Vector.h"
#include "RA4Simulation/SimWorld.h"

#ifndef RA4AI_API
#define RA4AI_API
#endif

namespace RA4
{
namespace AI
{

struct CombatPredictionInput
{
    std::vector<uint32_t> AttackerEntityIndices;
    std::vector<uint32_t> DefenderEntityIndices;
    bool bDefenderHasGarrison = false;
};

struct CombatPredictionResult
{
    float WinProbability = 0.0f;        // 0.0 to 1.0
    float AttackerRetainedValuePct = 0.0f; // 0.0 to 1.0
    float ExpectedDefenderLossPct = 0.0f;  // 0.0 to 1.0
    uint32_t EstimatedFightDurationSeconds = 0;
    bool bShouldEngage = false;
};

class RA4AI_API ForwardCombatSim
{
public:
    static CombatPredictionResult PredictOutcome(const SimWorld& World, const CombatPredictionInput& Input);
};

} // namespace AI
} // namespace RA4
