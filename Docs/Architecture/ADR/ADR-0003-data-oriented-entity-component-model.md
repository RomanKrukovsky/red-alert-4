# ADR-0003: Struct-of-Arrays (SoA) Entity Component Model

## Context
Traditional Object-Oriented (OOP) entity hierarchies cause severe CPU cache misses when iterating through 2,000 active entities every tick.

## Decision
Store entity components in contiguous `std::vector` arrays within `SimWorld` (Struct-of-Arrays memory layout).

## Rationale
- Maximizes L1/L2 CPU cache hit rates during movement, health, and combat tick passes.
- 500-entity stress test completes 1,000 ticks in <450ms (`ProvingGround.HeadlessStressScenario500Entities`).

## Status
**ACCEPTED / IMPLEMENTED**.
