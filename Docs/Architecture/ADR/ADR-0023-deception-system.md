# ADR-0023: Deception System — Signatures, Decoys and Information Attacks

**Status**: **ACCEPTED 2026-08-05** (product owner, after independent review APPROVE-WITH-CHANGES and
application of all required changes: corrected scope estimate, ADR-0022 dependency, phantom
physicality rules, fabrication boundary). Implementation gated on its hard dependency: ADR-0021/0026
intel milestones through I-M4 (fabrication plumbing) must exist first.
**Depends on**: ADR-0021 (Knowledge Map — hard dependency), ADR-0022 (Command Network — `EmissionLevel` feeds CommandGraph detection, and jamming interacts with signature masking), ADR-0009 (data-driven content), ADR-0008 (AI zero-cheat)

## Context

With a per-player belief state (ADR-0021), deception becomes representable honestly: a decoy is not a UI trick but an entity whose **signature** causes observers to record a false `ObservedArchetype` / false confidence in their KnowledgeMap. Without ADR-0021 this system is unimplementable without cheating, therefore it is strictly sequenced after it.

## Decision

### 1. Signature model

Every sim entity carries a `SignatureProfile` (data-driven JSON):

```
SignatureProfile {
    VisualArchetype;    // what optics report
    RadarArchetype;     // what radar reports
    ThermalMagnitude;   // affects detection range
    EmissionLevel;      // radio noise; feeds CommandGraph detection too
}
```

Observation resolves through the observer's sensor type against the target's profile and writes the *resolved* archetype into the observer's IntelRecord — not the true one. Truth is recoverable only by higher-grade sensors or sustained observation (confidence threshold flips the record to true archetype; thresholds are deterministic integers).

### 2. Deception content (v1 scope, deliberately narrow)

Only three deception tools ship in the first iteration — each maps to one signature axis:

1. **Decoy structure** — cheap building broadcasting a false RadarArchetype (e.g. superweapon signature). Dies in one hit; sustained visual contact exposes it.
2. **Phantom column** — projector unit emitting N fake visual contacts that move on scripted paths;
   contacts have low max confidence and never survive close observation.
   **Physicality rules (review P-9)**: phantom contacts are *belief-only* — they occupy **no**
   navigation cells, **no** reservation slots, and do **not** count against entity caps or the
   soft command limit (ADR-0014). They exist as entries in observers' perceived worlds, nothing
   else; enemy units path *through* a phantom's believed position unimpeded, which is itself one of
   the exposure mechanics. The projector unit is an ordinary entity and counts normally.
   **Boundary with ADR-0026 fabrication (review P-9)**: ADR-0026's fabrication stage models
   *unintentional* false contacts (a panicking observer inventing tanks); this ADR models
   *deliberate, player-purchased* deception. They share the phantom-track plumbing (a fabricated
   track is a fabricated track), but their sources never mix: fabrication rates come from observer
   psychology tuning, deception contacts come from a projector entity with an owner, a cost and a
   counter. ADR-0026's guaranteed-refutation rule (`MaxPhantomLifetimeTicks`) applies to both.
3. **Signature masking** — module (via unit modification path) lowering EmissionLevel/ThermalMagnitude at an energy cost.

Explicitly out of v1 (recorded for later ADRs, not to be smuggled in): fake resource counts, fake attack notifications, icon substitution in enemy UI, forged radio chatter. These touch UI-truthfulness and accessibility concerns and each needs its own decision.

### 3. Counterplay is mandatory

Design rule: every deception tool must have at least two counters (sensor grade, time-under-observation, cross-referencing multiple source types). The Knowledge Map UI surfaces disagreement between sources ("radar says tank column, optics see nothing — confidence 30%"). A deception with no counter is a design defect.

### 4. AI symmetry

The AI Commander both *falls for* deception (it reads only its KnowledgeMap, ADR-0021 K-invariants) and *may use* deception through the same command interface as the player. No AI-only truth channel, no AI-only decoy discount.

### 5. Determinism

All resolution (sensor vs signature, confidence thresholds, phantom paths) is fixed-point/integer inside the tick. Phantom "randomness" comes from the sim's seeded deterministic RNG stream.

## Consequences

**Positive**: transforms scouting into an information game.

**Scope correction (review P-9)**: the original claim that v1 tools are "pure data + KnowledgeMap
writes — no new engine systems" was an underestimate and is withdrawn. Resolving each observer's
sensor grade against each target's `SignatureProfile` is a **new sensor-resolution model** inside the
intel observation phase, with O(observers × targets) worst-case cost per sensor pass — i.e. a rework
of ADR-0026's observation stage, not a data drop-in. What remains cheap is the *content*: once the
resolution model exists, each deception tool is data. Budget impact goes to PERFORMANCE_BUDGETS §4.1
(the intel row absorbs sensor resolution; it needs its own measured sub-line at P-7 time).

**Negative / risks**:
- Frustration risk: being deceived must feel like *my scouting failed*, not *the game lied*. Mitigation: all deceptions are exposable, confidence is always displayed, post-match documentary (future) reveals what was real.
- Balance surface expands multiplicatively with faction asymmetry — v1 tools should be shared-tech before faction-specific variants.
- Test surface: needs scenario tests, not just unit tests.

**Additional design risks (review P-12)**:
- **Spectator readability under mass fakes**: a screen full of phantom contacts is unreadable for
  casters and replays. Rule: the spectator/objective view renders ground truth with deceptions
  *marked* (distinct silhouette), never the deceived view by default; a caster can toggle into any
  player's perceived view. Interacts with ADR-0010's delay buffer — marked deceptions reveal
  nothing actionable at 3+ minutes delay.
- **Transparency-of-counters teaching gaming**: documenting "every deception has two counters" also
  documents how to bait counters. Accepted: that is the intended mind-game layer, and it is
  symmetric.

## Verification plan

1. Unit tests: sensor-vs-signature resolution matrix, deterministic across instances.
2. Scenario test: decoy structure appears as superweapon in enemy KnowledgeMap until direct visual contact for T ticks.
3. AI test: AI commits an attack against a phantom column under defined conditions AND stops falling for it after exposure (belief update works).
4. Zero-leak regression from ADR-0021 remains green with deception entities present.
