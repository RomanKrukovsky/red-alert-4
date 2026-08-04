# Artificial Intelligence System Audit (`AI_AUDIT.md`)

**Audit Date**: August 4, 2026  
**Module**: `RA4AI` (`Source/RA4AI/`)  
**Test Suite**: `RA4AITests` (46 passed, 0 failed)  

---

## 1. Executive Summary

The AI system in RA4 is a deterministic, utility-based AI commander (`AICommander`) designed for Skirmish and Campaign bot opponents.
It operates purely by issuing standard simulation commands to `CommandBus`, adhering strictly to the same rules, vision limits, and resource constraints as human players.

---

## 2. Strategic Utility Decision Loop

### Utility Strategies (`StrategyType`)
1. **Economy Strategy**: Prioritizes building Refineries, Harvesters, and expanding base resource collection.
2. **Tech Strategy**: Prioritizes constructing Radar, Tech Labs, and unlocking higher tier unit blueprints.
3. **AssembleArmy Strategy**: Recruits combat units, groups them into tactical army formations, and prepares offensive operations.
4. **Assault Strategy**: Launches targeted army strikes against identified enemy base structures or harvesters.
5. **Fortify Strategy**: Triggered upon taking recent damage; builds defense turrets and mobilizes combat units to base perimeter.
6. **Recovery Strategy**: Emergency protocol activated when economy is severely depleted; spends credit reserves to rebuild harvesters.

### Hysteresis & Emergency Overrides
- **Strategy Hysteresis**: Prevents rapid strategy toggling when utility scores are close (e.g. `AI.StrategyHysteresisKeepsCurrentChoiceNearATie`).
- **Emergency Overrides**: Sudden damage or base assault bypasses hysteresis to immediately trigger `Fortify` mode.

---

## 3. Tactical Operation Lifecycle & Army Groups

- **Tactical Operation Lifecycle**: `Staging` -> `Advancing` -> `Engaging` -> `Retreating` -> `Completed`.
- **Squad Formations**: Units are assigned to squad groups based on movement speed and role (Tanks lead front line, Artillery stays back).
- **Wounded Unit Retreat**: Units dropping below 25% health dynamically issue retreat orders back to friendly repair facilities (`AI.WoundedUnitRetreatsToBase`).

---

## 4. Difficulty Profiles & Fair-Play Audit

| Parameter | Easy | Medium | Hard |
| :--- | :--- | :--- | :--- |
| **Decision Interval (Ticks)** | 60 ticks (~2 sec) | 30 ticks (~1 sec) | 10 ticks (~0.3 sec) |
| **Micro Reactivity** | Low (Basic Attack) | Medium (Focus Fire) | High (Focus Fire + Wounded Retreat) |
| **Resource Multiplier** | 1.0x (Standard) | 1.0x (Standard) | 1.0x (Standard) |
| **Vision Rules** | Strict Fog-of-War | Strict Fog-of-War | Strict Fog-of-War |

### Zero-Cheat Compliance Audit (`AI.NoCheatResources`, `AI.FogOfWarStrictCompliance`)
- **Empirical Proof**:
  - `AI.NoCheatResources` (PASS): Verified that AI starting credits and income match human rules exactly; no passive credit injection.
  - `AIKnowledge.EnemiesOutsideVisionAreNotObserved` (PASS): Verified that AI spatial memory grid ignores enemy entities concealed by Fog-of-War.
  - `AI.HardDifficultyIssuesMoreDecisionsThanEasy` (PASS): Hard AI issues 483 decisions per 120s vs 123 for Easy AI.

---

## 5. Skirmish AI Test Execution Suite (`RA4AITests`)

All 46 unit tests in `RA4AITests` execute cleanly in 4.826s:
- `AI.FiveSkirmishScenariosFinishWithAWinner` (PASS - 5 diverse match setups finish deterministically).
- `AI.IsDeterministic` (PASS - Repeated runs produce bit-identical command sequences and final state hashes).
- `AI.MassSimulationsBenchmark` (PASS - Benchmark completes 100 concurrent AI matches without thread stalls or memory leaks).
