# Performance & Benchmark Report

## Overview
This report documents computational performance metrics, decision rates, and memory footprint of the **AI Commander** and simulation suite measured on macOS arm64 hardware.

---

## Metric Benchmarks

### 1. Test Suite Execution Time
- **Total Test Suite Time**: **2287 ms** for 224 unit & scenario tests.
- **5 Skirmish Scenarios Execution**: **789 ms** for 5 full simulated matches (up to 18,000 ticks).
- **Match Simulation Speedup**: ~176 simulated seconds completed in **< 150 ms** real-time (over 1100x realtime speedup).

### 2. Proving Ground 500-Entity Stress Scenario
- **Entities**: 500 active units (250 Player 0 vs 250 Player 1) engaged in combat and pathfinding.
- **Duration**: 200 ticks (10 simulated seconds).
- **Execution Time**: **417 ms** total (~2.08 ms per tick for 500 active entities).
- **State Checksum Integrity**: Verified identical hash output across multi-run seeds (`ProvingGround.HeadlessStressScenario500Entities`).

### 3. Memory & Computational Budgets
- **AI Decision Cadence**: Evaluated every `Config.DecisionIntervalTicks` (default 10 ticks = 0.5 s at 20 Hz).
- **Server Command Rate Limit**: Capped at `kMaxCommandsPerPlayerPerTick = 64` to eliminate command spam.
- **Memory Footprint**: `ArmyGroupManager` overhead < 50 KB per match.
