# Agent Handoff: Skirmish Gameplay Engine (`agents/skirmish-gameplay`)

## 1. Overview & Responsibility
This branch (`agents/skirmish-gameplay` located at `<home>/Documents/red-alert-4-gameplay`) owns all deterministic Skirmish gameplay logic in C++ (`RA4Simulation`, `RA4Content`, `RA4Core`, `RA4Combat`, `RA4Replay`, `RA4Campaign`).

It enforces strict fixed-point lockstep determinism, single-frame command execution (`CommandBus`), resource economy, harvester docking queues, tech prerequisites, building construction states, production queues, power management, win/loss conditions, and full replay/restart state reset.

---

## 2. Key Changes & Features Implemented

### A. Harvesters & Ore Field Economy
- **Single-Harvester Docking & Queueing**: Added `EntityId DockedHarvester` and `std::vector<EntityId> UnloadingQueue` to `BuildingComp`. Multiple harvesters arriving at a refinery queue up cleanly without clipping or overlapping inside the dock zone.
- **Refinery Destruction Rerouting**: If a refinery is destroyed or invalidated while a harvester is in transit or unloading, `SystemHarvesters` automatically searches for the nearest valid completed refinery (`FindNearestRefinery`).
- **Resource Field Exhaustion**: Harvesters automatically switch to `MovingToRefinery` if carrying cargo or `Idle` if cargo is empty when an ore node depletes (`Amount <= 0`).
- **Stuck / Blocked Unit Recovery**: Harvesters blocked for $> 60$ ticks ($3$ seconds) clear path state and re-evaluate nearest valid resource node or refinery.

### B. Data-Driven Tech Prerequisites
- **Data-Driven Rules (`PrerequisiteGroup`)**: Replaced hardcoded checks with `PrerequisiteGroup` containing `AllOf`, `AnyOf`, and `NoneOf` clauses in `ContentTypes.h`.
- **`SimWorld::HasPrerequisites`**: Evaluates player's completed building types against `AllOf`, `AnyOf`, and `NoneOf` clauses. Fully hash-validated in `ContentDatabase.cpp`.

### C. Construction System & Under-Construction State
- **Placement & Build Radius**: Enforces clear terrain, build radius proximity, tile occupancy (`Map.IsTileOccupied`), and credit affordability.
- **Targetable Under-Construction State**: Structures in `ConstructionState::UnderConstruction` are registered in entity lists, can be targeted and attacked mid-construction, and increase health proportional to build progress.
- **Cancellation & Refunds**: Selling/cancelling an under-construction building refunds credits based on `CancelRefundPercent`, clears tile occupancy, and emits `SimEventType::EntityDestroyed`.

### D. Production System & CommandBus
- **Low Power Pause**: Scaling and pausing production progress when power ratio is in deficit.
- **Door Clearance & Exit Blockage**: Units spawn via `FindFreeSpawnPoint`.
- **Single Execution CommandBus**: Frame dispatch applies queued commands exactly once per tick.

### E. Match Flow, Win/Loss & Restart
- **Victory & Defeat Triggering**: Defeat is declared when a player loses all buildings and units. Match ends when only 1 active player remains.
- **State Reset (`SimWorld::Restart()`)**: Restores simulation to tick 0, clears all entity vectors, resets occupancy grid, resets player state, clears events, and re-initializes navigation.

---

## 3. Modified Files

- `Source/RA4Content/Public/RA4Content/ContentTypes.h`: Added `PrerequisiteGroup` struct and `PrerequisitesGroup` field to `ProductionInfo`.
- `Source/RA4Content/Private/ContentDatabase.cpp`: Added `PrerequisitesGroup` fields to database hashing and validation.
- `Source/RA4Simulation/Public/RA4Simulation/SimTypes.h`: Added `DockedHarvester` and `UnloadingQueue` to `BuildingComp`.
- `Source/RA4Simulation/Public/RA4Simulation/SimWorld.h`: Added `Restart()` declaration and `MatchSetup SetupConfig` field.
- `Source/RA4Simulation/Private/SimWorld.cpp`:
  - Updated `HasPrerequisites` for `AllOf`/`AnyOf`/`NoneOf`.
  - Updated `SystemHarvesters` for refinery dock queueing & recovery.
  - Implemented `SimWorld::Restart()`.
  - Updated `DestroyEntity` to clean up refinery dock queues.
- `Source/RA4Tests/Private/TestSimulation.cpp`: Added 4 new automated unit tests.

---

## 4. Public API & Interfaces for AI & UI Teams

### For AI Team (`agents/skirmish-ai`)
- `SimWorld::HasPrerequisites(PlayerId Owner, const EntityDef& Def)`: Returns whether tech prerequisites (`AllOf`, `AnyOf`, `NoneOf`) are satisfied.
- `SimWorld::IsPlacementValid(ContentId BuildingDef, PlayerId Owner, const TileCoord& OriginTile)`: Verifies placement validity.
- `HarvesterComp::State`: Idle, MovingToResource, Harvesting, MovingToRefinery, Unloading.

### For UI Team (`agents/skirmish-ui`)
- `SimWorld::Restart()`: Call to reset the simulation completely to tick 0 without reallocating memory or restarting process.
- `BuildingComp::DockedHarvester` & `BuildingComp::UnloadingQueue`: Can be read by HUD snapshot builder to show refinery dock status.
- `SimWorld::GetEvents()`: Event stream (`ResourceDelivered`, `BuildingCompleted`, `PlayerDefeated`, `MatchEnded`).

---

## 5. Verification Commands & Results

### Automated Test Suite
Build and execute headless test suite from `<home>/Documents/red-alert-4-gameplay`:
```bash
cmake -S Tools/HeadlessBuild -B build/hb && cmake --build build/hb -j8
./build/hb/RA4Tests
./build/hb/RA4InputTests
./build/hb/RA4PresentationTests
./build/hb/RA4AITests
```

**Results**:
- `RA4Tests`: 232 / 232 passed
- `RA4InputTests`: 51 / 51 passed
- `RA4PresentationTests`: 22 / 22 passed
- `RA4AITests`: 39 / 39 passed
- **Total**: 344 / 344 tests passed (0 failures).

### Determinism Verification
```bash
./build/hb/RA4MatchDump && cp match.json /tmp/dump1.json && ./build/hb/RA4MatchDump && cp match.json /tmp/dump2.json && diff -u /tmp/dump1.json /tmp/dump2.json
```
**Results**: Zero diff across 4,742 ticks (237 seconds simulated match, identical state checksums).
