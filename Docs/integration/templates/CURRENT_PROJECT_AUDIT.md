# CURRENT_PROJECT_AUDIT

Date: 2026-07-28
Auditor: Orchestrator (direct inspection, no subagents — see INTEGRATION_LOG.md)
Baseline: git tag `baseline-pre-template-integration` → commit `7f9f9e9` (branch `nav-milestone`)
Archive: `~/Documents/ra4-backups/ra4-pre-template-integration-20260728-212529.tar.gz` (207 MB, 574 files, **includes** the 83 uncommitted files the tag does not)

Every number below was produced by running a command against the tree, not estimated.

---

## 1. Engine and targets

| Item | Value |
| --- | --- |
| EngineAssociation | 5.6 (installed build at `/Users/Shared/Epic Games/UE_5.6`, 5.6.1, CL 44394996) |
| Build verified | `Build.sh RedAlert4Editor Mac Development` → **Result: Succeeded** (observed compile + link of project modules) |
| Target platforms declared | Windows, Linux, Mac |
| Targets | `RedAlert4` (Game), `RedAlert4Editor` (Editor), `RedAlert4Server` (Server) |
| BuildSettingsVersion | V5 |
| IncludeOrderVersion | Unreal5_6 |

**Finding A-1 (medium).** `RedAlert4Server.Target.cs` lists only `RedAlert4` in `ExtraModuleNames`, while the Game and Editor targets list all fifteen `RA4*` modules. The server target is therefore configured asymmetrically. This must be reconciled before any dedicated-server claim is made.

## 2. Plugins: enabled vs actually used

| Plugin | Enabled in .uproject | Files referencing it in `Source/` |
| --- | --- | --- |
| GameplayAbilities | yes | **0** |
| CommonUI | yes | 2 |
| ModelViewViewModel | yes | 1 |
| EnhancedInput | yes | **0** (`UInputMappingContext`: 0) |
| FunctionalTestingEditor | yes | 0 |
| PythonScriptPlugin | yes | 0 |
| **MassEntity / MassGameplay / MassAI** | **not enabled** | **0** |
| StateTree | not enabled | 0 |

Symbol-level counts across all `.h/.cpp/.cs`:

```
AbilitySystemComponent   0      FMassFragment      0
UGameplayAbility         0      UMassProcessor     0
UAttributeSet            0      FGameplayTag       0
UInputMappingContext     0      UBehaviorTree      0
UCommonActivatableWidget 2      PrimaryDataAsset   2
UMVVMViewModelBase       1      UDataAsset         0
```

**Finding A-2 (high).** GAS is enabled as a plugin and `RedAlert4.Build.cs` declares `GameplayAbilities`, `GameplayTags` and `GameplayTasks` as dependencies, but **no code uses any of them**. The same holds for EnhancedInput. These are dead dependencies: they inflate build time and link surface while providing nothing, and they make the project *look* like it has GAS when it does not.

**Finding A-3 (high).** There is **no MassEntity in this project at all** — not enabled, not referenced. Any statement that the project "uses Mass" would be false. The RTS/RPG Unit Template is the proposed source of Mass; nothing here would conflict with it, but equally nothing here can be compared against it yet.

**Finding A-4 (high).** There are **no Gameplay Tags** anywhere. `GAMEPLAY_TAG_MIGRATION.md` therefore has no source-side content to describe and is not written in this pass.

## 3. C++ modules

21 modules declared in `.uproject`. Measured source size (`.h` + `.cpp` lines):

| Module | LOC | State |
| --- | ---: | --- |
| RA4Tests | 3024 | real — 103 tests |
| RA4Simulation | 2900 | real — authoritative match simulation |
| RA4UI | 2895 | real — CommonUI/MVVM foundation, themes, viewmodels |
| RedAlert4 | 1646 | real — presentation, input adapter, subsystem |
| RA4Input | 1359 | real — camera, selection, order resolution, hit test |
| RA4Navigation | 1295 | real — NavGrid, FlowField, ReservationGrid, MNavRouter, Formation |
| RA4Content | 1286 | real — data model, damage table, validation, content hash |
| RA4Core | 1062 | real — fixed point, RNG, ids, commands, serialization, checksum |
| RA4Campaign | 446 | partial |
| RA4Replay | 434 | real — record, playback, checksum verification |
| RA4FogOfWar | 180 | skeleton |
| RA4AI | 146 | skeleton |
| RA4Network | 133 | skeleton |
| RA4Editor | 77 | skeleton |
| RA4Units, RA4Buildings, RA4Economy, RA4Combat, RA4Audio, RA4Modding, RA4SaveSystem, RA4Diagnostics | 30 each | **empty stubs — module boilerplate only** |

**Finding A-5 (medium).** Eight modules exist as name-only stubs. Their functionality currently lives inside `RA4Simulation` (economy, production, combat are systems in `SimWorld.cpp`). The target architecture in the brief lists them as separate owners. Splitting them is a real refactor, not a rename, and must not be conflated with template integration.

### Dependency direction (verified from `Build.cs`)

```
RA4Core        → Core
RA4Content     → Core, RA4Core
RA4Navigation  → Core, RA4Core          [+ private: CoreUObject, Engine]
RA4Simulation  → Core, RA4Core, RA4Content, RA4Navigation      ← no Engine, no UMG
RA4Input       → Core, RA4Core, RA4Content, RA4Simulation       [+ private: CoreUObject, Engine]
RA4Replay      → Core, RA4Core, RA4Content, RA4Simulation
RedAlert4      → Engine, UMG, CommonUI, MVVM, GAS, RA4*         ← the only presentation module
```

**Good:** `RA4Simulation` has **no dependency on Engine, UMG, CommonUI, Niagara or Slate**. The brief's core invariant is already satisfied, and the same sources compile in a plain CMake harness (`Tools/HeadlessBuild`) in ~2 seconds.

**Finding A-6 (low).** `RA4Navigation` declares private dependencies on `CoreUObject` and `Engine` that it does not use — it compiles cleanly in the engine-free CMake harness. Since `RA4Simulation` depends on `RA4Navigation`, this dead dependency weakens the engine-free guarantee on paper. Remove it.

## 4. Gameplay framework classes

| Framework slot | Present | Class |
| --- | --- | --- |
| GameModeBase | **two** | `ARA4UIShowcaseGameMode`, `ARA4SkirmishGameMode` |
| GameStateBase | **absent** | — |
| PlayerController | yes | `ARA4PlayerController` |
| PlayerState | **absent** | — |
| GameInstance | **absent** | — |
| WorldSubsystem | yes | `URA4SimWorldSubsystem` (tickable, owns the simulation) |
| LocalPlayerSubsystem | **absent** | — (`URA4UIRouterSubsystem` exists; kind not yet confirmed) |
| HUD | **two** | `ARA4HUD` (RA4UI), `ARA4RtsHud` (RedAlert4) |
| Pawn | yes | `ARA4CameraPawn` |
| Asset Manager | default | no custom `AssetManagerClassName` in `DefaultEngine.ini` |

`Config/DefaultEngine.ini`:
```
GameDefaultMap        = /Engine/Maps/Entry
GlobalDefaultGameMode = /Script/RedAlert4.RA4UIShowcaseGameMode
```

**Finding A-7 (high, pre-existing conflict).** Two GameModes and two HUD classes already exist, and the *UI showcase* GameMode — not the playable one — is the global default. The brief forbids competing GameModes. This duplication predates any template and must be resolved before templates are introduced, or a third GameMode arrives into an already-ambiguous slot.

**Finding A-8 (high).** `ARA4GameState`, `ARA4PlayerState`, `URA4GameInstance` and `URA4LocalPlayerSubsystem` — all four required by the brief — **do not exist**. There is no replicated per-player state object at all, which is a prerequisite for anything multiplayer.

## 5. Systems inventory

| System | Status | Location |
| --- | --- | --- |
| Deterministic simulation, fixed 20 Hz tick, system order | **working** | `RA4Simulation/SimWorld.cpp` |
| Command pipeline + full server-side validation + rate limit | **working** | `RA4Core/Command.h`, `SimWorld::ApplyCommand` |
| Economy (harvest, refinery, credits, finite fields) | **working** | systems inside `SimWorld` |
| Power grid (derated by damage, throttles production) | **working** | `SimWorld::SystemPower` |
| Production queues, placement, cancel/refund, rally | **working** | `SimWorld` |
| Combat (armour/warhead matrix, projectiles, splash, turrets) | **working** | `SimWorld` |
| Navigation (hierarchical grid, portals, flow fields, reservations, formations) | **working, with regressions** | `RA4Navigation` + `SimWorld::SystemMovement` |
| Selection, control groups, contextual orders, camera, hit test | **working** | `RA4Input` (+ UE adapter in `RedAlert4`) |
| Replay record/playback + checksum verification | **working** | `RA4Replay` |
| Fog of war | **skeleton** (180 LOC, not integrated into `SimWorld`) | `RA4FogOfWar` |
| AI | **skeleton** (146 LOC) | `RA4AI` |
| Networking / dedicated server | **skeleton** (133 LOC), no replication, no RPC, no sessions | `RA4Network` |
| Save/load | **stub** | `RA4SaveSystem` |
| Minimap | **absent** | — |
| GAS abilities | **absent** | — |
| Missions | partial | `RA4Campaign` |
| Audio | **stub** | `RA4Audio` |

### Determinism evidence

- Identical final state checksum `edd01225b0d869ee` from `-O3` and from `-O0 + ASan + UBSan`.
- Per-tick checksum comparison across two runs of a full match: identical.
- Replay playback reproduces every recorded checkpoint checksum.

## 6. Tests

`cmake --build build/hb --target RA4Tests && ./build/hb/RA4Tests`

```
98 passed, 5 failed, 100 ms
```

Failing (all in navigation-integration work uncommitted in the tree, verified reproducible in isolation and independent of `RA4Input`):

```
Movement.UnitsReachTheirDestination
Movement.QueuedWaypointsAreFollowedInOrder
Economy.HarvesterCompletesTheFullGatherLoop
VerticalSlice.FullMatchFromBaseBuildingToVictory   (harvested == 0)
Navigation.LocalAvoidancePicksBestOpenNeighbor
```

Single symptom: a unit reports arrival (`bHasDestination == false`) while still >120 units from its goal, so harvesters never reach the refinery.

No Automation Tests, no Functional Tests, no dedicated-server tests, no load or soak tests exist.

## 7. Content

| Metric | Value |
| --- | --- |
| `.uasset` | 57 (all under `Content/RA4UI` — themes and widgets) |
| `.umap` | **0** |
| `Plugins/` | **does not exist** |
| `Content/ThirdParty/` | does not exist |

**Finding A-9 (high).** There is **no map in the project**. Nothing can be played, profiled in Insights, cooked meaningfully, or used as a dedicated-server test bed until a map exists. This blocks the brief's vertical technical circuit independently of the templates.

## 8. Documentation

Existing and current: `Docs/Architecture.md`, `Docs/Roadmap.md`, `Docs/ThreatModel.md`, `Docs/ADR/0001..0011`, `AssetAcquisitionPlan.md`, `AssetRequirements.csv` (87 rows), `Content/AssetRegistry/ThirdPartyAssets.json` (empty registry — no third-party asset has been acquired).

## 9. Blocking findings, ranked

1. **The three packages are not present on this machine** (§ see `LICENSE_AND_AI_USAGE_REPORT.md`). Integration cannot begin.
2. No map, no GameState/PlayerState/GameInstance, no replication → the vertical circuit cannot be run end to end even with the packages.
3. Two GameModes and two HUDs already compete; the wrong GameMode is the global default.
4. Five failing simulation tests in uncommitted concurrent work.
5. A second session is editing this working tree live. Branch operations are unsafe until it stops.
