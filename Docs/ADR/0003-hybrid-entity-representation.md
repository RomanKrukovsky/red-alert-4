# ADR 0003 - Simulation entities are decoupled from their representation

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
