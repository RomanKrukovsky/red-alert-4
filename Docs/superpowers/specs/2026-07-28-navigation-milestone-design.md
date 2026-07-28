# Navigation Milestone — Design

**Date:** 2026-07-28
**Stage:** Roadmap stage 2 ("navigation") on the existing deterministic headless core.
**Status:** Approved by user; ready for implementation plan.

## Context

The Red Alert 4 project already has a real, deterministic, engine-independent C++
simulation core (`Source/RA4Core`, `RA4Simulation`, `RA4Content`, `RA4Replay`)
that compiles headless (no Unreal), passes 56 tests, and has verified cross-build
determinism. The full vertical slice — HQ → power → refinery → harvester → war
factory → 4 tanks → assault → enemy HQ destroyed → replay verified — already
runs. UE 5.6 is installed; the `.uproject`/`.Build.cs`/`.Target.cs` are written but
**never compiled through UnrealBuildTool** (that is a later stage).

This milestone finishes the navigation stage as defined in `Docs/Roadmap.md`.

**Corrections to the roadmap's framing**, confirmed by reading the actual code:

1. Flow fields **are already** wired into `SystemMovement`. It queries the `NavGrid`,
   uses the cached shared `FlowField`, follows `FlowField::GetDirection` per tile, and
   does turn-rate-limited steering + acceleration + passability blocking + blocked-tick
   accounting. The roadmap's "connect flow fields to movement" is done.
2. What is genuinely missing is the rest of the roadmap's "Not started" list for
   navigation: **macro routing** (sector-portal A\*), **reservation grid**, **local
   avoidance**, **formations**, an **async per-tick repath budget**, and a **debug
   snapshot**. The acceptance test is: 300 units repathing, no per-unit A\*,
   deterministic checksum.

This is not a green-field design. It extends the existing `NavGrid`, `FlowField`,
`FlowFieldCache`, and `SystemMovement`.

## Decisions (locked with the user)

1. **Topology:** Full hierarchical — sector-portal A\* → per-sector shared flow fields
   as sub-goals, plus a reservation grid. More robust on large maps and many distinct
   targets, at the cost of more invalidation surface (accepted).
2. **Reservations:** Soft reservation grid with per-tick expiry and a blocked-tick
   fallback. Deterministic, cheap; deadlock risk bounded by the existing
   `BlockedTicks` repath. Stable slot-order tie-breaking.
3. **Async model:** Time-sliced on the sim thread, per-tick repath budget. No
   background threads in the sim (determinism forbids it). "Async" = amortized
   across ticks in stable entity-slot order.

## Scope

### In scope

- Hierarchical macro routing: sector-portal A\* producing a coarse corridor of
  sector-center sub-goals; per-sector shared flow fields (reuses the existing
  `FlowField` + `FlowFieldCache`, key extended to sub-goal sector center, not the
  final tile).
- Soft `ReservationGrid` over the tile map: `OccupantId` + `ExpiryTick` per tile,
  per-tick expiry, stable slot-order tie-breaking, blocked-tick fallback repath.
- Local avoidance: when the next tile is reserved by another unit, pick the best
  open neighbor by flow-direction alignment; fall back to waiting.
- Formations: a formation descriptor assigns per-unit offset slots; the group
  leader's path drives the formation; members path to their slot.
- Time-sliced repath budget: `SystemMovement` spends at most a configurable budget
  (flow-field builds / macro-path builds) per tick; unprocessed units follow their
  last-known flow direction. All updates applied in stable entity-slot order.
- Debug snapshot (headless, pure data first): a `NavDebug` struct that the tests and
  a future Unreal bridge can read — grid, sectors, portals, flow vectors,
  reservations, blocked tiles. **No `DrawDebug*` in the sim module.**
- Regression test: 300 units issued a move order to one rally point across a map
  with a chokepoint; assert no per-unit A\* (flow-field build count ≤ ceiling), all
  units arrive within a tick budget, and the match checksum matches across `-O3`
  and `-O0+ASan`.

### Out of scope (deferred to later stages)

- Unreal visual debug draw (presentation bridge stage).
- Networked movement reconciliation (networking stage).
- AI-driven pathing (AI stage).
- Naval/air movement *systems*. The `NavLayer` enum exists; this milestone wires
  ground layers only. Air/naval get their own systems later.
- ORCA/continuous avoidance. Tile-based local avoidance is enough for the milestone.

## Files

- `Source/RA4Navigation/Public/RA4Navigation/MNavRouter.h` / `Private/MNavRouter.cpp` — new sector-portal A\*.
- `Source/RA4Navigation/Public/RA4Navigation/ReservationGrid.h` / `Private/ReservationGrid.cpp` — new.
- `Source/RA4Navigation/Public/RA4Navigation/Formation.h` / `Private/Formation.cpp` — new.
- `Source/RA4Navigation/Public/RA4Navigation/NavDebug.h` — new snapshot struct (pure data, no .cpp in sim).
- `Source/RA4Simulation/Private/SimWorld.cpp` — `SystemMovement` rewritten to use router + reservations + budget; `GetFlowField` keyed on sub-goal sector center.
- `Source/RA4Simulation/Public/RA4Simulation/SimTypes.h` — add `FormationId`/formation slot to `MovementComp`, repath budget config to `SimConfig.h`.
- `Source/RA4Tests/Private/TestNavigation.cpp` — add the 300-unit regression test + reservation/avoidance/formation unit tests.
- `Docs/ADR/0011-hierarchical-navigation-and-reservations.md` — new ADR.
- `Docs/Roadmap.md` — mark navigation stage done once tests pass.

## Data model

All new types live in `RA4Navigation` (engine-free, deterministic). The sim
references them through the existing `Nav::` namespace; no Unreal types.

### `ReservationGrid` — tile-keyed, one entry per tile

```cpp
struct ReservationCell {
    uint32_t OccupantSlot = kInvalidSlot;   // entity slot index (stable iteration order)
    TickIndex ExpiryTick = 0;               // 0 = free
};
class ReservationGrid {
    ReservationGrid(int32_t W, int32_t H);
    bool IsFree(const TileCoord&, TickIndex Now) const;
    // Reserve for the next k ticks. Returns false if a higher-priority occupant
    // (lower slot index, tie broken deterministically) already holds it.
    bool TryReserve(const TileCoord&, uint32_t Slot, TickIndex Now, int32_t HoldTicks);
    void Release(uint32_t Slot);            // clears all cells owned by Slot
    void Expire(TickIndex Now);              // sweep; O(tiles) but only run when Now changes
    void Snapshot(NavDebug& Out) const;
};
```

Stable tie-break: lower slot index wins. This is the only ordering rule and it is
deterministic.

### `MNavRouter` — hierarchical macro router

```cpp
struct MacroPath {
    std::vector<TileCoord> Waypoints;        // sector-center sub-goals, final = destination
    uint32_t BuiltTopologyRevision = 0;      // invalidates when grid topology changes
    NavQuery Query{};
};
class MNavRouter {
    MNavRouter(const NavGrid& Grid);
    // A* over sector portals from From→To. Emits at most MaxWaypoints sub-goals.
    // Deterministic: neighbor expansion order is fixed (sector id ascending),
    // tie-break on (g+h, sector id). Returns empty path if unreachable.
    MacroPath Find(const TileCoord& From, const TileCoord& To,
                   const NavQuery& Query, int32_t MaxWaypoints);
    void InvalidateAll();                     // called on topology revision bump
};
```

The router is cached per `(FromSector, ToSector, Query, TopologyRevision)` so 300
units in the same sector heading to the same destination share one macro path.
Cache eviction: LRU by `LastUsedTick`, cap configurable (default 128).

### `Formation` — slot assignment, deterministic

```cpp
struct FormationDef {
    ContentId Id;                             // so formations live in content data, not code
    std::vector<Vec2> Offsets;                 // relative to leader heading
};
struct FormationAssignment {
    ContentId FormationId;
    EntityId Leader;
    std::vector<EntityId> Members;           // slot i → Members[i]
};
```

Leader path drives the formation; members path to
`LeaderPos + Rotate(Offset[i], LeaderFacing)`.

### `MovementComp` additions (in `SimTypes.h`)

```cpp
struct MovementComp {
    Vec2 Destination;
    bool bHasDestination = false;
    Fixed CurrentSpeed = Fixed::Zero();
    Fixed ArriveRadius = Fixed::FromInt(30);
    int32_t BlockedTicks = 0;
    // --- new ---
    MacroPath CurrentMacroPath;
    int32_t NextWaypointIndex = 0;            // index into CurrentMacroPath.Waypoints
    TileCoord CurrentSubGoal;                // the sector center being followed
    ContentId FormationId;                    // ContentId::Invalid() if unassigned
    int32_t FormationSlot = -1;
    TickIndex LastRepathTick = 0;
};
```

### Repath budget config (in `SimConfig.h`)

```cpp
constexpr int32_t kMaxFlowFieldBuildsPerTick = 2;     // amortize the expensive part
constexpr int32_t kMaxMacroPathBuildsPerTick = 4;
constexpr int32_t kRepathBlockedTickThreshold = 60;   // 3s at 20Hz; reuse existing const
```

### `NavDebug` — pure data snapshot, no draw

```cpp
struct NavDebugSnapshot {
    uint32_t TopologyRevision = 0;
    std::vector<FlowDirection> FlowFieldSample;   // sparse: only dirty tiles
    std::vector<ReservationCell> ReservationSample;
    std::vector<TileCoord> BlockedTiles;
    std::vector<MacroPath> ActiveMacroPaths;      // for debug; capped
};
```

## Data flow — per-tick movement pipeline

`SystemMovement` is rewritten to the pipeline below. Everything runs in **stable
entity-slot order** (0 → HighWaterMark), single-threaded, deterministic. No per-unit
A\*; no per-unit flow-field build when units share a destination.

```
SystemMovement():
  ExpireReservationGrid(Now = CurrentTick)
  flowBuilds = 0; macroBuilds = 0

  for slot in 0..HighWaterMark:          # stable order = deterministic
    if !Core[slot].bAlive or Kind != Unit: continue
    M = Movements[slot]; T = Transforms[slot]; D = Content->Find(Core[slot].Def)

    # 1. Order → destination (from OrderQueue, unchanged from today)
    if M.bHasDestination == false: continue

    # 2. Macro path: get/build once per (sector, target, topology rev)
    fromTile = Map.WorldToTile(T.Position)
    toTile   = ResolveNavigationTarget(Map.WorldToTile(M.Destination), Query)
    if CurrentMacroPath stale or empty:
       if macroBuilds < kMaxMacroPathBuildsPerTick:
          M.CurrentMacroPath = Router.Find(fromTile, toTile, Query, MaxWaypoints=8)
          M.NextWaypointIndex = 0
          macroBuilds++
       else:
          # budget exhausted: follow last-known flow this tick, retry next tick
          goto STEER

    # 3. Sub-goal selection: next sector-center waypoint
    M.CurrentSubGoal = M.CurrentMacroPath.Waypoints[M.NextWaypointIndex]
    # advance waypoint when inside the sub-goal sector (within sector half-size)
    if SectorOf(fromTile) == SectorOf(M.CurrentSubGoal):
       M.NextWaypointIndex++ (clamp)

    # 4. Flow field for the sub-goal (shared across all units heading there)
    Field = GetFlowField(M.CurrentSubGoal, Query)   # cached; built only if missing
    if Field == null or !Field.IsReachable(fromTile):
       M.BlockedTicks++; continue
    if flowBuilds used this tick: counted inside GetFlowField via a passed budget

    # 5. Steering: flow direction → desired tile
    dir = Field.GetDirection(fromTile)
    desiredTile = fromTile + dir

    # 6. Reservation check (soft)
    nextTile = desiredTile
    if Reservation.TryReserve(desiredTile, slot, Now, HoldTicks=2):
       M.BlockedTicks = 0
    else:
       # 6b. Local avoidance: pick best open neighbor by flow-dir alignment
       best = null; bestScore = -inf
       for n in NeighborsInFixedOrder(desiredTile):   # N,E,S,W,NE,SE,SW,NW — fixed
          if Reservation.IsFree(n) and Grid.IsTraversable(n, Query):
             score = Dot(flowDir, n - fromTile)        # fixed-point dot
             if score > bestScore: best = n; bestScore = score
       if best != null:
          Reservation.TryReserve(best, slot, Now, 2)
          nextTile = best
          M.BlockedTicks = 0
       else:
          M.CurrentSpeed = 0; M.BlockedTicks++
          continue   # do not steer this tick

    # STEER (shared by both reservation-success branches): rotate hull toward
    # TileCenter(nextTile), accel, move
    rotate hull toward TileCenter(nextTile), accel, move

    # 7. Arrived check (existing) — uses ArriveRadius scaled by group size
    if DistSq(T.Position, M.Destination) <= ArriveRadius^2:
       M.bHasDestination = false; Reservation.Release(slot)

    # 8. Blocked fallback (existing threshold, now with repath)
    if M.BlockedTicks > kRepathBlockedTickThreshold:
       M.CurrentMacroPath.clear()    # forces a fresh macro path next tick
       M.BlockedTicks = 0
       M.LastRepathTick = Now
```

### Key determinism invariants this pipeline enforces

- One pass, ascending slot order — a unit can never read a tile a higher-slot unit
  reserved this tick before its own decision; ties resolved by slot index.
- `GetFlowField` returns the *cached* field or builds one only if the per-tick
  `flowBuilds` budget allows; otherwise returns the stale cached field
  (deterministic) and the unit follows it.
- Topology revision bump (building placed/bridge destroyed) invalidates all macro
  paths + flow fields; they rebuild lazily under budget over the next few ticks.
  During that window units keep moving on stale fields — acceptable and bounded.
- `ReservationGrid.Expire` runs once at tick start, not per-unit, so expiry is a
  single deterministic sweep.

### Formations

When an order targets a group with a `FormationId`, the leader's `MovementComp`
gets the macro path; members' `Destination` is set each tick to
`LeaderPos + Rotate(Offset[slot], LeaderFacing)`, and they run the same pipeline
with that dynamic destination. Members never build their own macro path — they
share the leader's sub-goals.

## Determinism & testing

### Determinism invariants (must not break)

1. **Single-threaded, fixed system order.** `SystemMovement` runs after
   `SystemOrders`, before `SystemCombat`, unchanged from the existing system order
   in `SimWorld.h`. No background threads touch sim state.
2. **Fixed entity-slot iteration.** All loops over `Core` use
   `for (slot = 0; slot < HighWaterMark; ++slot)`. No `std::unordered_map`
   iteration. No pointer-based ordering.
3. **Reservation tie-break = slot index.** Lower slot wins a contested tile. The
   only ordering rule, applied everywhere a tie can occur.
4. **Neighbor expansion order is fixed.** Macro A\* expands sector neighbors in
   ascending sector-id order; local avoidance scans neighbors in a fixed
   N,E,S,W,NE,SE,SW,NW order. Documented as a constant, not an accident.
5. **Repath budget is tick-bounded, not wall-clock.** `kMaxFlowFieldBuildsPerTick`
   and `kMaxMacroPathBuildsPerTick` are constexpr ints. A slow machine does *fewer*
   builds per tick, not different ones — it catches up over more ticks. Identical
   inputs → identical build sequence → identical checksum.
6. **Topology invalidation is lazy and budgeted.** A revision bump marks caches
   stale; they rebuild over subsequent ticks under the same budget. The transient
   state (units on stale fields) is deterministic because the field object is
   immutable once built — stale is still a valid, fixed field.
7. **`ReservationGrid.Expire` is a single sweep at tick start**, not per-unit, so no
   per-unit ordering leaks in.
8. **No `GetAllActorsOfClass`, no `FindObject` in the sim** (already true; this
   milestone adds none).

### Test plan

All in `Source/RA4Tests/Private/TestNavigation.cpp` unless noted.

| # | Test | Asserts |
|---|------|---------|
| T1 | `MacroRouterFindsShortestCorridor` | Two sectors, one portal; A\* returns 2 waypoints, deterministic across builds |
| T2 | `MacroRouterRespectsLayerAndClearance` | Portal masked out for wheeled; path empty for wheeled query, valid for tracked |
| T3 | `MacroRouterInvalidatesOnTopologyRevision` | Place building → revision bumps → cached path rejected |
| T4 | `FlowFieldSharedAcrossUnits` | 50 units to one rally point → `flowBuilds` counter == 1, not 50 |
| T5 | `ReservationLowerSlotWinsTie` | Two units target same tile same tick → lower slot occupies, higher waits |
| T6 | `ReservationExpiresAndFreesTile` | Reserve for 2 ticks; at tick+3 tile is free |
| T7 | `LocalAvoidancePicksBestOpenNeighbor` | Desired tile blocked by static obstacle; unit diverts to aligned open neighbor |
| T8 | `BlockedUnitRepatsAfterThreshold` | Unit wedged against wall; after `kRepathBlockedTickThreshold` ticks macro path cleared, new path found |
| T9 | `FormationMembersFollowLeaderSlot` | 8-unit formation; members arrive within slot offsets of leader, no member builds own macro path |
| T10 | `ThreeHundredUnitsRallyNoPerUnitAStar` | 300 units, one move order, map with a 1-tile chokepoint; assert `flowBuilds ≤ ceil(300/sectors_touched) + 1`, `macroBuilds ≤ sectors+1`, all 300 arrive within `T_budget` ticks, checksum stable across `-O3` vs `-O0+ASan` |
| T10b | `ThreeHundredUnitsDistinctDestinationsPerf` | 300 units, 300 distinct destinations; wall-clock per tick < 15 ms. Perf probe, not a determinism gate. |
| T11 | `RepathBudgetStallsDeterministically` | Force 10 simultaneous macro builds with budget=2 → builds split 2/2/2/2/2 across 5 ticks; checksum matches a run with budget=10 (different tick distribution, same final state) |
| T12 | `BridgeDestroyInvalidatesPath` | Unit mid-cross; destroy bridge → topology revision bump → unit repaths (or stops if unreachable) deterministically |
| T13 | `NavDebugSnapshotHasNoDrawDependency` | Snapshot serializes to bytes; runs headless with no `UWorld` in scope |

T10 is the **acceptance test** named in the roadmap. It must pass under both
optimization levels with the same checksum, same as the existing cross-build
determinism test in `TestVerticalSlice.cpp`.

### How "no per-unit A\*" is proven, not claimed

The test reads a `MovementStats` struct the sim exposes
(`FlowFieldBuilds`, `MacroPathBuilds`, `ReservationContests`), incremented only
inside `GetFlowField`/`Router.Find`/`Reservation.TryReserve`. The test asserts upper
bounds. If the assertion fails, the code is wrong, not the test.

### Build verification

Run `cmake --build build/headless -j8 && ./build/headless/RA4Tests --filter=Navigation`
and paste the actual pass/fail counts. If anything fails, fix it before declaring
the milestone done. No claiming "tests pass" without running them.

## Error handling & performance

### Error handling (deterministic-first — no exceptions in the sim)

- **Invalid/missing content def:** `Content->Find` returns null → unit skipped this
  tick (existing behavior, preserved). Logged via the existing `SimEvent`/
  diagnostics path, not a throw.
- **Unreachable destination:** `Field.IsReachable(fromTile)` false →
  `M.BlockedTicks++`; after threshold, macro path cleared and the unit stops (no
  infinite spin). The unit's order stays in the queue so a later topology change
  (bridge rebuilt) can re-path it.
- **Macro path empty (no portal path):** same as unreachable — `BlockedTicks`
  accrues, order stays. Distinct from "path valid but long."
- **Reservation grid out of bounds:** `TryReserve`/`IsFree` clamp to grid bounds;
  out-of-bounds tile → treated as impassable, not a crash. `TileCoord` already has
  no negative members (verify in impl), but clamp anyway.
- **Flow field build budget exhausted:** not an error — the unit follows its last
  valid cached field. Only an error if *no* field exists yet (first move) and budget
  is 0 that tick → unit waits one tick; gets a field next tick. Bounded latency,
  no crash.
- **Topology revision bump mid-tick:** caches marked stale at bump time; stale
  fields are still valid immutable objects, so a unit reading one this tick gets a
  consistent answer. Rebuilds happen over subsequent ticks under budget. No torn
  reads.
- **Formation with dead leader:** leader's slot is dead → members' `Destination`
  stops updating; they fall through to normal arrival logic and idle. No dangling
  references.
- **Budget misconfiguration (0 or negative):** clamped to ≥1 in config load. A zero
  budget would deadlock the sim; the clamp is a hard guard, not a preference.

### Performance targets (measured, not promised)

| Metric | Target | Measurement |
|--------|--------|-------------|
| 300-unit rally, one destination | < 5 ms / tick on the 20 Hz sim thread | `TestNavigation` T10 wall-clock via `std::chrono` around the tick |
| 300-unit rally, 300 distinct destinations (worst case) | < 15 ms / tick | T10b variant |
| Flow-field cache hit rate (shared destination) | > 95% | counter in `GetFlowField` |
| Macro-path cache hit rate | > 80% | counter in `Router.Find` |
| Per-tick reservation sweep | O(tiles) once, not per unit | profiler; no per-unit full sweep |
| Memory: reservation grid | 8 bytes/tile (slot uint32 + tick uint32) | 256×256 map = 512 KB |
| Memory: macro-path cache | ~64 paths × ~8 waypoints × 16 bytes | ~8 KB |

### Budget tuning rule

`kMaxFlowFieldBuildsPerTick` starts at 2 (one flow field is a BFS over a sector —
the expensive op). `kMaxMacroPathBuildsPerTick` starts at 4 (A\* over ~16–64 sectors
is cheap). These are in `SimConfig.h` as constexpr so a dedicated-server config or a
test can override them via the existing config mechanism — not hardcoded in the hot
loop.

### Anti-slop (will NOT do this milestone)

- No `DrawDebugLine`/`DrawDebugBox` in `RA4Navigation` or `RA4Simulation` — debug
  draw belongs to the presentation bridge stage. `NavDebug` is pure data.
- No background threads in the sim. "Async" = amortized across ticks.
- No per-unit `Find`/`GetAllActorsOfClass`. No `TArray` growth in the hot loop
  (vectors pre-sized).
- No floats in the sim path — all math stays `Fixed`/`Vec2`/`TileCoord` as today.
  The one existing `std::chrono` usage is test-only wall-clock, never feeds sim
  state.
- No leaving the 300-unit test "for later." T10 is the gate; milestone is not done
  until it passes.

## Rollout & verification gate

### Implementation order (each step ends compilable + tested)

1. **`ReservationGrid`** — header + impl + unit tests T5, T6. Build headless, run
   tests, pass.
2. **`MNavRouter`** — sector-portal A\* + LRU cache. Unit tests T1, T2, T3. Build,
   pass.
3. **`Formation`** — slot assignment + leader-follows logic. Unit test T9. Build,
   pass.
4. **`NavDebug` snapshot** — pure-data struct + `Snapshot()` methods on
   grid/router/reservations. Test T13 (serializes headless, no UWorld). Build, pass.
5. **Rewire `SystemMovement`** — integrate router + reservations + budget + local
   avoidance. Keep `GetFlowField` but key on sub-goal sector center; add
   `flowBuilds`/`macroBuilds` counters. Existing `TestVerticalSlice` must still pass
   unchanged (the full match loop). Then add T4, T7, T8, T11, T12.
6. **Acceptance test T10** — 300-unit rally with chokepoint. Must pass under `-O3`
   and `-O0+ASan` with **identical checksums**.
7. **ADR `0011`** — record the decisions (B/A/A), the invariants, the budget model,
   and why we rejected background threads. Update `Docs/Roadmap.md` to mark
   navigation done with the verified test counts.

### Verification gate (will run these and paste real output)

```bash
# Headless, optimized
cmake -S Tools/HeadlessBuild -B build/headless -DCMAKE_BUILD_TYPE=Release
cmake --build build/headless -j8
./build/headless/RA4Tests --filter=Navigation        # T1–T13
./build/headless/RA4Tests                           # full suite — must stay green

# Cross-build determinism proof
cmake -S Tools/HeadlessBuild -B build/asan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build/asan -j8
./build/asan/RA4Tests --filter=Navigation
# T10 checksum from -O3 run == T10 checksum from -O0+ASan run
```

### Definition of done (all must be true, no exceptions)

- [ ] Every test T1–T13 passes under `-O3`.
- [ ] T10 passes under `-O0+ASan/UBSan` with the same checksum as `-O3`.
- [ ] Full existing suite (the 56 tests + new ones) is green; nothing regressed.
- [ ] `Roadmap.md` updated with verified counts and the date.
- [ ] ADR `0011` written and committed.
- [ ] No `DrawDebug*`, no background threads, no floats added to the sim path.
- [ ] `RedAlert4.uproject` / `.Build.cs` untouched (no Unreal compile this milestone
      — that is stage C, deferred).

### What "done" does NOT mean

- Not a visual debug overlay in the editor (presentation stage).
- Not networked movement (networking stage).
- Not AI pathing (AI stage).
- Not naval/air movement systems (later).