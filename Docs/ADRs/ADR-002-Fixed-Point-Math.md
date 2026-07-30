# ADR-002: Deterministic Fixed-Point Arithmetic

## Status
Accepted

## Context
IEEE 754 floating-point operations can yield divergent results across different CPU microarchitectures (x86_64 vs arm64), SIMD vector instructions, and compiler optimization flags (`-O3`, `-ffast-math`).

## Decision
All simulation state variables affecting movement, collision, health, damage, velocities, and bounding boxes must use `Fixed` (`Fixed32` / `Fixed64`), a 32-bit/64-bit integer fixed-point number representation (16.16 bit fixed scale). Floating-point `float` or `double` are forbidden inside `RA4Simulation`.

## Consequences
- Guaranteed bit-identical calculations across macOS, Windows, and Linux.
- Requires explicit fixed-point vector wrappers (`FixedVector2`, `FixedVector3`).
