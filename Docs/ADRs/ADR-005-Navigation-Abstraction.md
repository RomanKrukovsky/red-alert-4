# ADR-005: Multi-Layer Navigation & Grid Reservation

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
Pathfinding for 500+ RTS units on CPU cannot rely on per-unit A* path queries due to $O(N \cdot K)$ complexity.

## Decision
1. **Flow Fields**: Group navigation utilizes grid flow fields (`FlowField`) for macro movement.
2. **Reservation Grid**: Micro collision and position locking use `ReservationGrid` to prevent unit overlap on target cells.
3. **Movement Layers**: Support distinct layers: `Infantry`, `Wheeled`, `Tracked`, `Amphibious`, `Naval`, and `Air`.
