# ADR 0005 - Authoritative Dedicated Server, Command Replication, and Client Resynchronization

> **SUPERSEDED — NOT AUTHORITATIVE (marked 2026-08-05).**
> This file lives in `Docs/ADR/`, a legacy ADR directory. The single authoritative ADR
> location is **`Docs/Architecture/ADR/`** (per CLAUDE.md and ADR-0011). This document is
> retained for history and audit trail only; do not cite it as a current decision, and do not
> add new ADRs here. If the subject below is still live, the current record is in
> `Docs/Architecture/ADR/`.
>
> ADR-0011 decided in principle that these files be marked superseded, but the marking was never
> applied — this banner performs it.

Status: accepted.

## Context

In an 8-player RTS game with thousands of entities, traditional transform-based state replication overloads network bandwidth. While lockstep networking minimizes bandwidth by transmitting only client commands, network lag or client floating-point desynchronizations can stall or break the simulation if clients are treated as peer sources of truth. 

To maintain competitive integrity and fair reconnection, the dedicated server must remain the absolute authority over simulation state.

## Decision

1. **Authoritative State Engine**: The Linux Dedicated Server runs the authoritative `SimWorld` fixed-step simulation at 30 Hz.
2. **Command Input Stream**: Clients do not replicate transforms or entity velocities. Clients send serialized, timestamped, signed player commands (`FSimCommand`) to the server. The server validates ownership, affordability, placement, technology dependencies, and applies a rate-limit per player tick.
3. **Delta Checksum Audit**: Every tick, the server computes a 64-bit CRC/XXHash over entity positions, health, order queues, and resource balances. The server broadcasts this tick checksum to clients.
4. **Client Resynchronization**: If a client's local simulation checksum deviates from the server checksum, the client enters a `Resyncing` state. The server streams a compressed binary state snapshot (`ResyncFrame`) to the client, restoring local entity state without terminating the match or affecting other players.
5. **Desync Diagnostic Dumper**: Offline automated CI runs and local development builds use a `DesyncDiagnosticDumper` tool to record step-by-step entity diffs, identifying deterministic divergence causes (e.g. non-deterministic iteration order or floating-point variance) in replays and test suites.

## Consequences

**Positive**:
- Cheating by altering client memory/transforms is impossible because client state is non-authoritative.
- Spectators, late-joiners, and re-connecting players download snapshots and seamlessly jump back into active matches.
- Match continuity is maintained even if one client experiences local state corruption.

**Negative**:
- Streaming full resync snapshots requires transient server bandwidth overhead during client resync events.
- Client prediction for local unit selection/movement response requires careful visual interpolation against server command confirmation.
