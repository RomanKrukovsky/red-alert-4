# ADR-0007: Grid Flowfield Navigation for Large Army Pathfinding

## Context
Standard 3D Recast NavMesh pathfinding scales poorly when computing individual paths for 2,000 active units moving to identical rally points.

## Decision
Utilize a 2D Grid Flowfield Navigation Engine (`RA4Navigation`) where a single vector flow field is calculated per target destination and shared by all units in a squad.

## Rationale
- O(1) path lookup cost per unit once flowfield is computed.
- Zero CPU budget overruns during mass army movement.

## Status
**ACCEPTED / IMPLEMENTED**. Verified by `RA4Navigation` unit tests.
