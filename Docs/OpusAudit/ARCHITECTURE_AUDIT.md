# Opus Audit — Architecture Audit

**Auditor**: Claude Fable 5
**Date**: 2026-08-04
**Baseline**: `cae7b36`

---

## Module Architecture

### Engine-Free Core (11 modules)
All compile with standard Clang, no Unreal dependency:

| Module | Files | Purpose |
|--------|-------|---------|
| RA4Core | 8h/3cpp | Fixed-point math, commands, checksums, IDs, RNG |
| RA4Content | 5h/4cpp | Data-driven content database, JSON bible loading |
| RA4Simulation | 4h/3cpp | SimWorld, CommandBus, LockstepSession |
| RA4Navigation | 6h/7cpp | NavGrid, FlowField, MNavRouter, Formations |
| RA4Combat | 1h/1cpp | ArmorMatrix |
| RA4FogOfWar | 2h/2cpp | FogOfWarGrid |
| RA4AI | 11h/11cpp | AICommander, Doctrine, Strategy, TacticalOps |
| RA4Replay | 1h/2cpp | Replay recording/playback |
| RA4Input | 6h/7cpp | Camera, Selection, Orders, HitTest, ControlScheme |

### Unreal-Dependent Modules (4 modules)
| Module | Files | Purpose |
|--------|-------|---------|
| RedAlert4 | 17h/17cpp | Main game module, GameMode, PlayerController |
| RA4UI | 28h/24cpp | UMG widgets, ViewModels, Themes, Navigation |
| RA4Network | 3h/3cpp | RPC channels, NetworkManager subsystem |
| RA4Editor | 7h/7cpp | Commandlets (import, landscape, terrain) |

### Separate Web UI
| Path | Purpose |
|------|---------|
| `ra4-ui/` | React/TypeScript web prototype (EVALog, CommandBar, HUD, Minimap) |

---

## Architectural Invariants

### Invariant 1: Simulation determinism
**Status**: VERIFIED — Fixed-point math only, integer RNG, no `float` in simulation, `FixedMulRaw` uses 128-bit intermediate, `VerticalSlice.IdenticalInputsProduceIdenticalStateEveryTick` proves identical state across runs.

### Invariant 2: Command-only state mutation
**Status**: VERIFIED — All state changes go through `SimWorld::ApplyCommand()` which validates ownership, affordability, tech, placement, and rate limits. AI generates commands through the same interface.

### Invariant 3: Presentation does not mutate simulation
**Status**: VERIFIED architecturally — `RA4Presentation` module is engine-free and reads from `SimWorld`. The Unreal `RedAlert4` module contains the bridge code. No direct state mutation from presentation was found in the source.

### Invariant 4: Server authority in networked play
**Status**: VERIFIED — `LockstepSession` has `bIsAuthority`, only authority assembles frames. `ClientDoesNotAdjudicateChecksums` test proves clients don't overstep. `FrameAssemblyFollowsSlotOrderNotArrivalOrder` proves deterministic assembly.

### Invariant 5: Content validation before match
**Status**: VERIFIED — `ContentDatabase::Validate()` catches missing keys, invalid health, dangling refs. `HashChangesWithBalanceEdits` proves hash detects balance changes.

### Invariant 6: Replay format versioning
**Status**: VERIFIED — `RejectsFilesFromADifferentContentBuild` test proves content hash validation rejects replays from different balance states.

---

## Issues Found

1. **`RedAlert4` is a trademarked name** used throughout module names and the main game module — violates the project's own IP migration rule
2. **3 test files exist but are not compiled**: `Test_UI.cpp` (4 tests), `Test_HUDIntegrationAndGameState.cpp` (2 tests), `Test_RA3PipelineAndCommandBus.cpp` (6 tests) — wait, these ARE in CMakeLists.txt. Correction: all 21 files ARE compiled.
3. **RA4FogOfWar is linked twice** in the CMakeLists.txt — harmless but shows a maintenance gap
4. **No `SimConfig.h` constants file** — `kTicksPerSecond`, `kChecksumIntervalTicks` are defined inline, should be centralized
5. **`ContentDatabase::GetEntities()` returns mutable reference** — callers can mutate the database without validation
