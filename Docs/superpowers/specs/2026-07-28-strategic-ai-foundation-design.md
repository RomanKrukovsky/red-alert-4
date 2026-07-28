# Strategic AI Foundation — Design

**Date:** 2026-07-28
**Status:** Approved for planning
**Scope:** First AI milestone

## Goal

Turn the existing fixed-priority `AICommander` into a deterministic Utility AI
that can choose between economy, technology, defence, army production, assault and
recovery. The result must remain engine-free, use the normal command pipeline and
run in headless AI-versus-AI tests.

This milestone provides a stable foundation for later HTN planning, scouting,
influence maps, squad control and Unreal-side unit presentation. It does not add
those systems yet.

## Current state

The project already has an engine-free `AICommander`. It can build a basic economy,
place structures, train units, attack and use four behaviour profiles. Its choices
are made by a hard-coded priority chain, so a valid action near the top always
wins, even when another action is more important.

The focused AI test target currently stops at compilation because
`IsConstructionYard` is defined but unused. This must be fixed before behaviour is
changed, establishing a clean baseline.

The commander currently reads `SimWorld` directly and can therefore see every
enemy entity. Fog-of-war filtering is not part of this milestone. The new world
assessment boundary will make that filtering possible later without rewriting the
decision system.

## Chosen approach

Use a small, project-owned Utility AI inside `RA4AI`.

Marketplace plugins and Unreal systems such as Athena AI, StateTree and MassEntity
will not enter the authoritative simulation layer. A full HTN planner is also
deferred until the game has enough strategic actions and map knowledge to justify
multi-step plans.

All scores use integers. Candidate order, tie-breaking and command emission are
fixed, so identical inputs always produce identical results.

## Architecture

### `AICommander`

`AICommander` remains the public entry point. It owns the player slot, profile,
configuration, decision cadence, active strategy and diagnostic history. It reads
the world and emits ordinary `Command` values; it never mutates simulation state.

### `AIWorldAssessment`

At each decision interval, the commander scans `SimWorld` once and creates a small
value object containing only facts needed for decisions:

- credits, power and income;
- owned buildings, harvesters and armed units;
- queued production;
- available production categories;
- whether the base was recently damaged;
- known enemy buildings and units;
- current attack force and whether an assault is active.

Scoring and action selection use this assessment rather than repeatedly walking all
entities. The assessment is deterministic and contains no Unreal types.

### `AIStrategy`

The commander evaluates six mutually exclusive strategic modes:

1. `ExpandEconomy`
2. `TechUp`
3. `Fortify`
4. `AssembleArmy`
5. `Assault`
6. `Recover`

Every mode receives a score from 0 to 1000. Scores are built from explicit integer
terms such as resource shortage, missing prerequisites, recent damage, army
strength and profile weights. A stable enum order resolves equal scores.

The active strategy has hysteresis: another strategy must beat it by a configured
margin before taking over. Emergency recovery and defence may switch immediately.
This prevents the commander from changing its mind every decision interval.

### `AIActionCandidate`

The selected strategy creates valid action candidates. Each candidate records:

- command to issue;
- utility score;
- stable priority;
- short diagnostic reason.

The commander emits at most one production or placement action per decision
interval. It may also emit one coordinated army order when entering or maintaining
an assault. Existing server-side command validation remains authoritative.

### Profiles

`Balanced`, `Aggressive`, `Defensive` and `Economic` remain data-driven
configurations. Profiles modify score weights, thresholds, credit reserve, desired
economy size and attack strength; they do not have separate decision code.

For example, the aggressive profile raises `Assault` utility earlier, while the
economic profile raises `ExpandEconomy` utility until its larger harvester target
is met.

### Diagnostics

The existing bounded decision log is extended with:

- selected strategy;
- winning score;
- previous strategy;
- emitted command;
- concise reason.

The log must remain bounded and excluded from authoritative simulation state.

## Decision flow

On each simulation tick:

1. Return immediately if the match is over or the player is defeated.
2. Wait for the configured decision interval.
3. Build `AIWorldAssessment`.
4. Score every strategy in stable enum order.
5. Apply hysteresis and select the active strategy.
6. Build and rank valid action candidates for that strategy.
7. Emit commands through the normal command frame.
8. Record the decision and reason.

No wall-clock time, floating-point values, unordered iteration or presentation state
may influence this flow.

## Failure handling

- Missing content or producers produce no candidate rather than an invalid command.
- Insufficient credits preserve the configured reserve except during emergency
  recovery.
- Invalid building placement is handled by the existing deterministic ring search.
- If no action is valid, the commander records an idle reason and reassesses at the
  next interval.
- Command validation remains in `SimWorld`; the AI does not bypass ownership,
  prerequisites, placement or match-state rules.
- Strategy switches handle every declared value explicitly and fail loudly in
  development if an invalid value reaches them.

## Testing

The focused `RA4AITests` target is the main feedback loop.

Required coverage:

- the existing AI tests compile and pass before refactoring;
- every strategic mode can win under a controlled world state;
- profiles produce meaningfully different scores and match behaviour;
- hysteresis prevents repeated strategy changes near equal scores;
- emergency defence or recovery overrides hysteresis;
- command emission respects reserves and one-action limits;
- decision logs contain strategy, score and reason;
- two AI players build, earn and fight in a headless match;
- two identical runs produce the same checksum and decision sequence;
- sanitizer builds pass.

The full headless suite must also pass to guard simulation, navigation, input and
replay behaviour.

## Non-goals

This milestone does not include:

- HTN or other multi-step plan search;
- fog-of-war memory or anti-cheat visibility filtering;
- scouting and probabilistic enemy locations;
- influence maps, threat maps or EQS;
- squads, formations or retreat micro-management;
- StateTree, MassEntity, Learning Agents or marketplace plugins;
- difficulty bonuses or hidden information.

These become later milestones after the Utility AI foundation is verified.

## Completion criteria

The milestone is complete when:

- `RA4AITests` and the full headless suite compile and pass;
- the commander selects strategies through scored utility rather than a fixed
  priority chain;
- all four profiles produce observable behavioural differences;
- AI-versus-AI matches remain deterministic;
- diagnostics explain every selected strategy and emitted action;
- the authoritative simulation remains free of Unreal dependencies.
