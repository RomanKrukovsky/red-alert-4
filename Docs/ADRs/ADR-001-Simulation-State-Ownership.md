# ADR-001: Simulation State Ownership & Fixed-Step Clock

> **SUPERSEDED — NOT AUTHORITATIVE (marked 2026-08-05).**
> This file lives in `Docs/ADRs/`, a legacy ADR directory. The single authoritative ADR
> location is **`Docs/Architecture/ADR/`** (per CLAUDE.md and ADR-0011). This document is
> retained for history and audit trail only; do not cite it as a current decision, and do not
> add new ADRs here. If the subject below is still live, the current record is in
> `Docs/Architecture/ADR/`.
>
> ADR-0011 decided in principle that these files be marked superseded, but the marking was never
> applied — this banner performs it.

## Status
Accepted

## Context
Standard Unreal Engine gameplay systems rely on `AActor::Tick(float DeltaTime)` and variable frame rates, which are inherently non-deterministic due to floating-point rounding variations across platforms and CPU architectures. Classic RTS architectures (SAGE / Zero Hour / RA3) separate simulation state from visual presentation using a fixed-step simulation clock.

## Decision
1. **Engine-Free Core Simulation**: The simulation state is owned strictly by `SimWorld` in `RA4Simulation`, running completely independently of Unreal Engine's `UWorld` or `AActor` hierarchy.
2. **Fixed-Step Clock**: The simulation operates at a strict 20Hz (50ms per tick) fixed step using fixed-point time representation.
3. **State Snapshot & Hash**: Every tick, `SimWorld` calculates a canonical 64-bit state checksum (`CalculateStateChecksum()`) covering all active entities, player resources, health, positions, and commands.

## Consequences
- **Positive**: 100% cross-platform deterministic replay and lockstep multiplayer synchronization.
- **Negative**: Visual presentation must interpolate entity positions between fixed simulation ticks.
