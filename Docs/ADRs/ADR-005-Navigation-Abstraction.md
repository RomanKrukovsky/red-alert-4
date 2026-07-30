# ADR-005: Multi-Layer Navigation & Grid Reservation

## Status
Accepted

## Context
Pathfinding for 500+ RTS units on CPU cannot rely on per-unit A* path queries due to $O(N \cdot K)$ complexity.

## Decision
1. **Flow Fields**: Group navigation utilizes grid flow fields (`FlowField`) for macro movement.
2. **Reservation Grid**: Micro collision and position locking use `ReservationGrid` to prevent unit overlap on target cells.
3. **Movement Layers**: Support distinct layers: `Infantry`, `Wheeled`, `Tracked`, `Amphibious`, `Naval`, and `Air`.
