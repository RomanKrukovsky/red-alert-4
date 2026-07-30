# FEATURE_OWNERSHIP_MATRIX

Date: 2026-07-28. Reflects the tree as audited, **before** any template exists here.
Every system gets exactly one canonical owner. A template may later *feed* an owner,
but may never become a second owner of the same system.

## Canonical owners (current)

| System | Canonical owner | Engine-free | Notes |
| --- | --- | --- | --- |
| Fixed-point math, RNG, ids, checksums | `RA4Core` | yes | determinism substrate |
| Command definition + serialization | `RA4Core` | yes | `Command`, `CommandFrame` |
| Command validation + application | `RA4Simulation` | yes | `SimWorld::ApplyCommand` — the only mutation path |
| Match state, tick order, entity storage | `RA4Simulation` | yes | SoA, 20 Hz, deferred destruction |
| Economy (harvest, credits, power) | `RA4Simulation` | yes | to be extracted to `RA4Economy` |
| Production + construction | `RA4Simulation` | yes | to be extracted to `RA4Production` / `RA4Construction` |
| Combat, damage matrix, projectiles | `RA4Simulation` + `RA4Content` | yes | to be extracted to `RA4Combat` |
| Pathing, flow fields, reservations, formations | `RA4Navigation` | yes | |
| Content definitions, damage table, validation, content hash | `RA4Content` | yes | |
| Selection, control groups, order resolution, camera, picking | `RA4Input` | yes | UE adapter lives in `RedAlert4` |
| Replay capture/playback/verification | `RA4Replay` | yes | |
| Presentation, actors, input adapter, sim↔UE bridge | `RedAlert4` | no | the only module allowed engine types |
| HUD, menus, viewmodels, themes | `RA4UI` | no | |
| Fog of war | `RA4FogOfWar` | — | **skeleton, not integrated** |
| AI | `RA4AI` | — | **skeleton** |
| Networking, authority, replication | `RA4Network` | — | **skeleton** |
| Save/load | `RA4SaveSystem` | — | **stub** |
| Minimap | *unassigned* | — | **does not exist** |
| Abilities (GAS) | *unassigned* | — | **does not exist**; plugin enabled but unused |
| Mass representation | *unassigned* | — | **does not exist**; plugin not even enabled |

## Ownership conflicts that already exist

These predate the templates and must be closed before integration, or a third
implementation will land in an already-contested slot.

| Conflict | Competing implementations | Required action |
| --- | --- | --- |
| **GameMode** | `ARA4UIShowcaseGameMode` (global default), `ARA4SkirmishGameMode` | Pick one canonical `ARA4GameMode`; the showcase becomes a derived debug mode or is deleted. Needs an ADR. |
| **HUD** | `ARA4HUD` (RA4UI), `ARA4RtsHud` (RedAlert4) | `ARA4RtsHud` is assetless debug drawing; `ARA4HUD` is the CommonUI path. One must be demoted explicitly. |
| **Input path** | `RA4Input` raw `BindKey` in `ARA4PlayerController` vs EnhancedInput plugin enabled and declared but unused | Decide: adopt EnhancedInput with an IMC asset, or drop the dead dependency. |

## Rules binding future template integration

1. A template's system is **Reject** by default. Adoption requires an ADR with a
   measured comparison against the incumbent owner.
2. No template class may be registered as a second `GameMode`, `GameInstance`,
   `PlayerController`, `HUD`, default `Pawn`, or `AssetManager`.
3. No template may write to `DefaultEngine.ini`, collision profiles, Gameplay Tag
   sources, input settings or packaging settings without an ADR.
4. Any template system that would duplicate a row above must be wrapped behind the
   corresponding `IRA4*` interface, never linked directly by `RA4Simulation`.
5. `RA4Simulation` must keep zero dependency on Engine, UMG, CommonUI, Niagara,
   MetaSounds, Skeletal Mesh, `ACharacter` and Blueprint. This is verified by the
   CMake harness compiling the same sources without Unreal.
