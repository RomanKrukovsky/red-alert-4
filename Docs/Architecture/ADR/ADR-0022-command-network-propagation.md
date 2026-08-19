# ADR-0022: Command Network — Physical Order Propagation and Autonomy Fallback

**Status**: **ACCEPTED 2026-08-05** (product owner, after independent review APPROVE-WITH-CHANGES and
application of all required changes: end-to-end latency budget, order-supersede specification,
recompute phase/order contract). Implementation may be scheduled; sequencing per
PERCEPTION_WARFARE_DIRECTION §2 (after Knowledge Map system 1 milestones it depends on).
**Depends on**: ADR-0001 (fixed-tick lockstep, 20 Hz), ADR-0003 (command protocol), ADR-0008 (HTN/utility AI), ADR-0021 (Knowledge Map)

## Context

The perception-warfare direction requires that orders travel through a player-built command infrastructure (HQ, relay towers, command vehicles, satellites). Jamming or destroying infrastructure degrades command: delayed orders, continued execution of stale orders, or local-doctrine autonomy.

Critical constraint: in a lockstep engine, *player input commands* already have a fixed input-delay contract (LockstepSession). Command propagation must NOT touch the network input pipeline — it is a **simulation-side** mechanic layered after command admission.

## Decision

### 1. Two-stage command model

Stage A (unchanged): player input → CommandBus → deterministic admission at tick T. This preserves lockstep and replay format semantics (a replay stores admitted commands, exactly as today).

Stage B (new): an admitted order to unit group G is not applied instantly. It enters `OrderDelivery` state and propagates through the **CommandGraph**:

```
DeliveryTick = AdmissionTick + PropagationLatency(path from nearest command node to G)
```

- PropagationLatency is a deterministic function of graph topology (hop count × per-node latency, integer ticks).
- No connected path ⇒ order is queued at the nearest reachable node or dropped after TTL, per doctrine settings.

### 2. CommandGraph

- Nodes: HQ, relay structures, command vehicles, satellite uplinks — all are ordinary sim entities with a `CommandNode` component (data-driven via JSON content, per ADR-0009).
- Edges: derived deterministically from range/LOS rules. **Recompute contract (per review P-8)**:
  - **Tick phase**: CommandGraph maintenance runs as a fixed phase inside `SimWorld::Tick`,
    immediately *after* entity create/destroy processing and *before* order-delivery advancement,
    so a node destroyed at tick T affects deliveries from tick T, never retroactively.
  - **Traversal order**: nodes are processed in ascending `EntityId`; edges are evaluated in
    (lower id, higher id) pair order. Latency lookup is a BFS from the group's nearest node,
    visiting neighbours in ascending `EntityId` — identical on every peer by construction.
  - **Amortization**: full recompute is event-triggered (node created/destroyed/jammed); between
    events, K-tick refresh (K = 20, one checksum interval) revalidates range/LOS in id order,
    1/Kth of nodes per tick. Both paths are hash-covered, so any divergence is a desync at the
    next checksum tick, not a silent drift.
- Jamming: an area effect that raises edge latency or severs edges — implemented as a component state, not as RNG.

### 3. Autonomy fallback (no new AI stack)

Units without a live order link execute their **standing doctrine** — a small, player-authored policy (hold / retreat to rally / continue last order / local commander discretion). This reuses the existing HTN/utility machinery from RA4AI at squad scope. Explicitly rejected: a second, parallel AI system (forbidden by project rules on duplicate subsystems).

### 4. UX contract (anti-frustration)

- **End-to-end latency budget (includes lockstep input delay, per review P-8)**: total
  click-to-execution time is `InputDelayTicks + PropagationTicks`. LockstepSession's input delay is
  2 ticks (100 ms) in a healthy session; the propagation budget for a healthy CommandGraph is
  therefore **≤ 2 ticks (100 ms)**, keeping the end-to-end total at **≤ 4 ticks (200 ms)** — the
  threshold below which order latency is imperceptible in an RTS. Under degraded infrastructure,
  propagation may grow without bound (that is the mechanic); the *healthy-path* number is the CI
  gate. `Classic` mode sets propagation to 0, making end-to-end equal to bare lockstep.
- UI must show per-group link status (connected / degraded / autonomous) sourced from sim state.
- Skirmish option `CommandNetwork=Classic` disables Stage B entirely (latency 0) — needed for balance A/B and esports mode. This is a sim parameter, hash-relevant, recorded in replay header.

### 5. Determinism & replay

- All latencies are integer ticks computed from sim state. No wall-clock, no float.
- Replay format: unchanged record structure; header gains `CommandNetworkMode` field (versioned per SAVE_AND_REPLAY.md).
- State hash covers CommandGraph edges and in-flight orders.

## Consequences

**Positive**: infrastructure becomes a legitimate military target; "build the nervous system of your army" is a genuinely novel base-building axis; esports-safe via Classic mode.

**Negative / risks**:
- Balance risk is high: order delay is felt as input lag if tuned badly. Mitigation: healthy-path latency budget above + telemetry (ADR-0020 hooks).
- Pathological micro cases (rapid re-orders creating in-flight order floods) — closed by the
  **order-supersede rule**, specified here (per review P-8):
  - Key: `(IssuingPlayer, TargetGroupId)`. At most **one** undelivered order per key exists in the
    graph at any time.
  - On admission of a new order with the same key, the older undelivered order is cancelled *at
    admission tick*, deterministically, regardless of where in the graph it currently is. Cancelled
    orders are counted (telemetry) but produce no unit-visible effect.
  - An order that has already **delivered** is never affected — supersede applies to in-flight
    orders only; changing a unit's current activity requires the new order to arrive.
  - Queued-orders (shift-queue) form a single composite order under one key; superseding replaces
    the whole queue, matching what players expect from re-issuing commands.
  - Bound: in-flight order storage is therefore ≤ (players × alive groups), which fixes the memory
    budget in PERFORMANCE_BUDGETS §4.2 without a separate cap.
- Interaction with DirectControl (ADR-0029): **resolved — a possessed unit bypasses the
  graph entirely.** The commander is physically present in that vehicle, so there is no radio link to
  model, and adding delay to a first-person control scheme would be indefensible as a control feel.
  Consequences: possession becomes a deliberate counter to being jammed, at the cost of controlling
  exactly one unit while the rest of the army runs on standing doctrine — a real tradeoff rather than
  an exemption. On unpossession the unit rejoins the graph and its next order propagates normally.
  Recorded in GDD section 9.

**Additional design risks (review P-12)**:
- **Spectator/caster readability**: link-status states and delayed orders are invisible to a
  spectator watching the objective view; a caster cannot explain why an army ignored a command.
  The spectator overlay (ADR-0010 delay buffer) must be able to render any player's link status —
  accepted as a UI requirement, not a sim change.
- **Snowball coupling**: losing map control also degrades command, compounding defeat. Partially
  intended (infrastructure is a stake), but doctrine autonomy is the designed floor: a fully
  disconnected army still fights via standing doctrine, never becomes inert. Playtest gate: a
  disconnected-but-doctrined army must retain ≥70% of its connected combat effectiveness in a
  defensive stance.

## Verification plan

1. Determinism test: identical CommandGraph latency computation across two instances, 10k ticks.
2. Contract test: with `Classic` mode, behaviour byte-identical to pre-ADR baseline (regression harness).
3. Gameplay test: destroying the only relay puts dependent squads into doctrine state within TTL ticks.
4. Replay round-trip with both modes.
5. Perf: graph recompute for 200 nodes within amortized budget.
