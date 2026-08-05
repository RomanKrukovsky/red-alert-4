# ADR-0025: Adaptive Opponent — Cross-Match Player Modeling and Doctrine Counter-Selection

**Status**: Proposed (pending independent review — NEXT_ACTIONS P-1; no implementation authorized)
**Depends on**: ADR-0008 (HTN/utility AI), ADR-0020 (economic telemetry), ADR-0021 (Knowledge Map)

## Context

Direction: the AI opponent learns the *player's habits* between matches — heavy air usage ⇒ layered AA next match; habitual right-flank routes ⇒ mines on those routes; early-rush preference ⇒ bait-base openings. Constraint from the concept and from fairness: the AI gets **no unfair information or resources** — it adapts doctrine selection, not knowledge of the current match.

Hard architectural constraint: the sim is deterministic. Any learning artifact that influences a match must be an **input** to the match, fixed before tick 0 — never something mutating during the match from out-of-sim data.

## Decision

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
- Bounded influence: biases reweight existing utility scores within clamped ranges; they cannot grant resources, vision, or new capabilities (zero-cheat preserved).

### 4. "Digital twin" (stretch, same architecture)

After sufficient matches, a DoctrineBias can be compiled *from* the player's own profile ⇒ an AI that plays approximately like the player. This is a free by-product of the design, not a separate system; ships only if the base loop proves fun.

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

## Verification plan

1. Determinism: same replay + same DoctrineBias ⇒ identical match hash sequence.
2. Analyzer tests: labeled replay corpus ⇒ expected profile features (precision/recall thresholds defined in test plan).
3. Bias-bounds test: fuzz DoctrineBias inputs ⇒ AI never exceeds capability envelope (no cheat regression).
4. UX check: profile screen renders every field that influences AI behaviour (no hidden variables).
