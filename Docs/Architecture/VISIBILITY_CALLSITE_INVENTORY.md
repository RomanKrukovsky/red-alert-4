# Visibility Call-Site Inventory (`VISIBILITY_CALLSITE_INVENTORY.md`)

**Created**: 2026-08-06 (NEXT_ACTIONS P-6; gates I-M6)
**Method**: exhaustive grep over `Source/` for objective-state reads (`GetAllCores`,
`GetAllTransforms`, `GetFogGrid`, `GetTransform` of non-owned ids) and fog queries, then manual
classification of every hit. Test code excluded. Line numbers as of `main @ 55cd12e` — they will
drift; the *classification* is the durable part.

## Why this exists

INVARIANT 10/K3 says objective truth must not leak to consumers acting for one player. "Is this
visible?" is today answered three different ways (fog grid query, `IsEntityVisibleTo`, owner-gated
`GetAllCores` scans), and I-M6 will replace the AI's way with belief (`GetRecon()`). Migrating
without an inventory is how omniscience quietly survives in one forgotten call site.

## Classification legend

- **OWN** — reads only entities owned by the acting player. Legitimate forever (own units are
  always known); no migration.
- **FOG-GATED** — reads enemies but filters through the fog grid. Correct *today*; becomes a
  **belief-migration site** at I-M6 (fog tells you *that* you see, belief tells you *what* you
  believe).
- **OMNISCIENT-BY-DESIGN** — reads everything and is *supposed to*: sim-internal rules, spectator,
  determinism plumbing. Must never be reachable from a per-player decision path.
- **LEAK** — reads enemy objective state on behalf of one player without a gate. Defect.

## The inventory

### RA4AI (acts for one player — strictest rules)

| Site | Class | Notes |
| :--- | :--- | :--- |
| `AIWorldView.cpp` `SimWorldView::UpdateMemory` | **FOG-GATED** | The single funnel: scans `GetAllCores`, gates enemies through `Fog->GetVisibility` (CurrentlyVisible/RadarDetected), writes `EnemyMemory` with confidence+decay. **This is the I-M6 migration point** — replace the fog gate + private memory with `GetRecon().GetPerceivedWorld(Player)` reads. Everything downstream consumes its output, not the world. |
| `AICommander.cpp` ×13 `GetAllCores` scans | **OWN** | Every loop filters `Owner != Player → continue` before reading anything. Site at :764 even documents the contract: *"Whether an enemy target exists is answered from fog-limited memory below, not by scanning the live world."* |
| `ValueMap.cpp` :187 | **OWN** | Finds own construction yard only. |
| `ThreatMap.cpp` | **CLEAN** | Consumes `std::vector<EnemyMemory>` — never touches SimWorld for enemies. Model for how downstream AI code should look. |
| `AIDirectors.cpp`, `OpponentModel.cpp`, `BattlePredictor.cpp` | **CLEAN** | Same: EnemyMemory/aggregates in, no objective reads. |
| `AIDebugOverlay.cpp` | **OMNISCIENT-BY-DESIGN** | Debug rendering; must stay out of decision paths (it emits no commands — verified). |

**AI verdict**: the architecture is already funneled — exactly one belief-relevant read site.
I-M6 is a one-funnel replacement plus deleting `EnemyMemory` in favour of `PerceivedTrack`, not a
15-site migration. The 13 own-only scans stay as they are.

### RA4Presentation / RedAlert4 (acts for the local player)

| Site | Class | Notes |
| :--- | :--- | :--- |
| `HudSnapshot.cpp` :378 minimap markers | **FOG-GATED** | Enemies filtered through `Fog->GetVisibility`. Belief-migration site (minimap should show ghosts/confidence per UI_UX_BIBLE §1.2). |
| `HudSnapshot.cpp` :85, :220 | **OWN** | Own-unit counts, own production. |
| `HudSnapshot.cpp` :183 `bPrimaryIsOwned` | **OWN** | Ownership flag of selection. |
| `RA4PlayerController.cpp` :180, :1532 | **OWN** | Selection filtered to local player. |
| `RA4PlayerController.cpp` :677 cursor picking | **LEAK (minor today, real at I-M6)** | Picking loop iterates ALL alive non-projectile entities with no fog filter: the cursor can "find" a fogged enemy. Today's damage is limited by downstream command validation, but tooltips/cursor state can reveal presence. Must become visibility-gated (today) and belief-gated (I-M6). |
| `RA4SimWorldSubsystem.cpp` :770 actor sync | **LEAK (presentation)** | Spawns/updates an `ARA4EntityActor` for EVERY alive entity, fogged or not, and nothing in the sync path hides enemy actors by visibility. Unless a render-side fog masking pass exists elsewhere (none found in `RA4EntityActor.cpp` — its `SetVisibility` calls are selection decals and death anims), **fogged enemy units are visible on screen**. This is a player-facing fog hole *today*, independent of the recon layer. |
| `RA4SimWorldSubsystem.cpp` :131 | **OMNISCIENT-BY-DESIGN** | Entity-count logging. |

### RA4Campaign (mission scripting)

| Site | Class | Notes |
| :--- | :--- | :--- |
| `MissionRuntime.cpp` :17, :55 | **OMNISCIENT-BY-DESIGN** | Objective evaluation (`CountOwned(Subject, …)`) counts a *specified* player's entities to resolve win/loss triggers. Mission logic is the referee, not a player: it may see everything, and its results reach the player only through objective-state UI. Caveat for mission designers: a trigger that reveals enemy details to the player (e.g. "enemy built X → tell the player") is an intel channel and must go through belief once I-M6 lands. Found by the funnel test on first run — the manual sweep missed it, which is precisely the failure mode the test exists for. |

### RA4Simulation (the truth itself)

| Site | Class | Notes |
| :--- | :--- | :--- |
| `SimWorld.cpp` :2498, :3008 via `IsEntityVisibleTo` | **OMNISCIENT-BY-DESIGN** | Sim-internal rules (targeting legality, recon observation input). The sim may read itself. |
| `IsEntityVisibleTo(Viewer, Index)` (SimWorld.h:200) | — | **The canonical per-entity gate.** Underused: presentation LEAK sites above should call this instead of inventing their own filters. |
| `ReconSystem.cpp` | **OMNISCIENT-BY-DESIGN** | Reads truth to *produce* belief; that is its job. Its output side is covered by INVARIANTS 9/10 and the read-surface leak test. |

## Actions

| # | Action | Priority | Owner stream |
| :--- | :--- | :--- | :--- |
| V-A | Fix `RA4SimWorldSubsystem.cpp` actor sync: hide enemy actors whose tile is not CurrentlyVisible/RadarDetected for the local player (`IsEntityVisibleTo` exists; call it) | **HIGH — player-facing fog hole today** | Unreal Integration |
| V-B | Fix `RA4PlayerController.cpp:677` picking: skip entities failing `IsEntityVisibleTo(LocalPlayer, …)` | HIGH | Unreal Integration |
| V-C | I-M6: replace the `UpdateMemory` fog gate with `GetPerceivedWorld(Player)`; delete `EnemyMemory` in favour of `PerceivedTrack`; ThreatMap consumes tracks | Gated on I-M2+ | AI |
| V-D | Same migration for `HudSnapshot` minimap (ghost markers with confidence per UI_UX_BIBLE) | Gated on I-M6 + UI | UI/UX |
| V-E | Leak detector (this package): `Recon.ObjectiveStateFunnelInventory` pins the funnel list — a NEW ungated `GetAllCores` consumer file fails the test until classified here | DONE with this document | QA |

## Leak-detector contract

The companion test (`TestRecon.cpp`, `Recon.ObjectiveStateFunnelInventory`) asserts that the set of
non-test files calling `GetAllCores`/`GetAllTransforms` matches this document's inventory, via a
build-time generated list. It cannot judge *how* a new caller uses the data — a human must classify
the site here and update the expected list; the test's job is to make "forgot to classify" a build
failure instead of a silent leak. The stronger runtime detector (fail any read of a non-owned
entity outside a whitelisted funnel) needs instrumentation hooks in SimWorld and lands with I-M6.
