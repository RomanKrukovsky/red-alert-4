# ADR 0007 - Save System & Authoritative State Serialization

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

A single Red Alert 4 match or campaign mission contains thousands of simulation entities, active projectiles, production queues, power states, fog of war masks, and AI decision trees. Saving transient visual states (such as active Niagara particles, screen flashes, decals, audio voices, or physics ragdolls) inflates save file sizes, creates versioning incompatibilities, and risks non-deterministic desynchronization upon reloading.

## Decision

1. **Pure Simulation Serialization**:
   - The save file (`.ra4save`) stores exclusively authoritative `SimWorld` state in a versioned binary format.
   - Saved data includes: fixed-step tick index, deterministic PRNG seed, player resource banks, tech tree unlocks, building status, production queues, unit attributes (position, velocity vector, health, experience, order queue, target entity ID), active projectile trajectories, remaining lifetimes, and dirty fog-of-war bitmasks.
2. **Visual Presentation Exclusion**:
   - Transient visual entities (Niagara particle systems, decals, audio instances, client UI animations, physics debris, ragdolls) are EXCLUDED from save files.
3. **Post-Load Presentation Re-Hydration**:
   - Upon loading a save file, the simulation state is populated into `SimWorld`.
   - Presentation adapters (`PresentationSubsystem`) observe existing simulation entities and re-instantiate clean presentation representations (Actors, ISM instances, Mass representation slots) matching entity types and health states.
4. **Binary Schema & Versioning**:
   - Binary streams use tagged chunk headers (`uint32 ChunkID`, `uint32 Version`, `uint32 DataSize`).
   - Backward-compatibility migration handlers allow older save files to be loaded across minor game updates.

## Consequences

**Positive**:
- Save file sizes remain small (< 5 MB for large 8-player end-game states).
- Save/Load logic is 100% deterministic and testable via headless unit tests.
- Visual engine updates (e.g. changing Niagara particle systems or UI widgets) will never break existing save files.

**Negative**:
- Mid-flight aesthetic effects (e.g. a smoke puff mid-explosion) reset to initial particle state upon loading.
