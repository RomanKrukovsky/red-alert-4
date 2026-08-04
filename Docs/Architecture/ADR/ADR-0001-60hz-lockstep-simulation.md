# ADR-0001: 60Hz Lockstep Simulation Architecture

## Context
High-entity RTS games (1,000 - 2,000 active entities) require low bandwidth network protocols that guarantee bit-identical state across all clients.

## Decision
Adopt a **60Hz Lockstep Network Architecture** (`LockstepSession`) where clients exchange player command frames rather than full entity transform updates.

## Rationale
- Network bandwidth is reduced from ~500 KB/s per client (state sync) to <10 KB/s (command frames).
- Guarantees 100% accurate simulation reproduction across PC platforms.

## Status
**ACCEPTED / IMPLEMENTED**. Verified by `Lockstep.*` unit test suite (378/378 PASS).
