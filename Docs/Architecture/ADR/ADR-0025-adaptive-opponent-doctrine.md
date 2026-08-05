# ADR-0025: Adaptive Opponent — Cross-Match Player Modeling and Doctrine Counter-Selection

**Status**: **ACCEPTED 2026-08-05** (product owner, after independent review APPROVE-WITH-CHANGES and
application of all required changes: canonical encoding, extension-not-duplication statement, twin
claim withdrawn). Implementation sequenced LAST among the five systems (needs the replay analyzer
tooling and benefits from systems 1-4 as doctrine levers).
**Depends on**: ADR-0008 (HTN/utility AI), ADR-0020 (economic telemetry & balance metrics — recovered
from the non-compiling remediation branch, see its provenance note), ADR-0021 (Knowledge Map),
ADR-0005 (replay format — the analyzer reads `Source/RA4Replay/Public/RA4Replay/Replay.h`)

## Context

Direction: the AI opponent learns the *player's habits* between matches — heavy air usage ⇒ layered AA next match; habitual right-flank routes ⇒ mines on those routes; early-rush preference ⇒ bait-base openings. Constraint from the concept and from fairness: the AI gets **no unfair information or resources** — it adapts doctrine selection, not knowledge of the current match.

Hard architectural constraint: the sim is deterministic. Any learning artifact that influences a match must be an **input** to the match, fixed before tick 0 — never something mutating during the match from out-of-sim data.

## Decision

### 0. Relationship to existing RA4AI types (review P-11)

`RA4AI` already contains `OpponentModel` (`OpponentProfile` — the in-match estimate of the enemy
built from the AI's own scouting), `AIDoctrine` (`AIDoctrineType`, `AIPersonality`,
`FactionDoctrineDef`, `AIDoctrineRegistry` — the static per-faction doctrine definitions) and
`AIStrategy`. **This ADR extends those types; it does not duplicate them** (a parallel subsystem is
forbidden by project rules):

- `PlayerProfile` (new, out-of-sim) is the *cross-match* analogue of the in-match `OpponentProfile`.
  They are different lifetimes of the same idea and must share field vocabulary where they overlap
  (army-composition ratios, timing histograms) so the analyzer and the in-match estimator do not
  drift apart.
- `DoctrineBias` does not define doctrines. It **reweights the selection** among doctrines already
  registered in `AIDoctrineRegistry` and shifts `AIPersonality` scalars within clamps. If a desired
  bias cannot be expressed as a reweight of an existing `FactionDoctrineDef`, the answer is a new
  doctrine definition in the registry — not a bypass channel.
- The utility priors in `ThreatPriors[]` land in the same utility inputs `AIStrategy` already
  consumes; no second scoring path is added.

### 1. Two-loop separation

- **In-match loop (existing)**: AICommander plays from its KnowledgeMap with utility/HTN. Unchanged, deterministic.
- **Between-match loop (new, out-of-sim)**: a `PlayerProfile` is updated from **replays** after each match, and compiles into a `DoctrineBias` blob consumed by the AI at match start.

### 2. PlayerProfile (local, transparent)

- Built by a post-match analyzer over the replay command stream + telemetry (ADR-0020): opening build-order class, army composition ratios over time, attack timing histogram, preferred approach vectors per map, reaction patterns (does the player scout? does the player react to harass?).
- Stored locally per user profile as versioned JSON. **Privacy/product rules**: viewable by the player in-game ("Что противник знает о вас"), resettable, and OFF by default in ranked multiplayer contexts (applies to vs-AI modes).

### 3. DoctrineBias — the only channel into the sim

```
DoctrineBias {
    ProfileVersion;
    OpeningWeights[];        // reweights AI opening selection
    ThreatPriors[];          // e.g. expected air-heavy ⇒ AA utility prior up
    MapRoutePriors[mapId][]; // mined/watched approach weights
    BaitTacticsEnabled;      // unlocked only above habit-strength threshold
}
```

- Passed as match-setup data (like difficulty), hashed into match seed material, recorded in the replay header ⇒ replays remain exactly reproducible.
- **Canonical encoding (review P-11)** — required for cross-platform hash identity:
  - All weights and priors are `RA4::Fixed`, never float, serialized via the existing `ByteWriter`
    fixed-width path.
  - `MapRoutePriors` and every other keyed table serialize in **ascending key order** (mapId as
    uint64, route index ascending); iteration during play also follows that order, so behaviour
    never depends on hash-map layout.
  - **Unknown `mapId`** at load: the entry is dropped and counted in a `DroppedEntries` field that
    itself feeds the header hash — two peers disagreeing about what was dropped is a desync at tick
    0, not a silent divergence mid-match.
  - **Version skew**: a reader encountering `ProfileVersion` *older* than its own applies documented
    per-version upgrades (same discipline as save migration); encountering a *newer* version, it
    refuses the blob and starts unbiased — never a partial read. Refusal is recorded in the replay
    header.
  - Blob cap 64 KB enforced at write time; an over-cap profile is truncated by dropping
    lowest-weight entries in canonical order, deterministically.
- Bounded influence: biases reweight existing utility scores within clamped ranges; they cannot grant resources, vision, or new capabilities (zero-cheat preserved).

### 4. "Digital twin" (stretch, same architecture)

After sufficient matches, a DoctrineBias can be compiled *from* the player's own profile ⇒ an AI that plays approximately like the player. **Correction (independent review 2026-08-05): this is not a free by-product.** Reweighted utility
scores reproduce a player's *preferences*, not their play: imitating a style needs behavioural fidelity —
action sequencing, timing, micro habits — which is a separate system and warrants its own ADR. Treat the
idea as a research direction, not a deliverable of this ADR.

### 5. Explicitly rejected

- Online/neural learning inside the sim loop — breaks determinism and testability.
- Reading live match state to adapt mid-match beyond what KnowledgeMap allows — that is cheating by another name.
- Server-side profile aggregation across players — out of scope, privacy cost, no v1 need.

## Consequences

**Positive**: the headline feature "враг помнит тебя" with zero determinism risk; fully testable (given profile P, AI selects doctrine D); the transparency screen turns the model into a player-facing feature rather than hidden manipulation.

**Negative / risks**:
- Fun risk: hard-countering the player's favorite style can feel punitive. Mitigation: bias clamps + difficulty-gated adaptation strength + the transparency screen teaching the player to vary.
- Analyzer quality: garbage classification ⇒ nonsense adaptation. The analyzer needs its own labeled-replay test corpus.
- Scope: the replay analyzer is a real tool (Tools/ stream), estimate as such — this is the largest cost item of the five perception-warfare systems.

**Additional design risks (review P-12)**:
- **Profile poisoning**: a player can deliberately play 10 "all air" matches to teach the AI to
  over-invest in AA, then switch. Accepted as *legitimate metagaming* — outwitting the adaptation
  is playing the system as designed — with one guard: bias clamps mean the poisoned counter never
  exceeds the clamp, so the exploit yields an edge, not a free win. The transparency screen makes
  the same play available to everyone.
- **Newcomer hostility**: 3–5 matches at low difficulty must not produce a confident profile.
  Adaptation strength scales with sample size (below N=8 matches, DoctrineBias influence is zero)
  and is off entirely at the two lowest difficulties (already in RISK-14 mitigations; restated here
  as a hard rule of this ADR).
- **Transparency screen as an instruction manual**: showing "what the enemy learned" also shows how
  to manipulate it. Accepted consciously — see profile poisoning above; the alternative (hidden
  model) fails RISK-14 worse.

## Verification plan

1. Determinism: same replay + same DoctrineBias ⇒ identical match hash sequence.
2. Analyzer tests: labeled replay corpus ⇒ expected profile features (precision/recall thresholds defined in test plan).
3. Bias-bounds test: fuzz DoctrineBias inputs ⇒ AI never exceeds capability envelope (no cheat regression).
4. UX check: profile screen renders every field that influences AI behaviour (no hidden variables).
