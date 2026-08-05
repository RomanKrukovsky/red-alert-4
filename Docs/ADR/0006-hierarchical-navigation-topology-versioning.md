# ADR 0006 - Hierarchical Navigation, Vector Flow Fields, and Topology Versioning

> **SUPERSEDED — NOT AUTHORITATIVE (marked 2026-08-05).**
> This file lives in `Docs/ADR/`, a legacy ADR directory. The single authoritative ADR
> location is **`Docs/Architecture/ADR/`** (per CLAUDE.md and ADR-0011). This document is
> retained for history and audit trail only; do not cite it as a current decision, and do not
> add new ADRs here. If the subject below is still live, the current record is in
> `Docs/Architecture/ADR/`.
>
> ADR-0011 decided in principle that these files be marked superseded, but the marking was never
> applied — this banner performs it.

Status: accepted.

## Context

Standard Unreal NavMesh (`ARecastNavMesh`) cannot perform per-agent A* pathfinding queries for 1,000+ units per frame without severe CPU performance degradation and main-thread stalls. Furthermore, real-time map modifications in Red Alert 4 (destroying bridges, placing massive structures, terrain destruction, superweapon craters) require real-time pathfinding grid updates without costly global navmesh re-bakes.

## Decision

1. **Two-Tier Hierarchical Pathfinding (HPA*)**:
   - **Macro Layer**: The map is partitioned into a fixed sector grid (e.g. 64x64 fixed-point units per sector). Inter-sector connections are represented as border portals in a macro topological graph. Long-distance routes query the macro graph via A* to generate a sequence of sector portals.
   - **Micro Layer (Flow Fields)**: Mass movement within sectors or toward active chokepoints is driven by 2D vector Flow Fields (Eikonal / Dijkstra wave propagation). Units sample direction vectors from shared flow fields instead of executing individual path searches.
2. **Multi-Layer Passability**:
   - Navigation grid layers are maintained for distinct movement classes: Infantry, Wheeled Vehicles, Tracked Vehicles, Amphibious Units, Naval Vessels, and Air (3D flight corridors).
3. **Dynamic Topology Versioning (`TopologyVersion`)**:
   - Map modifications (e.g., bridge destruction, building placement) do NOT trigger global navmesh re-bakes.
   - Modifications invalidate only local sector portals and increment a global 32-bit `TopologyVersion` counter for affected sectors.
   - Cached unit paths store the `TopologyVersion` under which they were generated. When a unit crosses an invalidated sector or updates its order, a version mismatch check triggers an incremental local path update.
4. **Local Collision Avoidance**:
   - Micro-steering around dynamic obstacles and friendly units uses a spatial reservation grid combined with lightweight RVO2 / ORCA avoidance.

## Consequences

**Positive**:
- Moving 1,000+ units requires minimal CPU overhead because 100 units following the same destination share a single Flow Field calculation.
- Destroying bridges or erecting walls executes in sub-millisecond time by invalidating local sector portals and updating `TopologyVersion`.

**Negative**:
- Memory usage increase for storing vector grids per active movement sector.
- Chokepoints require flow field vector dampening to prevent overcrowding artifacts.
