# C++ Architecture & Simulation Audit (`ARCHITECTURE_AUDIT.md`)

**Audit Date**: August 4, 2026  
**Target Architecture**: Lockstep Deterministic C++ RTS Core  

---

## 1. Architectural Overview & Boundaries

The RA4 architecture strictly separates the **Deterministic Simulation Core** (`Source/RA4Simulation`, `Source/RA4Core`, `Source/RA4Content`) from the **Unreal Engine Presentation Layer** (`Source/RA4Presentation`, `Source/RedAlert4`, `Source/RA4UI`).

```
+-----------------------------------------------------------------------+
|                         UNREAL ENGINE 5 LAYER                         |
|   RA4Presentation | RedAlert4 | RA4UI | Slate / UMG / ViewModels      |
+-----------------------------------------------------------------------+
                                    | Reads Snapshots & Issues Commands
                                    v
+-----------------------------------------------------------------------+
|                    DETERMINISTIC SIMULATION CORE                      |
|   RA4Simulation | RA4Combat | RA4Navigation | RA4AI | RA4Network      |
|   RA4FogOfWar  | RA4Replay | RA4Campaign   | RA4Content | RA4Core     |
+-----------------------------------------------------------------------+
```

---

## 2. Module Dependency Analysis

### Pure C++ Engine Modules (Zero Engine/UObject Dependencies)
1. **`RA4Core`** (`Source/RA4Core`): Basic types, fixed-point math (`FixedPoint.h`), IDs (`EntityId`, `PlayerId`, `TickIndex`), Hash utilities (`Hash64.h`), and assertion macros.
2. **`RA4Content`** (`Source/RA4Content`): Database classes (`ContentDatabase`, `BibleContentLoader`) for unit stats, armor matrices, cost/build times, and voice line mappings loaded from `ra4_content.normalized.json`.
3. **`RA4Simulation`** (`Source/RA4Simulation`): Core deterministic state (`SimWorld`), entity definitions, harvester AI state machine, Command Bus (`CommandBus`), and Lockstep engine (`LockstepSession`).
4. **`RA4Combat`** (`Source/RA4Combat`): Combat mechanics, weapon range checks, armor matrix lookup, and damage calculation.
5. **`RA4Navigation`** (`Source/RA4Navigation`): Grid-based pathfinding, flowfields, unit collision avoidance, and spatial occupancy grid.
6. **`RA4FogOfWar`** (`Source/RA4FogOfWar`): Bit-grid vision calculation and fog-of-war reveal for simulation and AI.
7. **`RA4AI`** (`Source/RA4AI`): Utility AI commander (`AICommander`), army group management, strategic decision loops, and threat evaluation.
8. **`RA4Network`** (`Source/RA4Network`): Lockstep packet framing, LAN lobby handling, and client/server command distribution.
9. **`RA4Campaign`** (`Source/RA4Campaign`): Data-driven mission runner, objective state evaluator, and briefing cutscene triggers.
10. **`RA4Replay`** (`Source/RA4Replay`): Deterministic replay stream recorder and playback deserializer.

### Unreal Engine Bound Modules
1. **`RA4Presentation`** (`Source/RA4Presentation`): Maps simulation entity IDs to Unreal `AActor` / `USkeletalMeshComponent` visuals (`URA4PresentationSubsystem`, `URA4ArtMapping`).
2. **`RedAlert4`** (`Source/RedAlert4`): GameMode (`ARA4GameModeBase`), GameInstance, PlayerController, Pawn, and UE engine entry points.
3. **`RA4Input`** (`Source/RA4Input`): Mouse selection logic, marquee box selection, WASD camera panning, and CommandBus order dispatching.
4. **`RA4UI`** (`Source/RA4UI`): ViewModels, HUD state binding, UI input router, and widget catalog.

---

## 3. Determinism, State Hashing & Lockstep Verification

### State Hash Engine (`SimWorld::CalculateStateHash`)
- Hashes entity positions, health, shield, state flags, production queues, harvester cargo, and active commands.
- Verified by unit test `Lockstep.MatchingChecksumsDoNotReportDesync` and `ProvingGround.ForcedDesyncDetection`.
- **Finding**: State hash calculation is 100% deterministic and excludes frame-rate or visual presentation attributes.

### Lockstep Command Bus (`CommandBus.h`, `LockstepSession.h`)
- Implements lockstep frame assembly, input delay buffering (configurable target ticks), and tick isolation.
- Duplicate frames are discarded; missing frames trigger peer stalls until authoritative retransmission arrives.
- **Empirical Test Proof**:
  - `Lockstep.TwoPeersStayInSyncAcrossAFullMatch` (PASS)
  - `Lockstep.DesyncIsCaughtOnTheTickItHappens` (PASS)
  - `ProvingGround.HeadlessStressScenario500Entities` (PASS - 500 entities simulated for 1000 ticks without desync).

---

## 4. Architectural Gaps & Debt

1. **NoesisGUI Unreal Plugin Gap**:
   - `Source/RA4UI` defines `RA4NoesisHUDViewModel.h` and `RA4NoesisHUDViewModel.cpp`, but the `Plugins/NoesisGUI` plugin is absent.
   - Standard Unreal Build Tool (UBT) builds fail unless Noesis code is either guarded by preprocessor macros or Noesis plugin is restored.
2. **Simulation World Snapshot Leaks**:
   - `URA4PresentationSubsystem` polls `SimWorld` every frame to sync positions. High entity counts (>1000) show linear polling overhead. A delta-changed queue pattern is recommended.
