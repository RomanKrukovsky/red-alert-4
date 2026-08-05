# ADR-007: Multi-Tier Economy & Faction Resources

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
Each faction in Red Alert 4 features distinct economic mechanics alongside global credits and power.

## Decision
1. **Shared Economy**: Credits, Power, and Command Limit are tracked per player state in `RA4Simulation`.
2. **Unique Faction Resources**:
   - `Soviet`: Mobilization (accrues from damage taken).
   - `Alliance`: Intelligence (gained via radar surveillance and recon).
   - `Eastern Coalition`: Synchronization (gained via formation combat).
   - `Chrono Legion`: Temporal Stability (regenerates passively over time).
