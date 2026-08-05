# ADR-0024: Battlefield Memory — Persistent Terrain State and Wreck Salvage

**Status**: **ACCEPTED 2026-08-05** (product owner, after independent review APPROVE-WITH-CHANGES and
application of all required changes: flow-field invalidation contract, per-region rolling hash).
Implementation may be scheduled; independent of the intel stream, parallel-safe per
PERCEPTION_WARFARE_DIRECTION §2.
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
- Nav/flow-field layer (ADR-0007) consumes TerrainStateLayer as a cost modifier — one-way
  dependency, no cycles. **Invalidation contract (review P-10)**:
  - **Batch phase**: terrain cost mutations accumulate in a dirty-cell set during combat/movement
    processing and are **applied once per tick**, in a fixed phase after TerrainStateLayer updates
    and before any pathfinding query of the same tick. No mid-tick cost change is ever visible to a
    query.
  - **Deterministic recompute order**: dirty cells are drained in ascending cell index; affected
    flow-field sectors are re-queued in ascending sector index. Field recomputation processes the
    queue FIFO with a fixed per-tick budget; a sector recomputed at tick T serves stale-but-
    consistent costs until then, identically on every peer (staleness is deterministic, so it is
    not drift).
  - **Recompute budget**: sector recompute work is charged against the existing 2.0 ms/sim-tick
    pathfinding budget (PERFORMANCE_BUDGETS §2), with the §4.3 line item (≤1.0 ms amortized)
    bounding the TerrainStateLayer-triggered share. Worst-case storm (mass crater event) degrades
    to more ticks of staleness, never to a budget breach.
- Hash-relevant, replay-relevant, save-relevant. **Hashing scheme (review P-10)**: the full grid
  is too large to fold into the checksum every interval, so TerrainStateLayer keeps a
  **per-region rolling hash** (regions = the flow-field sector partition): a cell mutation updates
  its region hash incrementally at mutation time; the checksum tick folds the (fixed-order) region
  hash vector, cost O(regions), not O(cells). Serialization writes regions in ascending index with
  per-region dirty flags for delta saves. A full-grid rehash exists as a debug validation path
  only.

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

**Additional design risks (review P-12)**:
- **Wreck-spam as griefing**: deliberately feeding cheap units into the enemy's economy zone floods
  the wreck cap and can flush *valuable* enemy wrecks out of it (cap eviction is
  oldest-lowest-value). Mitigation: eviction value uses original unit cost, so cheap-unit floods
  evict each other, not tanks; per-player wreck-source quota within the cap if playtests still show
  abuse.
- **Salvage rewarding failed attacks**: the attacker's dead units become salvage near the
  defender's base — for the *defender*. That is the intended punishment asymmetry (the field owner
  profits), but the reverse case (attacker salvaging mid-siege) weakens the cost of a failed push.
  Rule: salvaging requires an engineer-class unit holding position for a duration — under fire this
  is rarely viable, keeping failed attacks expensive.
- **Improvised roads collapsing route diversity**: the movement bonus makes the popular route more
  popular — positive feedback toward a single highway. Cap the bonus (single tier, +10% max), and
  contamination/cratering on the same cells cancels it, so contested roads decay naturally.

## Verification plan

1. Determinism: identical TerrainStateLayer hash across two instances after scripted 5k-tick battle.
2. Round-trip: save→load mid-match preserves layer + wrecks bit-exact.
3. TheaterState migration test (v1 blob loads under v2 reader).
4. Corruption test: truncated TheaterState ⇒ clean map load + logged warning, no crash, no partial state.
5. Perf: 4-player late-game map with max wrecks + full damage layer within tick and memory budgets (numbers to PERFORMANCE_BUDGETS.md first).
