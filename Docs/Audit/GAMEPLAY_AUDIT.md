# RA4 — Gameplay Audit

**Audit date:** 2026-08-04
**Pinned commit:** `d915757`

Supersedes the previous version, which reported several systems as "Functional" in the
Presentation/Visuals column without any evidence of the game having been run, and cited a
camera source file (`Source/RA4Input/Private/Camera/RA4CameraComponent.cpp`) that does not
exist.

## Method and its hard limit

Every system below was assessed by reading the implementation and by running the headless
test suite. **The game was never launched.** No interactive session, no PIE, no packaged
build. Therefore *nothing in the "Visual" column of any table in this repository is
verified*, including in this document. Because `Source/RA4Tests` is not a UBT module
(see ARCHITECTURE_AUDIT.md §2.4), no automated test can reach engine-side behaviour either.
This is the single biggest verification gap in the project.

Legend: **SIM-VERIFIED** = implemented and covered by a passing headless test.
**SIM-ONLY** = implemented in the core, no test. **UNVERIFIED** = requires running the game.
**ABSENT** = not implemented.

## 1. Requested problem areas

| # | Area | Core | Visual | Verdict |
| --- | --- | --- | --- | --- |
| 1 | Building display | `SyncPresentation()` spawns/destroys `ARA4EntityActor` per entity; `RA4ArtMapping` binds meshes; 142 FBX present | not run | **SIM-ONLY / UNVERIFIED** |
| 2 | Resource gathering & unloading | full state machine + refinery queue | not run | **SIM-VERIFIED** |
| 3 | Harvester replacement | **no implementation found** | — | **ABSENT** |
| 4 | Camera control | `CameraController.cpp` | not run | **SIM-VERIFIED** (11 `Camera.*`) |
| 5 | WASD | `KeyBindings.cpp`, `ControlScheme.cpp` | not run | **SIM-VERIFIED** (15 `KeyBindings.*`, 11 `ClassicScheme.*`) |
| 6 | Construction | production queue, power gating | not run | **SIM-VERIFIED** (3 `Production.*`, 1 `Construction.*`) |
| 7 | Placement | footprint/terrain validation | ghost actor exists, not run | **SIM-VERIFIED** (2 `Placement.*`) |
| 8 | Combat commands | `OrderResolver.cpp` | not run | **SIM-VERIFIED** (13 `Orders.*`) |
| 9 | Pathfinding | `FlowField/NavGrid/MNavRouter/ReservationGrid/Formation` | not run | **SIM-VERIFIED** (17 `Navigation.*`, 5 `Movement.*`) |
| 10 | HUD | `HudSnapshot.cpp` + `RA4HUDWidget` | not run | **SIM-VERIFIED** (22 `Hud.*`) — data only |
| 11 | Minimap | **no sim-side producer**; 3 UI files reference it | not run | **ABSENT (data layer)** |
| 12 | Sound | `RA4AudioSubsystem`, 1088 real VO WAVs, 2 music tracks | not run | **SIM-ONLY / UNVERIFIED** |
| 13 | VFX | 3 files mention Niagara; no particle systems | not run | **EFFECTIVELY ABSENT** |
| 14 | Lighting | 2 files mention `DirectionalLight`; no Lumen/PostProcess config | not run | **EFFECTIVELY ABSENT** |
| 15 | Map surface | 7 files mention Landscape; ground plane spawned in code | not run | **UNVERIFIED** |
| 16 | Game data loading | `BibleContentLoader` reads `ra4_content.normalized.json` | n/a | **SIM-VERIFIED** (15 `BibleImport.*`, 4 `BibleContent.*`, 5 `Content.*`) |
| 17 | AI | utility commander | n/a | **SIM-VERIFIED** (46 tests) — see AI_AUDIT.md |
| 18 | Victory / defeat | implemented | not run | **SIM-VERIFIED** but only 2 tests |
| 19 | Replay | binary format, versioned, corruption-checked | n/a | **SIM-VERIFIED** (5 `Replay.*`) |
| 20 | State hash | 64-bit; `4e6d9e69576c002b` stable | n/a | **SIM-VERIFIED** but only 1 dedicated test |
| 21 | Packaged build | never produced | — | **ABSENT** |

## 2. Detail on the items that are not fine

### 2.1 Harvester replacement — ABSENT (item 3)

The previous audit stated: *"Player can order new Harvesters from War Factory; newly
produced harvesters auto-acquire nearest Ore node"* and marked it FULLY FUNCTIONAL.

A repo-wide search for replacement logic returns only three unrelated hits, all in
`TestInput.cpp`, all `SelectionMode::Replace` (marquee selection semantics, nothing to do
with harvesters). There is no auto-rebuild, no "replace lost harvester" behaviour, and no
test named for it.

Generic production would let a player *manually* queue another harvester, and a newly
built harvester does enter the gather loop via `FindNearestResourceNode`. But the specific
behaviour described — automatic replacement — does not exist. This matters because losing
harvesters without replacement is an economy death-spiral that AI and players both hit.

### 2.2 Minimap — no simulation-side data (item 11)

`Minimap` appears in exactly three files, all UI: `RA4HUDWidget.h`,
`RA4SidebarWidget.cpp`, `RA4ShowcaseWidget.cpp`. There is no minimap snapshot in
`HudSnapshot.cpp`, no fog-aware entity blip list, and no `Minimap.*` test. The previous
claim that it "displays own forces, neutral objects, and obscures hidden enemy units" is
unsupported — the data required to draw that does not leave the simulation.

Fog itself is real (`FogOfWarGrid`, 5 `FogOfWar.*` tests, and `IsEntityVisibleTo` gating
target acquisition), so the substrate exists; the minimap projection of it does not.

### 2.3 VFX and lighting — effectively absent (items 13, 14)

`Niagara` appears in 3 files, `DirectionalLight` in 2, and `ParticleSystem`, `Lumen`,
`PostProcess` in none. For an RTS, weapon tracers, muzzle flashes, explosions, build
effects and death effects are gameplay-legible feedback, not polish. Their absence means
combat is currently unreadable on screen regardless of whether the simulation is correct.

Per the architectural invariant "a missing decorative asset must not corrupt match state",
this is a presentation gap and not a correctness risk — but it is a large content gap.

### 2.4 Building display — plumbed, unproven (item 1)

The mechanism is real and reasonably written:

- `URA4SimWorldSubsystem::SyncPresentation()` (`RA4SimWorldSubsystem.cpp:765`) diffs sim
  entities against an `EntityActors` map, spawning `ARA4EntityActor` for new entities
  (`:829`) and removing them on death (`:796`, `:972`).
- `RA4ArtMapping` resolves entity → mesh; 142 blockout FBX exist; 36 PBR models were
  integrated per `afbb447`.

What is unproven is whether buildings *appear correctly* — correct mesh, scale, orientation,
footprint alignment, and material. That was the originally reported symptom and it cannot be
closed without running the editor.

### 2.5 Victory, state hash and save each rest on a single test

- `Victory.*` — 2 tests (lose-everything, surrender). No test for the multi-condition
  defeat rule the previous audit describes (all production structures *and* military units).
- `Checksum.*` — 1 test (`DetectsSingleBitChanges`).
- `SaveSystem.*` — 1 test (`MidMatchSaveAndRestorePreservesStateAndChecksum`).

These are the invariants the whole lockstep/replay design depends on. One test each is thin.
Note also that the previous audit cited the *save* test as proof of *replay* determinism;
they are different tests of different subsystems.

## 3. What is genuinely strong

The economy loop deserves specific credit. `SimWorld.cpp:1669-1735` implements a real
harvester state machine with a refinery `UnloadingQueue`, `UnloadPerTick` metering, and
queue hand-off to the next waiting harvester on completion. It is covered by
`Economy.HarvesterCompletesTheFullGatherLoop`, `Economy.MultiHarvesterTenCyclesAndRefineryQueue`
and `Economy.ResourceFieldsAreFinite`. The full vertical slice runs base-building to victory
in 2 816 ticks and harvests 6 000 credits deterministically.

Navigation (flow fields + reservation grid + formations, 17 tests) and order resolution
(13 tests, including attack-move) are likewise substantive.

## 4. Priority recommendation

Before any new gameplay feature: make `RA4Tests` a UBT module so engine-side behaviour can
be tested, then run the editor once and record what is actually seen. Roughly half the
"known problem areas" in the brief are visual, and none of them can be confirmed or denied
today.
