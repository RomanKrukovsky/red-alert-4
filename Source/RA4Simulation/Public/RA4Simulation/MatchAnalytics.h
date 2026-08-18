#pragma once

#include "CoreMinimal.h"
#include "SimTypes.h"
#include "EconomyMatchSummary.h" // ADR-0020

/**
 * ADR-0028 Post-Match Analytics
 * Computed from the unified event log at match end (or offline).
 * Used by MatchViewer, balance regression, and dashboards.
 */

namespace RA4
{

struct FMatchAnalytics
{
    // Losses & value
    int32_t TotalUnitsLost[4] = {};
    int32_t TotalValueLost[4] = {};
    int32_t DamageDealt[4] = {};
    int32_t DamageTaken[4] = {};
    float   DamageEfficiency[4] = {};   // dealt / taken

    // Build & tech timing
    TickIndex FirstBuildTick[4][256] = {}; // indexed by UnitTypeId (content bible)
    TickIndex Tech2Tick[4] = {};
    TickIndex Tech3Tick[4] = {};

    // Economy (extended from ADR-0020)
    EconomyMatchSummary Economy[4];

    // Map control timeline (sampled every 300 ticks)
    struct FMapControlSample { TickIndex Tick; float ControlPercent[4]; };
    TArray<FMapControlSample> MapTimeline;

    // Army value timeline (sampled every 60 ticks)
    struct FArmyValueSample { TickIndex Tick; int32_t Value[4]; };
    TArray<FArmyValueSample> ArmyTimeline;

    // Path & stuck metrics
    int32_t TotalPathFailures[4] = {};
    int32_t TotalStuckEvents[4] = {};
    float   AveragePathRetryCount = 0.0f;

    // Perf
    float AverageTickDurationUs = 0.0f;
    uint32_t MaxTickDurationUs = 0;
    uint32_t DesyncCount = 0;
};

} // namespace RA4