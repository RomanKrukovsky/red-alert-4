# ADR-008: Hierarchical AI & Strategic Director

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
RTS AI must manage macro economy, base building, tech choices, army composition, and micro tactics simultaneously without CPU spikes.

## Decision
1. **Multi-Layer Architecture**:
   - `StrategicDirector`: Evaluates match state, switches macro strategy (`Economy`, `TechUp`, `Assault`, `Fortify`, `Recovery`).
   - `EconomyPlanner` / `ProductionPlanner`: Issues building and training orders based on utility scoring.
   - `TacticalOperation`: Manages strike forces, raiding parties, and defensive positioning.
