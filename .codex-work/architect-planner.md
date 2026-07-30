# Architect Planner Audit

Date: 2026-07-30
Repository: `/Users/romanmolodyko/Documents/red-alert-4`
Scope: read-only audit of `Research/RA3_SAGE_Study`, `Source/`, `*.Build.cs`, `*.Target.cs`, `Tools/HeadlessBuild/CMakeLists.txt`, `.github/workflows`, top-level docs.

## Notes about scope

- Root `AGENTS.md` file is not present in the repo tree. I used the instructions passed in task context plus real repo files.
- The worktree is dirty. I did not touch source files and did not revert anything.

## What I validated

### Read fully

- `Research/RA3_SAGE_Study/README.md`
- `Research/RA3_SAGE_Study/01_SOURCE_MATRIX.md`
- `Research/RA3_SAGE_Study/02_RA3_DATA_MODEL.md`
- `Research/RA3_SAGE_Study/03_SAGE_TO_UNREAL_ARCHITECTURE.md`
- `Research/RA3_SAGE_Study/04_CLEAN_ROOM_POLICY.md`
- `Research/RA3_SAGE_Study/05_IMPORTER_BLUEPRINT.md`
- `Research/RA3_SAGE_Study/06_RESEARCH_BACKLOG.md`

### Build / runtime validation

- Configured and built headless core with:
  - `cmake -S Tools/HeadlessBuild -B build/architect-audit -DCMAKE_BUILD_TYPE=Release`
  - `cmake --build build/architect-audit -j8`
- Ran `./build/architect-audit/RA4Tests`
  - Result: `208 passed, 0 failed`

### Validation paths covered

- Normal path:
  - `VerticalSlice.FullMatchFromBaseBuildingToVictory`
  - output included `harvested 6000 credits, final checksum 4e6d9e69576c002b`
- Failure path:
  - command rejection coverage exists and passed:
    - `Commands.RejectsOrdersOnUnitsYouDoNotOwn`
    - `Commands.RejectsStaleEntityHandles`
    - `Commands.ThrottlesCommandFloods`
- Integration edge:
  - replay / command stream path passed:
    - `Replay.RecordedMatchReplaysToTheSameResult`
    - `Replay.PlaybackReproducesEveryCheckpointChecksum`
    - `CommandBus.QueueAndDispatch`

## Factual architecture map

### Headless core graph actually built by CMake

From [Tools/HeadlessBuild/CMakeLists.txt](/Users/romanmolodyko/Documents/red-alert-4/Tools/HeadlessBuild/CMakeLists.txt:36):

```text
RA4Core
  -> RA4Content
  -> RA4Navigation
  -> RA4FogOfWar

RA4Content
  -> RA4Combat
  -> RA4Campaign

RA4Content + RA4Navigation + RA4FogOfWar + RA4Combat
  -> RA4Simulation

RA4Simulation
  -> RA4Input
  -> RA4Presentation
  -> RA4Replay
  -> RA4AI
```

This is the strongest positive fact in the repo: the deterministic path is real, builds fast, and is CI-gated in [.github/workflows/core.yml](/Users/romanmolodyko/Documents/red-alert-4/.github/workflows/core.yml:21).

### Core boundaries that are already correct

- Authoritative simulation boundary is explicit in [Source/RA4Simulation/Public/RA4Simulation/SimWorld.h](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Simulation/Public/RA4Simulation/SimWorld.h:3).
- Mutation boundary is explicit in [Source/RA4Core/Public/RA4Core/Command.h](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Core/Public/RA4Core/Command.h:3).
- Replay is command-stream based in [Source/RA4Replay/Public/RA4Replay/Replay.h](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Replay/Public/RA4Replay/Replay.h:3).
- Input is engine-free and resolves to commands in [Source/RA4Input/Public/RA4Input/OrderResolver.h](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Input/Public/RA4Input/OrderResolver.h:1).
- Presentation snapshot boundary is clean in [Source/RA4Presentation/Public/RA4Presentation/HudSnapshot.h](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Presentation/Public/RA4Presentation/HudSnapshot.h:7).

### Unreal build graph as declared today

Important difference: the Unreal build files do not encode the same clean boundaries as the headless build.

- Supposedly engine-free modules still declare Engine-side private deps:
  - [Source/RA4Input/RA4Input.Build.cs](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Input/RA4Input.Build.cs:25)
  - [Source/RA4Navigation/RA4Navigation.Build.cs](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Navigation/RA4Navigation.Build.cs:21)
  - [Source/RA4FogOfWar/RA4FogOfWar.Build.cs](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4FogOfWar/RA4FogOfWar.Build.cs:17)
  - [Source/RA4Campaign/RA4Campaign.Build.cs](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Campaign/RA4Campaign.Build.cs:17)
  - [Source/RA4Combat/RA4Combat.Build.cs](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Combat/RA4Combat.Build.cs:17)
  - [Source/RA4Network/RA4Network.Build.cs](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Network/RA4Network.Build.cs:17)
- `RA4Editor` depends on `UnrealEd` and `RedAlert4` in [Source/RA4Editor/RA4Editor.Build.cs](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Editor/RA4Editor.Build.cs:18).
- But the game target still adds `RA4Editor` in [Source/RedAlert4.Target.cs](/Users/romanmolodyko/Documents/red-alert-4/Source/RedAlert4.Target.cs:19).
- The server target claims it links only headless gameplay, but actually only names `RedAlert4`, and that module privately depends on `RA4UI`, `UMG`, `CommonUI`, `ModelViewViewModel` in [Source/RedAlert4/RedAlert4.Build.cs](/Users/romanmolodyko/Documents/red-alert-4/Source/RedAlert4/RedAlert4.Build.cs:30).

## Main findings

### 1. Deterministic core and command-stream architecture are real, not aspirational

Evidence:

- `SimWorld` is the source of truth and exposes `ApplyCommand()` plus fixed system order:
  - [SimWorld.h](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Simulation/Public/RA4Simulation/SimWorld.h:116)
  - [SimWorld.h](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Simulation/Public/RA4Simulation/SimWorld.h:145)
- `Command` / `CommandFrame` are compact, serializable, and versionable enough for replay/net:
  - [Command.h](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Core/Public/RA4Core/Command.h:54)
  - [Command.h](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Core/Public/RA4Core/Command.h:110)
- Replay verifies checkpoints against fresh simulation:
  - [Replay.h](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Replay/Public/RA4Replay/Replay.h:104)
- Headless CI runs every push:
  - [.github/workflows/core.yml](/Users/romanmolodyko/Documents/red-alert-4/.github/workflows/core.yml:21)

Risk level: low.

Expected gain from preserving this shape:

- replay, AI, input, and eventual network transport can stay one mutation path instead of diverging systems.

Tradeoff:

- the project must now protect this boundary in Unreal build config too, not only in CMake.

### 2. The runtime code boundary is cleaner than the Unreal build boundary

Evidence:

- Docs claim engine-free modules:
  - [README.md](/Users/romanmolodyko/Documents/red-alert-4/README.md:21)
- But several headless modules still declare `CoreUObject` / `Engine` in `.Build.cs`:
  - [RA4Input.Build.cs](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Input/RA4Input.Build.cs:25)
  - [RA4Navigation.Build.cs](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Navigation/RA4Navigation.Build.cs:21)
  - [RA4FogOfWar.Build.cs](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4FogOfWar/RA4FogOfWar.Build.cs:17)
  - [RA4Campaign.Build.cs](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Campaign/RA4Campaign.Build.cs:17)
  - [RA4Combat.Build.cs](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Combat/RA4Combat.Build.cs:17)
  - [RA4Network.Build.cs](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Network/RA4Network.Build.cs:17)

Why this matters:

- Today the code compiles headless because CMake is the real enforcer.
- Tomorrow a harmless UE include, reflection helper, or type leak can silently enter a “core” module because the Unreal graph already permits it.

Smallest fix:

- remove Engine-side private dependencies from modules that already compile without them, one module at a time, starting with `RA4Input`, `RA4Navigation`, `RA4FogOfWar`.

Expected risk reduction:

- build config starts enforcing the same architecture the source code already mostly follows.

### 3. Replay boundary is real; network boundary is mostly scaffold / stub

Evidence:

- Replay path is implemented and tested:
  - [Replay.h](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Replay/Public/RA4Replay/Replay.h:56)
  - [Docs/Roadmap.md](/Users/romanmolodyko/Documents/red-alert-4/Docs/Roadmap.md:29)
- `RA4Network` is a UE subsystem shell with no actual frame handoff:
  - local queue only in [RA4NetworkManager.cpp](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Network/Private/RA4NetworkManager.cpp:17)
  - no implementation in `OnCommandFrameReceived()` at [RA4NetworkManager.cpp](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Network/Private/RA4NetworkManager.cpp:33)
  - no implementation in `SubmitStateChecksum()` at [RA4NetworkManager.cpp](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Network/Private/RA4NetworkManager.cpp:40)

Mismatch vs target architecture:

- Target research wants `command stream + replay/net/data boundaries`.
- Current repo has solid command and replay boundaries, but network is not yet a real boundary adapter over `CommandFrame`.

Smallest fix:

- define one explicit seam between `URA4NetworkManager` and `CommandBus` / `URA4SimWorldSubsystem` around authoritative `CommandFrame` ingestion.

Expected risk reduction:

- networking work can grow from the existing command protocol instead of inventing a second path.

### 4. Shipping / editor separation is currently unsafe

Evidence:

- `RA4Editor` is an editor-only module in `.uproject`:
  - [RedAlert4.uproject](/Users/romanmolodyko/Documents/red-alert-4/RedAlert4.uproject:81)
- But `RedAlert4Target` still adds it to the game target:
  - [Source/RedAlert4.Target.cs](/Users/romanmolodyko/Documents/red-alert-4/Source/RedAlert4.Target.cs:19)
- `RA4Editor` depends on `UnrealEd`:
  - [Source/RA4Editor/RA4Editor.Build.cs](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Editor/RA4Editor.Build.cs:21)
- Server target claims headless behavior:
  - [Source/RedAlert4Server.Target.cs](/Users/romanmolodyko/Documents/red-alert-4/Source/RedAlert4Server.Target.cs:3)
- But `RedAlert4` depends on UI stack:
  - [Source/RedAlert4/RedAlert4.Build.cs](/Users/romanmolodyko/Documents/red-alert-4/Source/RedAlert4/RedAlert4.Build.cs:30)

Why this matters:

- This is the clearest mismatch with the target “shipping/editor dependencies” rule.
- It threatens dedicated server stripping and may make UBT validation harder once Unreal builds become part of CI.

Smallest fix:

- remove `RA4Editor` from `RedAlert4Target` immediately.
- then split client/UI glue from server-safe glue, or gate UI deps out of server builds.

Expected risk reduction:

- keeps future server and shipping targets from being contaminated by editor/UI dependencies.

### 5. Data ownership is split, and campaign data already leaks into code

Evidence:

- README says names and terminology should live in data, not code:
  - [README.md](/Users/romanmolodyko/Documents/red-alert-4/README.md:8)
- `RA4Campaign` hardcodes 38 missions, faction story metadata, map asset paths, cutscene ids and dialogue keys in C++:
  - [CampaignDatabase.cpp](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Campaign/Private/CampaignDatabase.cpp:14)
  - [CampaignDatabase.cpp](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Campaign/Private/CampaignDatabase.cpp:212)
- `RA4Editor` also creates UE `UDataAsset` content from normalized JSON:
  - [RA4ContentImportCommandlet.cpp](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Editor/Private/RA4ContentImportCommandlet.cpp:51)

Why this matters:

- There are already at least two content authorities:
  - engine-free `ContentDatabase`
  - editor-generated `UDataAsset` graph
- campaign metadata is not yet behind the same data/schema boundary.

Residual risk:

- content migration, clean-room swap, and replay/content hash compatibility become harder once runtime starts reading both paths.

Smallest fix:

- choose one authoritative authored format per domain:
  - `RA4Content` for simulation data
  - external authored data file for campaign registry
- keep UE assets as presentation/editor projection, not as a second gameplay source of truth.

## Empty / placeholder / stray modules

### Clearly placeholder today

- `RA4Network`
  - shell exists, gameplay boundary not implemented
- `RA4Editor::URA4MapBaker`
  - writes zero-filled dummy grid and returns success:
    - [RA4MapBaker.cpp](/Users/romanmolodyko/Documents/red-alert-4/Source/RA4Editor/Private/RA4MapBaker.cpp:11)

### Stray architecture branch

- `RAAI`
  - separate AI module, engine-bound, MassAI/StateTree/AIModule-based:
    - [RAAI.Build.cs](/Users/romanmolodyko/Documents/red-alert-4/Source/RAAI/RAAI.Build.cs:9)
  - not listed in `.uproject`
  - not part of the documented target architecture
  - overlaps conceptually with `RA4AI`

Assessment:

- `RAAI` is architectural debt even if harmless at runtime today, because it suggests a second AI direction that bypasses the engine-free command-emitter design.

## Documentation drift

Evidence:

- `README.md` says nothing graphical, networked or campaign-related exists yet:
  - [README.md](/Users/romanmolodyko/Documents/red-alert-4/README.md:16)
- `Docs/Roadmap.md` still says `150 tests, 150 passing`:
  - [Docs/Roadmap.md](/Users/romanmolodyko/Documents/red-alert-4/Docs/Roadmap.md:11)
- Actual current headless run on 2026-07-30 is `208 passed, 0 failed`.
- `Docs/Roadmap.md` says UI, save/load, fog, campaign runtime are “not started”:
  - [Docs/Roadmap.md](/Users/romanmolodyko/Documents/red-alert-4/Docs/Roadmap.md:48)
- But the repo now has implemented HUD snapshot tests, save/load tests, fog tests, campaign registry tests, and AI-vs-AI tests.

Why this matters:

- planning and architecture decisions become noisier when “target docs” and “repo reality” diverge this much.

## Three nearest safe migration tasks

### Task 1. Fix build-graph hygiene first

Scope:

- remove `RA4Editor` from `RedAlert4Target`
- stop declaring `Engine` / `CoreUObject` in engine-free modules that do not need them
- keep `RA4UI` and other client-only pieces out of server-safe module paths

Why first:

- zero gameplay rewrite
- highest risk reduction for future shipping/server work

### Task 2. Create one real command-frame adapter seam for networking

Scope:

- `URA4NetworkManager` should ingest / emit authoritative `CommandFrame`
- connect that seam to `CommandBus` or the sim subsystem
- do not add transform replication or alternate mutation paths

Why second:

- leverages the architecture that already works
- unlocks replay/net convergence instead of divergence

### Task 3. Consolidate data authority without changing match rules

Scope:

- move campaign registry literals out of C++ into authored data
- make editor import generate projections from the authoritative schema, not a parallel gameplay authority
- document which module owns content hash, versioning, and migration

Why third:

- reduces long-term data drift without rewriting simulation systems

## Overall assessment

The repo is already much closer to the target architecture than the docs imply on the deterministic core, command stream, replay, HUD projection, and AI command-emitter path. The main architectural risk is no longer “do we have an engine-free core?” The answer is yes.

The main risks now are boundary enforcement and drift:

- Unreal build graph does not yet protect the same boundaries the headless build proves.
- network is still a scaffold, not a real adapter over the command stream.
- shipping/editor separation is currently wrong in targets.
- data ownership is split and campaign content is already slipping into code.

Priority recommendation:

1. build/target hygiene
2. real `CommandFrame` network seam
3. single authoritative data boundary

