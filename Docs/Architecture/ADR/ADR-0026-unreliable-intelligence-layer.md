# ADR-0026: Unreliable Intelligence Layer (Perceived World)

**Status**: Accepted for M0 — **but two K-invariant violations found in independent review (2026-08-05)
must be fixed before M1.** See "Independent review findings" at the end of this document.
**Date**: 2026-08-05
**Depends on**: ADR-0001 (20 Hz lockstep), ADR-0002 (pure C++ sim), ADR-0003 (data-oriented ECM), ADR-0004 (state hashing), ADR-0005 (replay/checkpoints), ADR-0009 (JSON content)
**Relates to**: ADR-0021 (Knowledge Map — this ADR is its concrete implementation decision record; ADR-0021 stays as the design-direction document), ADR-0011 (DirectControl), ADR-0008 (AI commander)

## Context

The perception-warfare direction (PERCEPTION_WARFARE_DIRECTION.md) requires that a
player commands their *belief* about the battlefield, not the battlefield itself.
Reports from units arrive late, distorted and with motivated bias; the HQ map is
built exclusively from those reports. An external feature specification
("Unreliable Intelligence Layer", targeting UE 5.8) was evaluated against this
codebase; several of its structural mandates conflict with existing, load-bearing
architecture decisions. This ADR records what was adopted, what was rejected, and
why.

## Decisions

### 1. Rejected: Mass Entity Framework as the substrate

The external spec mandated Mass fragments and `UMassProcessor` pipelines. Rejected
because the simulation already lives in a headless, engine-free C++ core
(ADR-0002, ADR-0003) that compiles under `Tools/HeadlessBuild` with no Unreal at
all — the determinism CI job depends on that. Mass would (a) break the headless
build and the Linux server path, (b) subordinate tick order to Mass scheduling
outside our checksum coverage, (c) create exactly the "parallel duplicate
subsystem" our rules forbid. The *goals* behind the Mass mandate (no actors per
unit, no per-item Tick, dense data) are already structural properties of SimWorld.

**Implementation**: new engine-free module `Source/RA4Intel` (static lib in the
CMake harness, Runtime module in the .uproject), orchestrated as one system
(`SystemIntel`) inside `SimWorld::Tick`, immediately after `SystemFogOfWar`.

### 2. Threat model: interface secrecy, not memory secrecy

The spec demanded "ground truth never reaches the client". Under lockstep
(ADR-0001) every client simulates full GT locally by definition; that guarantee is
unachievable without abandoning lockstep. Adopted model **(а)**: the perceived
world is the *only interface* through which UI, input and (from M6) AI may read
enemy information. GT remains in client memory but `PerceivedTrack` carries no
`EntityId`; the track↔entity association lives in a core-internal table. We defend
against leaks through the interface, not against a memory scanner — the same
posture the fog of war already has.

### 3. Belief state is deterministic, checksummed simulation state

`PerceivedWorld` (per player) and in-flight reports serialize with the match
(save version 2→3, with a v2 migration path) and feed `ComputeStateChecksum`.
Rationale: once the AI plays from belief (M6), belief influences commands, so a
divergent belief is a real desync and must be caught on the tick it happens by the
existing lockstep machinery (ADR-0004).

### 4. Isolated RNG stream

Intel draws come from a dedicated `Random IntelRng` seeded from the match seed
with a distinct sequence constant, not from the main `Rng`. Otherwise every intel
draw would shift the draw sequence of existing systems and invalidate all
pre-intel replays at once. Fixed-point only (48.16), per ADR-0002; the external
spec's `FRandomStream`/float math is rejected for cross-platform determinism.

### 5. Configuration via JSON content, not UDataAsset

All tunables live in `Content/RA4/Data/Intel/intel_settings.json` (distortion
profiles, confusion matrix, comms profiles, track tuning), loaded and validated by
`RA4Intel` (ADR-0009). Fractions are converted to per-mille integers exactly once
at load; no double reaches sim state. The validator fails loudly on authoring
mistakes (confusion rows must sum to exactly 1.0, phantom lifetime must be
bounded, etc.). `UDeveloperSettings`/CVars are reserved for presentation-side
debug toggles that cannot affect sim results.

### 6. Kill switch semantics

`"enabled": false` (the shipped default until M2) means the layer is genuinely
absent: no per-player worlds are allocated, `SystemIntel` returns immediately,
serialization writes one flag, and simulation results are bit-identical to a build
without the module (pinned by test `Intel.DisabledLayerDoesNotChangeSimulationResults`).
A save records its enabled-ness; loading across a mismatch is refused.

### 7. Scope of unreliability (M0–M5)

Unreliable intel covers *enemy and neutral* entities. Own units remain selectable
and commandable by true position — DirectControl (ADR-0011) and the existing input
path stay intact. Self-report bias (M4) affects only informational summaries about
own forces, not selection. Moving own-unit interaction onto belief state is a
separate future ADR if playtests warrant it.

### 8. Contested reports: flag, not hypothesis split

When independent sources disagree, the track gets `bContested = true` and a
widened count interval / error radius; it does not split into competing hypothesis
tracks. Splitting doubles state, complicates phantom refutation and the UI for the
same gameplay signal. The track structure permits revisiting this later.

### 9. AI plays from belief (M6)

`AICommander` currently scans `SimWorld` directly. At M6 it switches to
`GetIntel().GetPerceivedWorld(aiPlayer)`, making zero-cheat structural (the intent
already recorded in ADR-0021). Until M6 the AI reads GT — a documented, temporary
unfairness.

## Milestones

M0 skeleton (this ADR) → M1 truthful pipeline (PS≡GT) → M2 distortion stages 1–5 +
two-map debug overlay → M3 chain of command, delays, aggregation, blackout →
M4 fabrication + self-report bias + guaranteed phantom refutation → M5 profiling
against budgets → M6 AI on belief. Each milestone: tests green, independent
review, atomic commit, merge to main.

## Performance budgets (owner: M5)

- All intel phases combined: ≤ 0.8 ms per sim tick at 5000 entities (assumed
  target; PERFORMANCE_BUDGETS.md entry pending real unit-count targets).
- Zero steady-state allocations in the per-tick path.
- ≤ 16 MB per player's perceived world at 4096 tracks (hard cap enforced by
  `MaxTracksPerPlayer`; allocation refuses beyond it).

## Consequences

**Positive**: unique gameplay pillar lands on existing deterministic
infrastructure; belief becomes testable and replayable ("what did the player
believe at tick T"); AI zero-cheat becomes structural.

**Negative / risks**: save format bump (v3) — v2 saves load only into
intel-disabled sessions; state and hash cost grow with track counts (bounded by
hard cap); UX risk of perceived unfairness until the M5 post-match report exists —
mitigated by per-stage disable flags and the fabrication master switch.

## Verification (M0, actually executed)

- Headless build (`Tools/HeadlessBuild`, clang, `-Wall -Wextra -Werror
  -Wconversion -Wsign-conversion`): clean.
- Full suite: 331 passed, 0 failed (317 baseline + 14 new `Intel.*` tests
  covering config load/validation, kill switch bit-equality, generational track
  handles, hard cap, region query, negative knowledge, serialization round-trip,
  SimWorld save/load with intel enabled, cross-enabledness load refusal,
  two-instance lockstep drift check).


---

## Independent review findings (2026-08-05)

An independent reviewer that authored neither this ADR nor ADR-0021 compared the two against the M0
code. Verdict: **divergences are partly undocumented**. The two BLOCKER items were re-verified
directly in the headers and are real.

### BLOCKER 1 — ground-truth flag inside the player-facing struct

`PerceivedTrack::bPhantom` is commented "core-internal truth flag, never shown to UI"
(`IntelTypes.h:120`) but it sits in the very struct handed out by
`PerceivedWorld::GetTracksInRegion(... std::vector<const PerceivedTrack*>&)` (`PerceivedWorld.h:49`).
Any UI, presentation or AI caller holding a track can read whether that contact is fabricated. This
defeats K3 and this ADR's own threat model: the comment is a convention, and a convention is not an
invariant.

**Required fix**: remove `bPhantom` — and any other ground-truth field — from `PerceivedTrack`. Keep it
in a core-internal side table beside the track↔entity association, which already never leaves the
simulation. Add a leak-detector test that fails the build if a ground-truth field is reachable from the
read surface (this is also ADR-0021 verification item 2, currently unimplemented).

### BLOCKER 2 — K1 is not structural

ADR-0021 K1 states that nothing outside the simulation may write to belief state. Two public members
contradict it: `IntelSystem::GetPerceivedWorldMutable(PlayerId)` (`IntelSystem.h:80`) and
`PerceivedWorld::SetLastObservedTick(...)` (`PerceivedWorld.h:56`). Anything holding a reference to the
system can mutate another player's belief.

**Required fix**: move both behind a private writer interface with `IntelSystem` as a friend, or split
the read surface into a separate const-only view type. Then record K1/K2/K3 in
`Docs/Architecture/INVARIANTS.md` — they currently exist only inside ADR-0021 prose, which is why they
were implementable-around.

### Undocumented divergences from ADR-0021 (MAJOR)

| ADR-0021 intent | M0 reality | Consequence |
| :--- | :--- | :--- |
| `SourceType` (visual / radar / report / inference) on each record | Absent; replaced by `IndependentSourceCount` (a count, not a type) | ADR-0021's per-source-type decay ("radar decays faster than visual") is **unimplementable** — `TrackTuning` has a single global `ConfidenceDecayPerSecondPerMille` |
| Amortized round-robin decay, 1/N of records per tick | `PhaseTrackUpdate(TickIndex)` is empty | Honest as an M0 skeleton, but there is no cursor and no batch-size config, so the requirement has fallen out of the design surface rather than being scheduled |
| Cell-level intel layer (terrain, structures) | Only `LastObserved` per tile | No cell-level belief about terrain or buildings |
| `Confidence` as `uint8` 0–255, "not float" | `Fixed Confidence` | Satisfies INVARIANT 2, contradicts ADR-0021's letter |
| `LastConfirmedTick` | `LastUpdateTick` | Semantic shift: an update from an unreliable or fabricated report is not a confirmation |
| UI shows an exact count ("24 tanks, confidence 61%") | `BelievedCountMin/Max` interval by design | Direct contradiction; the interval is the better design, but ADR-0021 was never amended |

Also: this ADR depends on "ADR-0001 (20 Hz)" while ADR-0021 said 60Hz — the 20 Hz figure is the correct
one and the rest of the documentation has since been corrected to match, with an erratum on ADR-0001.
The update point moved from ADR-0021's "exclusively inside `CommandBus::DispatchTick`" to `SystemIntel`
within `SimWorld::Tick` after `SystemFogOfWar`, without a record. Terminology diverged wholesale
(`KnowledgeMap`/`IntelRecord` → `PerceivedWorld`/`PerceivedTrack`) with no mapping table.

### Status of ADR-0021

ADR-0021 remains **Proposed** and is superseded in part by this document. It must gain an explicit
rejection log covering every row of the table above. Until then the two ADRs mislead any reader who
takes ADR-0021 as current. Tracked as NEXT_ACTIONS P-2.
