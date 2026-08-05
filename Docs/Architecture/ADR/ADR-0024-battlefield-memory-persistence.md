# ADR-0024: Battlefield Memory — Persistent Terrain State and Wreck Salvage

**Status**: Proposed (blocked until remediation/foundation-fixes is green; no implementation authorized)
**Depends on**: ADR-0003 (entity/component model), ADR-0005 (replay/checkpointing), ADR-0009 (data-driven content), SAVE_AND_REPLAY.md

## Context

Two related ideas from the perception-warfare direction:

1. The battlefield permanently records combat: craters, burned forests, wrecks, improvised roads, contamination zones — persisting within a match and (in campaign) across missions.
2. Destroyed units leave *usable remains*: hull, ammo, crew, salvage value, capturable technology.

Both reduce to one architectural need: a **TerrainStateLayer** and a **WreckEntity** class that are ordinary deterministic sim state, serializable, and exportable between campaign missions.

## Decision

### 1. TerrainStateLayer

- A per-cell overlay grid (same resolution family as fog/nav grids): `{SurfaceType, DamageLevel, ContaminationType, ContaminationTicksRemaining, TrafficCounter}`.
- All mutations happen in-tick from combat/movement events (explosion at cell ⇒ DamageLevel up; N vehicle passes ⇒ TrafficCounter up, above threshold cell becomes `ImprovisedRoad` with a movement-cost bonus in the nav layer).
- Nav/flow-field layer (ADR-0007) consumes TerrainStateLayer as a cost modifier — one-way dependency, no cycles.
- Hash-relevant, replay-relevant, save-relevant.

### 2. WreckEntity

- On destruction, a unit spawns a wreck entity from a data-driven `WreckTable` (per archetype): salvage credits, capturable-tech flag, cover value, hazard (ammo cook-off), crew-survivor spawn chance (seeded RNG stream).
- Wrecks are entities, so existing systems work on them for free: they occlude, they can be targeted, engineers can interact via ordinary commands (salvage / rig with explosives / recover).
- Lifetime and cap: hard cap per map (config), oldest-lowest-value wrecks despawn deterministically — bounds entity count and hash cost.

### 3. Campaign persistence

- End-of-mission: TerrainStateLayer + surviving named wrecks serialize into a `TheaterState` blob attached to the campaign save (versioned, migratable per save-format rules).
- Mission start: map loader applies TheaterState deltas over the authored map. Missing/invalid blob ⇒ authored map loads clean (a decorative persistence failure must never corrupt a mission — extends the "missing decorative asset" invariant).
- Skirmish/multiplayer: persistence OFF by default; in-match memory only.

### 4. Presentation

Presentation reads TerrainStateLayer to select decals/VFX/foliage states. It never writes. Visual density is a scalability setting; sim layer is identical regardless of graphics settings (frame-rate independence invariant).

## Consequences

**Positive**: high perceived-uniqueness per engineering dollar — the sim work is a grid overlay plus one entity class; the "wow" ships in presentation. Wreck salvage deepens economy (ties into ADR-0016 resource model) without new currency types.

**Negative / risks**:
- Save-format churn: TheaterState must be versioned from day one.
- Nav-cost coupling: improvised roads change pathing mid-match — needs determinism tests around flow-field invalidation.
- Wreck spam in large battles — cap policy above is mandatory, not optional.
- Campaign QA surface: missions must be beatable with ANY plausible TheaterState. Requires property-based scenario tests (randomized valid TheaterStates ⇒ mission-critical paths remain traversable). Mission design rule: critical routes are persistence-immune zones.

## Verification plan

1. Determinism: identical TerrainStateLayer hash across two instances after scripted 5k-tick battle.
2. Round-trip: save→load mid-match preserves layer + wrecks bit-exact.
3. TheaterState migration test (v1 blob loads under v2 reader).
4. Corruption test: truncated TheaterState ⇒ clean map load + logged warning, no crash, no partial state.
5. Perf: 4-player late-game map with max wrecks + full damage layer within tick and memory budgets (numbers to PERFORMANCE_BUDGETS.md first).
