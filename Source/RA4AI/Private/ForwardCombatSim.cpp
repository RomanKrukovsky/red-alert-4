// Copyright (c) Red Alert 4 project. Fast Forward Combat Simulator.
#include "RA4AI/ForwardCombatSim.h"
#include <algorithm>

namespace RA4
{
namespace AI
{

CombatPredictionResult ForwardCombatSim::PredictOutcome(const SimWorld& World, const CombatPredictionInput& Input)
{
    CombatPredictionResult Result;
    size_t Attackers = Input.AttackerEntityIndices.size();
    size_t Defenders = Input.DefenderEntityIndices.size();

    if (Attackers == 0)
    {
        Result.WinProbability = 0.0f;
        Result.AttackerRetainedValuePct = 0.0f;
        Result.ExpectedDefenderLossPct = 0.0f;
        Result.bShouldEngage = false;
        return Result;
    }

    if (Defenders == 0)
    {
        Result.WinProbability = 1.0f;
        Result.AttackerRetainedValuePct = 0.95f;
        Result.ExpectedDefenderLossPct = 1.0f;
        Result.EstimatedFightDurationSeconds = 10;
        Result.bShouldEngage = true;
        return Result;
    }

    float AttackerPower = static_cast<float>(Attackers) * 100.0f;
    float DefenderPower = static_cast<float>(Defenders) * 110.0f * (Input.bDefenderHasGarrison ? 1.4f : 1.0f);

    float PowerRatio = AttackerPower / std::max(1.0f, DefenderPower);
    Result.WinProbability = std::min(1.0f, std::max(0.0f, 0.5f + (PowerRatio - 1.0f) * 0.4f));
    Result.AttackerRetainedValuePct = std::min(1.0f, std::max(0.0f, Result.WinProbability * 0.8f));
    Result.ExpectedDefenderLossPct = std::min(1.0f, std::max(0.0f, 1.0f - Result.WinProbability * 0.5f));
    Result.EstimatedFightDurationSeconds = 15;
    Result.bShouldEngage = (Result.WinProbability >= 0.55f);

    return Result;
}

} // namespace AI
} // namespace RA4
