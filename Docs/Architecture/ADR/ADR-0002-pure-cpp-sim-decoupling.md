# ADR-0002: Pure C++ Simulation Core Decoupling from Unreal Engine

## Context
Coupling RTS simulation logic to Unreal Engine UObjects creates garbage collection pauses, non-deterministic float arithmetic, and slow headless compilation.

## Decision
Build the **Simulation Kernel (`RA4Simulation`) as a pure C++ static library** completely isolated from UObjects and engine headers.

## Rationale
- Enables ultrafast headless C++ unit testing (378 tests execute in <6 seconds).
- Eliminates Unreal Engine Garbage Collection pauses during match ticks.
- Guarantees cross-platform bit-identical execution.

## Status
**ACCEPTED / IMPLEMENTED**. Verified by `RA4Tests`.
