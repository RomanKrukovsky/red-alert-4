# ADR-008: Hierarchical AI & Strategic Director

## Status
Accepted

## Context
RTS AI must manage macro economy, base building, tech choices, army composition, and micro tactics simultaneously without CPU spikes.

## Decision
1. **Multi-Layer Architecture**:
   - `StrategicDirector`: Evaluates match state, switches macro strategy (`Economy`, `TechUp`, `Assault`, `Fortify`, `Recovery`).
   - `EconomyPlanner` / `ProductionPlanner`: Issues building and training orders based on utility scoring.
   - `TacticalOperation`: Manages strike forces, raiding parties, and defensive positioning.
