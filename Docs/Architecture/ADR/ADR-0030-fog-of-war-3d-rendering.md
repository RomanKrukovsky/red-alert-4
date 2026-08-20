# ADR-0030: Fog of War Rendered in the 3D World

> **Restored and renumbered 2026-08-20**: this document was ADR-0028 and was deleted by commit
> 7ca8fd6 ("33") along with `FogVisibilityTexture.h` and the C++ upload path, while the same commit
> introduced two new ADR-0028 files about telemetry. The material generation in
> `RA4LayeredTerrainSetupCommandlet` survived, so the terrain material kept asking for a fog texture
> and four parameters that nothing set any more — which is why enemy units simply vanished with no
> fog drawn. Restored here under the next free number; ADR-0028 now means the telemetry work.

**Status**: Accepted — implementation in progress on `feat/fog-3d-rendering`.
**Depends on**: ADR-0002 (pure C++ sim decoupling), ADR-0026 (unreliable intelligence layer),
`Docs/Architecture/VISIBILITY_CALLSITE_INVENTORY.md` (V-A/V-B/V-F fog gates), `UI_UX_BIBLE.md` §1
(uncertainty language)

## Context

Fog of war is fully modelled in the simulation: `FFogOfWarGrid` holds a per-player, per-tile
`VisibilityState` of `NeverSeen` / `PreviouslySeen` / `CurrentlyVisible` / `RadarDetected`, and the
presentation layer already *gates* on it — hidden enemy actors (V-A), cursor picking (V-B) and
combat debug VFX (V-F) all consult `SimWorld::IsEntityVisibleTo` / `IsLocationVisibleTo`.

What is missing is the fog *itself*. Enemies vanish, but the ground under them renders as normal
lit terrain, so the player cannot distinguish **"there is nothing there"** from **"I am not looking
there"**. That is precisely the failure `UI_UX_BIBLE.md` §1 forbids for intel: absence of
information must be visible as absence, not as emptiness. Today the only surface that shows fog at
all is the minimap (`HudSnapshot`), which filters markers.

Note on prior art in this codebase: `FFogOfWarGrid::GetDirtyRegions()` exists and its comment reads
"Retrieve dirty regions to update textures" — the texture-upload path was designed for and never
wired up. A grep shows the method has no callers outside its own definition. This ADR consumes that
design rather than inventing a parallel one.

## Decision

### 1. One-way data flow: sim grid → texture → material

Each presentation update, the visibility grid for the **local player only** is uploaded into a
dynamic texture; the landscape material samples that texture to darken unseen ground.

```
FFogOfWarGrid (sim, authoritative)
        │  read-only, per tick
        ▼
FogVisibilityTexture  (UTexture2D, R8, one texel per tile)
        │  sampled by
        ▼
M_RA4_TerrainLayered  →  fog tint / desaturation
        │  and
        ▼
Post-process pass     →  soft edges, height-independent haze
```

The simulation knows nothing about the texture. This preserves INVARIANT 3 (presentation reads
snapshots, never mutates) and INVARIANT 1 (no Unreal types in sim modules): the texture and the
material live entirely in `RedAlert4`/`RA4Presentation`, and the grid is read through the existing
const accessor.

**Frame-rate independence is preserved** (INVARIANT: render rate never changes simulation results):
the upload is a pure read. A dropped or duplicated fog upload changes only what is drawn, never the
state hash. Fog rendering therefore needs no determinism test — but it must never be allowed to
become a *source* of gameplay decisions, which is why the gates keep asking the sim, not the
texture.

### 2. Texture format and cost

- One texel per **tile**, not per world unit: a 64×64 map is a 4 KB R8 texture. Even a 512×512 map
  is 256 KB. This is negligible, so no streaming or mip strategy is needed.
- Channel encoding, single byte per texel:
  `0 = NeverSeen`, `85 = PreviouslySeen`, `170 = RadarDetected`, `255 = CurrentlyVisible`.
  Evenly spaced so the material can branch on thresholds and so a future 5th state does not force a
  format change.
- Updated from `GetDirtyRegions()` when regions are available, falling back to a full upload when
  the dirty list is empty or the texture was just created. A full 4 KB upload is cheap enough that
  the dirty path is an optimisation, not a correctness requirement.
- **Bilinear filtering is deliberate**: hardware interpolation between texels is what turns a grid
  of squares into a soft boundary for free. The material must therefore treat the sampled value as
  a continuous 0..1 ramp, not as an enum — see §3.

### 3. Visual contract per state

| State | Ground appearance | Units | Rationale |
| :--- | :--- | :--- | :--- |
| `NeverSeen` | Black, no terrain detail | Hidden | Unexplored must read as unknown, not as dark grass. |
| `PreviouslySeen` | Terrain visible, heavily desaturated and dimmed | Hidden | The player remembers the *ground*; they do not know what is standing on it. This is the state that makes stale intel legible. |
| `RadarDetected` | Terrain visible, dimmed less than above | **Still hidden** | Consistent with the V-A MAJOR-2 decision: a radar contact is a minimap blip, not a rendered model. Radar tells you *something is out there*, not *what it looks like*. |
| `CurrentlyVisible` | Full lighting and colour | Visible | Baseline. |

Because filtering makes the sampled value continuous, the material interprets it as a ramp with the
four states as anchors, so transitions between neighbouring tiles blend rather than step.

### 4. Accessibility is part of the contract, not a follow-up

`UI_UX_BIBLE.md` §1.1 forbids conveying state by colour or brightness alone, and fog is exactly
such a signal. Therefore:

- Darkness carries the fog, but **desaturation carries it too** — a player who cannot perceive the
  brightness difference still sees the colour drop.
- A **high-contrast fog mode** (setting) replaces the gentle ramp with a hard, clearly delineated
  boundary and a stronger value separation between the four states.
- Fog strength is a **scalability setting** whose extremes are bounded: it may not be reduced to the
  point where `NeverSeen` and `CurrentlyVisible` are indistinguishable. Fog is not decoration —
  turning it off would hand the player information the rules deny them, so the floor is enforced in
  code rather than trusted to a config file.

### 5. Explicitly rejected

- **Volumetric fog / height fog for gameplay fog.** It is beautiful and it lies: volumetric density
  depends on view angle and distance, so the same tile reads differently from two camera positions.
  A gameplay-relevant boundary must be exact. Atmospheric height fog may still be used for mood,
  independently, at a strength that cannot be confused with the gameplay layer.
- **Spawning actors or decals per fogged tile.** A 64×64 map is 4,096 tiles; per-tile actors would
  cost more than the entire simulation. This is the naive approach that a texture exists to avoid.
- **Driving fog from `PerceivedWorld` (belief) instead of the fog grid.** Belief is about *entities*
  — what the player thinks is where. Terrain visibility is a different question with a different
  authority. When the recon layer's minimap ghosts land (inventory V-D), they will render on top of
  this fog, not replace it.
- **Letting the material read the texture to gate gameplay.** Any rule that needs to know "can the
  player see this" asks `SimWorld`, as it does today. The texture is a picture of the answer, never
  the answer.

## Consequences

**Positive**: closes the last surface where fog was invisible; makes the perception-warfare pillar
legible on the primary screen rather than only on the minimap; consumes an already-designed
(`GetDirtyRegions`) path; cost is a few kilobytes and one texture sample.

**Negative / risks**:
- The landscape material is generated by `RA4LayeredTerrainSetupCommandlet`, so the fog nodes must
  be added there rather than hand-wired in the editor, or the next commandlet run silently reverts
  them. This is the same class of failure as the ground-tiling fix that was "silently skipping"
  (commit 66f0b4a).
- Anything not part of the landscape (props, water, buildings) is unaffected by a landscape-material
  approach and needs the post-process pass to be covered — otherwise unexplored areas show floating
  lit props over black ground. Sequencing matters: material first, post-process before this is
  considered done.
- Local-player-only means split-screen or an observer seat needs a second texture. The subsystem
  already hardcodes local player 0 (recorded debt in V-A); this adds one more caller with the same
  assumption rather than a new problem.

## Verification plan

1. **Headless**: the state→texel encoding is a pure function and is unit-tested without Unreal
   (`FogTexel(VisibilityState)` in a sim-side or presentation-side header, no UTexture involved).
2. **Headless**: dirty-region upload and full upload produce identical texel buffers for the same
   grid — the optimisation cannot drift from the fallback.
3. **UE build**: editor target compiles with the material nodes generated by the commandlet.
4. **Visual, in editor**: unexplored area is black, explored-not-visible is dim and desaturated,
   currently-visible is full colour, boundaries are soft. **This is a real visual check and will be
   recorded as owed until someone launches the editor and looks** — the V-A row already carries one
   such debt and it must not be quietly inherited.
5. **Accessibility**: high-contrast mode produces a measurably different boundary; the fog-strength
   floor cannot be configured away.
