# ADR-0021: Knowledge Map — Per-Player Belief State with Intel Decay

**Status**: Proposed (blocked until remediation/foundation-fixes is green; no implementation authorized)
**Depends on**: ADR-0001 (60Hz lockstep), ADR-0002 (pure C++ sim), ADR-0004 (state hashing), ADR-0008 (AI zero-cheat fog compliance)

## Context

The current fog of war is binary per cell: hidden / explored / visible. The product direction "война восприятия" (perception warfare) requires that a player sees **their belief about the battlefield**, not the objective battlefield:

- previously seen entities remain displayed at their last known position;
- intel about an entity ages: exact counts degrade to ranges, positions degrade to areas;
- stale data is visually distinguishable (confidence value drives presentation);
- reconnaissance produces *reports with confidence*, not omniscient reveals.

The naive approach — implement this in the presentation layer ("just draw ghosts") — violates the project's own requirements: the AI Commander must play by the same rules as the player (zero-cheat), and AI lives in the deterministic sim. Therefore belief state must be simulation state.

## Decision

### 1. KnowledgeMap is deterministic simulation state

Each player (human or AI) owns a `KnowledgeMap` inside `RA4Simulation`:

- Fixed-point / integer only (INVARIANT 2). Confidence is `uint8` 0–255, not float.
- Updated exclusively inside `CommandBus::DispatchTick` (INVARIANT 4).
- Included in the 64-bit state hash (INVARIANT 5) — belief divergence is a desync.

### 2. Data model

```
struct IntelRecord {
    EntityId    Subject;          // 0 = unresolved contact
    uint32      LastConfirmedTick;
    FixedVec2   LastKnownPos;
    uint16      ObservedArchetype; // what the observer THINKS it is (deception hook)
    uint8       Confidence;        // 255 = currently in sensor range
    uint8       SourceType;        // visual / radar / report / inference
};
```

`KnowledgeMap` = spatial grid of cell-level intel (terrain, structures) + entity-level `IntelRecord` table. Memory budget: entity records are bounded by (players × max entities); cell layer reuses the existing fog grid resolution.

### 3. Decay rules (deterministic, tick-driven)

- Confidence decays on a fixed schedule per source type (e.g. visual contact: −1 per 30 ticks after loss of contact; radar: faster).
- Position uncertainty radius grows stepwise with decay; UI renders range/area instead of a point below defined thresholds.
- Records below a floor confidence are garbage-collected deterministically (same tick on all peers).
- Decay evaluation is amortized (1/N of records per tick, round-robin by index) to bound per-tick cost.

### 4. Contract with other layers

- **Presentation** renders player P strictly from `KnowledgeMap[P]` snapshot. It never reads objective `SimWorld` entity positions for enemy units. (Extends INVARIANT 3.)
- **AI Commander** reads only its own `KnowledgeMap` — this *replaces* the current ad-hoc fog queries and makes zero-cheat structural instead of disciplinary.
- **UI** shows confidence explicitly ("24 tanks, confidence 61%") — UI remains a view, not a source of truth.
- **Replay/saves**: KnowledgeMap serializes with the rest of sim state; format version bump required (SAVE_AND_REPLAY.md migration path).

### 5. Invariants introduced

- K1: No system outside `RA4Simulation` may write to a KnowledgeMap.
- K2: A player's render path must be reproducible from (replay, playerId) alone — "what did the player believe at tick T" is a first-class replay query.
- K3: Objective truth is never leaked to presentation for non-owned entities; leak = test failure, not code review finding.

## Consequences

**Positive**: makes Deception (ADR-0023) and honest AI structurally possible; unique selling point; belief state becomes testable ("after 3000 ticks without contact, confidence ≤ X").

**Negative / risks**:
- Sim state grows ~O(players × entities); hash cost grows accordingly — needs a performance budget entry before implementation.
- Every existing system that queried "is visible" must migrate to "what do I believe" — a wide but mechanical refactor.
- UX risk: stale-intel rendering can read as a bug. Requires explicit art/UI language (desaturation, timestamp badges) — UI_UX_BIBLE section required before implementation.

## Verification plan (Definition of Done gate)

1. Unit tests: decay determinism (same seed + commands ⇒ identical KnowledgeMap hash on 2 sim instances).
2. Regression test: AI receives zero information not present in its KnowledgeMap (instrumented leak detector in test build).
3. Replay test: belief-view reconstruction from replay matches live run.
4. Performance test: 500 entities × 4 players, decay + hashing within tick budget (budget TBD in PERFORMANCE_BUDGETS.md).
