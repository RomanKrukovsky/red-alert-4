# ADR-0020: Economic Telemetry and Balance Metrics

## Context

The economy design specifies extensive metrics for balance validation: income per minute, harvester utilization, expansion payback time, the moment a match becomes economically unrecoverable, and more. Without automated telemetry, balance tuning is guesswork. This ADR defines the telemetry system that records economic state every tick and produces post-match analysis reports for the headless simulator.

## Decision

### Telemetry Schema

Every tick, for each player, record:

```
struct EconomyTickRecord
{
    TickIndex   Tick;

    // Income
    int32_t     CreditsPerTick;           // gross income this tick
    int32_t     CreditsSpentTick;         // credits spent this tick (production + construction)
    int32_t     CreditsBalance;           // unspent balance

    // Harvester fleet
    int32_t     HarvesterCount;           // total harvesters alive
    int32_t     HarvestersHarvesting;     // in Harvesting state
    int32_t     HarvestersMoving;         // in MovingToResource or MovingToRefinery
    int32_t     HarvestersUnloading;      // in Unloading state
    int32_t     HarvestersIdle;           // in Idle state
    int32_t     HarvestersRetreating;     // in ThreatDetected or Retreating
    int32_t     HarvestersBlocked;        // in Blocked or WaitingForDock
    int32_t     TotalCargoInTransit;      // sum of Cargo across all harvesters

    // Resource nodes
    int32_t     ActiveNodeCount;          // nodes with Amount > 0
    int32_t     DepletedNodeCount;        // nodes with Amount == 0
    int32_t     TotalNodeAmount;          // sum of Amount across all nodes

    // Production
    int32_t     ActiveProductionItems;    // items in Funding or Paying state
    int32_t     StarvedItems;             // items in Starved state
    int32_t     EnergyThrottledItems;     // items in EnergyThrottled state
    int32_t     TotalProductionProgress;  // sum of ProgressTicks across all items

    // Energy
    int32_t     PowerProduced;
    int32_t     PowerConsumed;
    int32_t     PowerRatioPercent;

    // Command limit
    int32_t     CommandLimitUsed;
    int32_t     CommandLimitMax;
    int32_t     CommandLimitPenalty;      // current penalty percentage

    // Expansion
    int32_t     BaseCount;               // number of completed refineries
    int32_t     ExpansionInvestment;     // total credits spent on expansions
    int32_t     ExpansionIncome;         // total credits earned from expansions

    // Faction-specific
    int32_t     FactionResource;         // faction resource value (Mobilization, etc.)
    int32_t     TemporalDebtCount;       // active temporal debts (Chronolegion only)
    int32_t     TemporalDebtPrincipal;   // total remaining debt principal
    int32_t     TemporalPenaltyBPS;      // current income penalty from debt

    // Army
    int32_t     ArmyValue;               // sum of unit costs
    int32_t     ArmyCount;               // number of combat units

    // Tech
    int32_t     CurrentTechLevel;        // 1, 2, or 3
    TickIndex   Tech2Tick;               // tick when T2 was reached (0 if not yet)
    TickIndex   Tech3Tick;               // tick when T3 was reached (0 if not yet)
};
```

### Recording Frequency

- **Full record**: every 60 ticks (1 second). This is sufficient for balance analysis without excessive storage.
- **Summary record**: at match end, a single aggregate per player.

### Post-Match Summary

At match end, compute:

```
struct EconomyMatchSummary
{
    // Duration
    TickIndex   MatchDurationTicks;

    // Income
    int32_t     TotalIncome;             // total credits earned
    int32_t     TotalSpent;              // total credits spent
    int32_t     PeakBalance;             // highest unspent balance
    int32_t     AverageBalance;          // average unspent balance over match
    int32_t     IdleTimeTicks;           // ticks with 0 income (starvation)

    // Harvester efficiency
    int32_t     HarvesterUtilization;    // percentage of time harvesters were harvesting
    int32_t     AverageHarvestCycleTime; // average ticks for full load/unload cycle
    int32_t     HarvesterLossCount;      // harvesters lost
    int32_t     HarvesterLossValue;      // total credits lost from harvester deaths

    // Expansion
    int32_t     FirstExpansionTick;      // tick of first refinery completion (0 if never)
    int32_t     ExpansionPaybackTicks;   // actual payback time for first expansion
    int32_t     ExpansionCount;          // total refineries built
    int32_t     ExpansionLossCount;      // expansions destroyed

    // Production
    int32_t     TotalUnitsBuilt;
    int32_t     TotalBuildingsBuilt;
    int32_t     TotalUnitsLost;
    int32_t     TotalBuildingsLost;
    int32_t     QueueStarvationTicks;    // ticks with any Starved items

    // Power
    int32_t     PowerDeficitTicks;       // ticks with PowerRatio < 100%
    int32_t     PowerSevereTicks;        // ticks with PowerRatio < 40%

    // Command limit
    int32_t     OverCommandLimitTicks;   // ticks with CommandLimitUsed > CommandLimitMax
    int32_t     PeakCommandOverflow;     // max overflow percentage

    // Tech timing
    TickIndex   Tech2Timing;             // tick when T2 reached
    TickIndex   Tech3Timing;             // tick when T3 reached

    // Army
    int32_t     PeakArmyValue;           // highest army value at any point
    int32_t     FinalArmyValue;          // army value at match end

    // Faction-specific
    int32_t     FactionResourcePeak;     // peak faction resource value
    int32_t     TemporalDebtTotalBorrowed; // total credits borrowed (Chronolegion)
    int32_t     TemporalDebtCancellationCount; // debts cancelled early

    // Economic recovery
    int32_t     RecoveryTimeTicks;       // time from major loss to income recovery
    TickIndex   EconomicallyUnrecoverableTick; // tick when match became unrecoverable (0 if never)
};
```

### Economically Unrecoverable Detection

A match is "economically unrecoverable" when:

```
Condition = (ArmyValue < 20% of opponent's ArmyValue)
         AND (Income < 30% of opponent's Income)
         AND (BaseCount == 0 OR AllBasesUnderAttack)
         AND (Credits < 500)
         AND (This state persists for > 300 ticks / 5 seconds)
```

This is a heuristic, not a guarantee. The match is not automatically ended; the losing player can still fight. But the telemetry marks this tick for post-match analysis.

### Storage Format

Telemetry is written to a binary file:

```
File: {MatchId}_economy_telemetry.bin
Header: Magic (4 bytes), Version (4 bytes), PlayerCount (4 bytes)
Per-player section:
    PlayerId (4 bytes)
    RecordCount (4 bytes)
    Records[RecordCount]: EconomyTickRecord (fixed-size binary)
    Summary: EconomyMatchSummary (fixed-size binary)
```

### Determinism Verification

Telemetry records are **outputs**, not state. They do not affect simulation. However, for determinism verification:

1. Two runs with identical seeds and commands must produce identical telemetry records.
2. The CI pipeline can compare telemetry files between platforms.
3. Any difference in `CreditsBalance` or `PowerRatioPercent` at the same tick indicates a desync.

### Headless Simulator Integration

The headless economy simulator (Phase B) reads telemetry to:

1. **Parameter sweeps**: vary harvester speed, cargo capacity, node amounts, and measure impact on match duration, income curves, and win rates.
2. **Scenario testing**: run rush-vs-expansion, harvester raids, power shortages, and measure recovery times.
3. **Faction matchups**: run all 16 faction pairs and compare economic metrics.
4. **Balance reports**: generate tables of income curves, army values, and tech timing across scenarios.

### Performance Budget

- Telemetry recording: < 0.1 ms/tick (trivial: write to pre-allocated buffer).
- File I/O: buffered, flushed at match end. No per-tick disk writes.
- Memory: ~100 bytes per record × 3600 records/minute × 30 minutes = ~10 MB per match per player. Acceptable.

### UI Representation

- **In-match**: economic overlay (optional) showing income/spending graphs.
- **Post-match**: detailed economy tab in match summary screen with charts for income, army value, tech timing, and harvester efficiency.
- **Replay**: economy overlay available during replay viewing.

### AI Evaluation

AI does not read telemetry during matches (it's a post-match analysis tool). However, AI can query live economic state (same fields as telemetry) for decision-making.

## Rationale

- Comprehensive telemetry enables data-driven balance tuning.
- Binary format is compact and fast to write/read.
- 1-second recording frequency is sufficient for balance analysis.
- Economically unrerecoverable detection helps identify balance issues.
- Determinism verification via telemetry comparison catches subtle bugs.

## Status

**ACCEPTED**. Pending implementation. Telemetry system will be built as part of the headless economy simulator.
