# ADR-0004: 64-Bit State Hashing for Desync Detection

## Context
Multiplayer lockstep requires instant detection if any client diverges due to memory corruption, illegal modifications, or non-deterministic code.

## Decision
Calculate a 64-bit FNV-1a state checksum (`SimWorld::CalculateStateHash`) every 10 ticks and exchange it in network lockstep frames.

## Rationale
- Instantaneous desync detection on the exact tick divergence occurs.
- Tested by `Lockstep.DesyncIsCaughtOnTheTickItHappens`.

## Status
**ACCEPTED / IMPLEMENTED**.
