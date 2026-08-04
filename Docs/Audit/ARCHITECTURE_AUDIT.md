# RA4 — Architecture Audit

**Audit date:** 2026-08-04
**Pinned commit:** `d915757`

Supersedes the previous version, whose "Architectural Gaps" section describes a NoesisGUI
build blocker that no longer exists and cites files (`RA4NoesisHUDViewModel.cpp`) that are
not in `Source/RA4UI`.

## 1. The core architecture is sound and the key invariant holds

The separation between an engine-free deterministic core and the Unreal layer is real, not
aspirational. Verified structurally:

- `Tools/HeadlessBuild/CMakeLists.txt` compiles `RA4Core`, `RA4Content`, `RA4Simulation`,
  `RA4Combat`, `RA4Navigation`, `RA4FogOfWar`, `RA4AI`, `RA4Replay`, `RA4Campaign`,
  `RA4Input` and `RA4Presentation` into static libraries **without linking Unreal at all**.
  A core that genuinely could not be decoupled would not compile in this configuration.
- `RA4Presentation` needs a shim (`Tools/HeadlessBuild/UnrealStub`) added to its include
  path, which marks it correctly as the boundary module.

This is the project's strongest asset and should be protected in any subsequent work.

### 1.1 Determinism is enforced, not merely claimed

| Invariant | Evidence |
| --- | --- |
| Same seed + same commands ⇒ same state | `VerticalSlice.FullMatch` → `4e6d9e69576c002b`, identical across repeated runs |
| Frame rate does not affect simulation | `URA4SimWorldSubsystem::Tick` accumulates `TimeSinceLastSimTick` and steps a `while (… >= SimTickDelta)` fixed-timestep loop (`RA4SimWorldSubsystem.cpp:347-380`) |
| Desync detected at the tick it occurs | `Lockstep.DesyncIsCaughtOnTheTickItHappens`, `ProvingGround.ForcedDesyncDetection` |
| AI uses only player-legal commands | `AICommander::Tick(const SimWorld&, std::vector<Command>&)` — const world in, `Command` list out. No mutation path exists |
| Presentation does not mutate simulation | `SyncPresentation()` reads sim state and spawns/destroys `ARA4EntityActor`; no write-back observed |

## 2. Architectural violations found

### 2.1 Content is hardcoded in C++, contradicting ADR-0004 / ADR-004

`Docs/ADR/0004-content-lives-in-data-not-code.md` and `Docs/ADRs/ADR-004-Data-Driven-Content.md`
both state content must live in data. Campaign missions do not:

```
$ grep -c "Missions.push_back" Source/RA4Campaign/Private/CampaignDatabase.cpp
50
$ find . -iname "*mission*.json"
(no results)
```

Missions are ~50 hardcoded `push_back` calls with `MissionId = std::string(Chapter) +
"_mission_" + N`. Adding or balancing a mission requires a recompile. This also makes the
"38 authored campaign missions" claim untestable in the sense the ADR intended.

### 2.2 Three competing ADR directories

```
Docs/ADR/          12 files, format 0001-kebab-case.md
Docs/ADRs/         11 files, format ADR-001-Title-Case.md
Docs/Architecture/ADR/  10 files, format ADR-0001-kebab.md
```

33 ADRs across three directories with three naming conventions and overlapping subject
matter — e.g. fixed-point math is decided in both `Docs/ADR/0001` and `Docs/ADRs/ADR-002`;
data-driven content in both `Docs/ADR/0004` and `Docs/ADRs/ADR-004`. CLAUDE.md mandates a
single `Docs/Architecture/ADR/`. There is no index establishing which set is authoritative,
so "change requires an ADR" is currently unenforceable — one can be written in whichever
directory is most convenient.

### 2.3 ADR-0008 describes an AI architecture that is not built

`Docs/Architecture/ADR/ADR-0008-htn-utility-ai-commander.md` specifies an HTN planner.
`HTNPlan.cpp`, `HTNTask.cpp`, `HTNWorldState.cpp` and their headers (488 lines) exist,
reference only one another, and are **excluded from the CMake `RA4AI` library**. The AI that
actually runs is the utility loop in `AIStrategy.cpp` / `AICommander.cpp`. See AI_AUDIT.md.

### 2.4 The test module is outside the Unreal build graph

`Source/RA4Tests/` has no `Build.cs` and is absent from `RedAlert4.uproject`. It is a
CMake-only target. Consequence: no automated test can exercise any UObject, actor,
rendering or UMG behaviour, so the entire Unreal half of the architecture is structurally
untestable. This is the deepest architectural gap in the project.

### 2.5 `RA4Presentation` is in the engine-free build

`RA4Presentation` compiles headlessly only because `UnrealStub` fakes the engine types it
touches. That is a pragmatic choice for testing `HudSnapshot`, but it means the module sits
on both sides of the boundary and the stub must be kept in sync by hand. Worth an explicit
ADR rather than remaining an implicit arrangement.

## 3. Module inventory (measured)

| Module | Files | Lines | Engine-free | In CMake |
| --- | --- | --- | --- | --- |
| `RA4Core` | 11 | 1 064 | yes | yes |
| `RA4Content` | 10 | 2 599 | yes | yes |
| `RA4Simulation` | 8 | 4 191 | yes | yes |
| `RA4Combat` | 4 | 186 | yes | yes |
| `RA4Navigation` | 14 | 1 312 | yes | yes |
| `RA4FogOfWar` | 4 | 210 | yes | yes |
| `RA4AI` | 23 | 3 864 | yes | partially (4 files excluded) |
| `RA4Replay` | 3 | 434 | yes | yes |
| `RA4Campaign` | 7 | 1 794 | yes | yes |
| `RA4Network` | 6 | 665 | yes | **no** |
| `RA4Input` | 13 | 2 325 | yes | yes |
| `RA4Presentation` | 5 | 1 052 | via stub | yes |
| `RedAlert4` | 34 | 6 472 | no | no |
| `RA4UI` | 52 | 7 120 | no | no |
| `RA4Editor` | 14 | 1 053 | no (editor) | no |
| `RA4Tests` | 23 | 7 436 | yes | yes (but not in UBT) |

`RA4Network` is engine-free by design and is covered by 13 `Lockstep.*` tests compiled into
`RA4Tests`, but the module itself is not a separate CMake library — its logic lives in
`RA4Simulation/Private/LockstepSession.cpp`. The `Source/RA4Network/` module is the Unreal-side
transport. Worth confirming these are not two implementations of the same protocol.

## 4. Dependency direction

No circular dependencies were observed in the CMake link graph:

```
RA4Core → RA4Content → RA4Navigation/RA4FogOfWar/RA4Combat → RA4Simulation
                                                           → RA4Input, RA4Presentation, RA4AI, RA4Replay, RA4Campaign
```

`target_link_libraries(RA4Simulation PUBLIC RA4Content RA4Navigation RA4FogOfWar RA4Combat)`
is the widest node and is the correct shape for this design.

## 5. Recommendations (not actioned in this stage)

1. Consolidate 33 ADRs into `Docs/Architecture/ADR/` with an index; supersede duplicates
   explicitly rather than deleting them.
2. Add `Source/RA4Tests/RA4Tests.Build.cs` and register the module so engine-side behaviour
   becomes testable. This unblocks every "cannot verify" item in GAMEPLAY_AUDIT.md.
3. Move campaign missions from `CampaignDatabase.cpp` into data, per ADR-0004.
4. Either build the HTN planner or delete it and amend ADR-0008.
5. Write an ADR covering `UnrealStub` and `RA4Presentation`'s dual-side position.
