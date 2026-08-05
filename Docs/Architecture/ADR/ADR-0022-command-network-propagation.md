# ADR-0022: Command Network — Physical Order Propagation and Autonomy Fallback

**Status**: Proposed (pending independent review — NEXT_ACTIONS P-1; no implementation authorized)
**Depends on**: ADR-0001 (fixed-tick lockstep; NOTE: actual rate is 20 Hz per SimConfig.h kTicksPerSecond, not the 60Hz claimed in ADR-0001), ADR-0003 (command protocol), ADR-0008 (HTN/utility AI), ADR-0021 (Knowledge Map)

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
- Edges: derived deterministically from range/LOS rules each K ticks (amortized recompute, event-triggered on node create/destroy).
- Jamming: an area effect that raises edge latency or severs edges — implemented as a component state, not as RNG.

### 3. Autonomy fallback (no new AI stack)

Units without a live order link execute their **standing doctrine** — a small, player-authored policy (hold / retreat to rally / continue last order / local commander discretion). This reuses the existing HTN/utility machinery from RA4AI at squad scope. Explicitly rejected: a second, parallel AI system (forbidden by project rules on duplicate subsystems).

### 4. UX contract (anti-frustration)

- Default multiplayer latency for a healthy network: ≤ 4 ticks (200 ms at 20 Hz) — barely perceptible; the mechanic becomes visible only under attack on infrastructure.
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
- Pathological micro cases (rapid re-orders creating in-flight order floods) — need an order-supersede rule: a newer order to the same group cancels older undelivered ones.
- Interaction with DirectControl (ADR-0011-DirectControl): **resolved — a possessed unit bypasses the
  graph entirely.** The commander is physically present in that vehicle, so there is no radio link to
  model, and adding delay to a first-person control scheme would be indefensible as a control feel.
  Consequences: possession becomes a deliberate counter to being jammed, at the cost of controlling
  exactly one unit while the rest of the army runs on standing doctrine — a real tradeoff rather than
  an exemption. On unpossession the unit rejoins the graph and its next order propagates normally.
  Recorded in GDD section 9.

## Verification plan

1. Determinism test: identical CommandGraph latency computation across two instances, 10k ticks.
2. Contract test: with `Classic` mode, behaviour byte-identical to pre-ADR baseline (regression harness).
3. Gameplay test: destroying the only relay puts dependent squads into doctrine state within TTL ticks.
4. Replay round-trip with both modes.
5. Perf: graph recompute for 200 nodes within amortized budget.
