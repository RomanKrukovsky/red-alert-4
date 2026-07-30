# Headless C++ Test Matrix & Verification

## Overview
All AI Commander, Army Group, Formation, and Player Control functionality is validated via the engine-free headless C++ test suite (`RA4Tests`).

---

## Test Execution Results
- **Command**: `cmake -S Tools/HeadlessBuild -B build/hb && cmake --build build/hb -j8 && ./build/hb/RA4Tests`
- **Pass Rate**: **224 passed / 0 failed (100% pass rate)**
- **Total Execution Time**: 2287 ms

---

## Suite Highlights

| Test Category | Key Tests | Status |
| :--- | :--- | :---: |
| **Fog of War & Scouting** | `AIKnowledge.EnemiesOutsideVisionAreNotObserved`<br>`AIKnowledge.MemoryIndexesTheFogGridInTilesNotWorldUnits`<br>`AI.DispatchesScoutWhenNoTargetsAreKnown` | **PASS** |
| **Entity Taxonomy** | `AI.EntityRoleDerivationWorksForUnitsAndBuildings` | **PASS** |
| **Operational & Tactical** | `AI.ArmyGroupManagerLifecycle`<br>`AI.TacticalOperationLifecycleStateTransitions`<br>`AI.SquadsGatherBeforeAdvancing`<br>`AI.WoundedUnitRetreatsToBase` | **PASS** |
| **Faction Doctrines** | `AI.FactionDoctrinesSovietAndAlliance` | **PASS** |
| **Explainability Overlay** | `AI.AIDebugOverlaySnapshotCreation` | **PASS** |
| **Player Army Control** | `Selection.SelectIdleUnitsSelectsOnlyIdleUnits`<br>`Selection.SelectWoundedUnitsSelectsDamagedUnits` | **PASS** |
| **Full Match Scenarios** | `AI.TwoCommandersPlayAMatchToCompletion`<br>`AI.FiveSkirmishScenariosFinishWithAWinner`<br>`AI.IsDeterministic` | **PASS** |
| **Stress & Performance** | `ProvingGround.HeadlessStressScenario500Entities` (500 entities @ 20Hz)<br>`ProvingGround.ForcedDesyncDetection` | **PASS** |
