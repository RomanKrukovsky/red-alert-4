# ADR-0026: Unreliable Intelligence Layer (Perceived World)

**Status**: Accepted — M0 (skeleton) implemented on `feat/intel-unreliable`
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
