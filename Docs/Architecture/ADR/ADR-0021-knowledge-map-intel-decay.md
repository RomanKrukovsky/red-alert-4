# ADR-0021: Knowledge Map — Per-Player Belief State with Intel Decay

**Status**: **Superseded in part by ADR-0026** (which is the authority on implemented behaviour).
Independent review completed 2026-08-05; see the rejection log at the end of this document. The
un-implemented parts (cell-level intel layer, per-source-type decay, amortized decay) remain Proposed.
**Depends on**: ADR-0001 (fixed-tick lockstep, 20 Hz), ADR-0002 (pure C++ sim), ADR-0004 (state hashing), ADR-0008 (AI zero-cheat fog compliance)

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
    Vec2        LastKnownPos;      // RA4::Vec2 (fixed-point), see RA4Core/Vector.h
    uint16      ObservedArchetype; // what the observer THINKS it is (deception hook)
    uint8       Confidence;        // 255 = currently in sensor range
    uint8       SourceType;        // visual / radar / report / inference
};
```

`KnowledgeMap` = spatial grid of cell-level intel (terrain, structures) + entity-level `IntelRecord` table. Memory budget: entity records are bounded by (players × max entities); cell layer reuses the existing fog grid resolution.

### 3. Decay rules (deterministic, tick-driven)

- Confidence decays on a fixed schedule per source type (e.g. visual contact: −1 per 10 ticks (0.5 s at 20 Hz) after loss of contact; radar: faster).
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

**Positive**: makes Deception (ADR-0023) and honest AI structurally possible; unique selling point; belief state becomes testable ("after 1200 ticks (60 s at 20 Hz) without contact, confidence ≤ X").

**Negative / risks**:
- Sim state grows ~O(players × entities); hash cost grows accordingly — needs a performance budget entry before implementation.
- Every existing system that queried "is visible" must migrate to "what do I believe" — a wide but mechanical refactor.
- UX risk: stale-intel rendering can read as a bug. Requires explicit art/UI language (desaturation, timestamp badges) — UI_UX_BIBLE section required before implementation.

## Verification plan (Definition of Done gate)

1. Unit tests: decay determinism (same seed + commands ⇒ identical KnowledgeMap hash on 2 sim instances).
2. Regression test: AI receives zero information not present in its KnowledgeMap (instrumented leak detector in test build).
3. Replay test: belief-view reconstruction from replay matches live run.
4. Performance test: 500 entities × 4 players, decay + hashing within tick budget (budget TBD in PERFORMANCE_BUDGETS.md).


---

## Rejection log — what ADR-0026 changed (independent review, 2026-08-05)

ADR-0026 is the implementation decision record for this design. Where the two differ, **ADR-0026 wins on
implemented behaviour and this document states intent only.** Every divergence found by independent
review is listed here so no reader mistakes this document for current.

| This ADR specified | ADR-0026 / M0 did instead | Disposition |
| :--- | :--- | :--- |
| `IntelRecord` with `SourceType` (visual/radar/report/inference) | `PerceivedTrack` with `IndependentSourceCount` — a count, not a type | **RESOLVED 2026-08-05 (I-B3, architecture decision)**: `SourceType` is restored — as a `uint8` enum on `Observation` and a `DominantSourceType` on `PerceivedTrack` — because three consumers need it regardless of decay: UI source attribution (UI_UX_BIBLE §1.4), ADR-0023's sensor-axis resolution (visual/radar/thermal are its three signature axes), and the contested-source readout. **Decay itself stays a single curve**: M0's `Clarity` scalar plus the per-category confusion matrix already model sensor quality at observation time, which is where the difference belongs; a per-type decay curve would duplicate that influence at track time. `TrackTuning` gains `DecayMultiplierPerSourcePerMille[]` defaulting to identity (1000), so per-type decay becomes a data-tuning option without a code change if playtests demand it. Implementation lands with I-M1 (the field is part of the truthful pipeline's vocabulary). |
| `Confidence` as `uint8` 0–255, explicitly "not float" | `Fixed Confidence` (0..1) | **Accepted change.** `Fixed` satisfies INVARIANT 2 and is more natural in this codebase. This ADR's insistence on `uint8` was an over-specification. |
| `FixedVec2 LastKnownPos` | `Vec2 BelievedPosition` + `PositionErrorRadius` | **Accepted change and an improvement** — an explicit error radius is better than an implied one. |
| `LastConfirmedTick` | `LastUpdateTick` | **Accepted with a caveat.** The rename is a real semantic shift: a track touched by an unreliable or fabricated report has been *updated*, not *confirmed*. Any rule that meant "confirmed" must be re-read. |
| UI shows an exact count: "24 tanks, confidence 61%" | `BelievedCountMin`/`BelievedCountMax` — "count is an interval, never one number" | **Accepted change; this ADR was wrong.** An interval is more honest than a false-precision integer. Section 4's example is superseded; GDD section 8 documents the interval presentation. |
| Cell-level intel layer (terrain, structures) | Only `LastObserved` tick per tile | **Deferred, not rejected.** No cell-level belief exists yet. |
| Amortized round-robin decay, 1/N records per tick, deterministic GC | `PhaseTrackUpdate()` is empty in M0 | **Deferred to M1/M2.** Honest for a skeleton, but neither a round-robin cursor nor a batch-size parameter exists in `TrackTuning`, so the requirement currently has no home in the design. Add both, or record explicitly that a full sweep per tick is affordable within budget. |
| Updated exclusively inside `CommandBus::DispatchTick` | `SystemIntel` inside `SimWorld::Tick`, after `SystemFogOfWar` | **Accepted change.** Equivalent determinism guarantee; the phrasing here was too narrow. INVARIANT 4 uses the same wording and has the same imprecision. |
| 60Hz tick assumptions throughout | 20 Hz (`kTicksPerSecond`) | **This ADR was wrong**, along with most of the documentation. Corrected here and across nine documents; erratum added to ADR-0001. |
| Naming: `KnowledgeMap`, `IntelRecord`, `RA4Simulation` owns it | `PerceivedWorld`, `PerceivedTrack`, `RA4Intel` module | **Accepted change.** Read this document's names as referring to the ADR-0026 types. |

### Invariants K1–K3: not yet honoured

Independent review found that K1 and K3 are violated by the M0 read surface — `bPhantom` (a
ground-truth flag) is a member of the struct handed to UI callers, and
`GetPerceivedWorldMutable` / `SetLastObservedTick` are public. Details and required fixes are in
ADR-0026's review section. **K1–K3 must also be promoted into
`Docs/Architecture/INVARIANTS.md`**: while they live only in this document's prose they are advice, not
invariants, which is precisely how they came to be implemented around.

Verification items 2 (instrumented leak detector) and 3 (belief-view reconstruction from replay, i.e.
K2) from section "Verification plan" are **not** among M0's 14 tests and are not listed as deferred
anywhere. They gate M1.
