# ADR 0003 - Simulation entities are decoupled from their representation

> **SUPERSEDED — NOT AUTHORITATIVE (marked 2026-08-05).**
> This file lives in `Docs/ADR/`, a legacy ADR directory. The single authoritative ADR
> location is **`Docs/Architecture/ADR/`** (per CLAUDE.md and ADR-0011). This document is
> retained for history and audit trail only; do not cite it as a current decision, and do not
> add new ADRs here. If the subject below is still live, the current record is in
> `Docs/Architecture/ADR/`.
>
> ADR-0011 decided in principle that these files be marked superseded, but the marking was never
> applied — this banner performs it.

Status: accepted; simulation side implemented, representation side not started.

## Context

An RTS with thousands of units cannot afford an `ACharacter` with a
`CharacterMovementComponent`, an `AIController`, a Behavior Tree and a `Tick` per
soldier. Equally, buildings, heroes and aircraft need real animation and
interactivity.

## Decision

Simulation entities are rows in structure-of-arrays storage with stable
`EntityId` handles. Representation is chosen per entity type: instanced static
meshes or MassEntity for massed infantry, drones and simple projectiles; light
Actors for buildings, heroes, large vehicles, aircraft and ships.

The simulation exposes storage, movement, targeting and representation through its
own interfaces, so the Mass-backed implementation can be replaced without touching
system logic.

## Consequences

The simulation is testable and cheap. The cost is a synchronisation layer between
simulation rows and presentation objects, driven by `SimEvent` plus per-tick state
reads and interpolated between ticks.
