# ADR-002: Deterministic Fixed-Point Arithmetic

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
IEEE 754 floating-point operations can yield divergent results across different CPU microarchitectures (x86_64 vs arm64), SIMD vector instructions, and compiler optimization flags (`-O3`, `-ffast-math`).

## Decision
All simulation state variables affecting movement, collision, health, damage, velocities, and bounding boxes must use `Fixed` (`Fixed32` / `Fixed64`), a 32-bit/64-bit integer fixed-point number representation (16.16 bit fixed scale). Floating-point `float` or `double` are forbidden inside `RA4Simulation`.

## Consequences
- Guaranteed bit-identical calculations across macOS, Windows, and Linux.
- Requires explicit fixed-point vector wrappers (`FixedVector2`, `FixedVector3`).
