# ADR-0014: Per-Tile Threat Map and Value Map Grids

## Context

The AI commander previously selected targets via a flat scoring loop that walked all `EnemyMemory` entries and assigned integer scores based on entity role (building = 1000, production = +400, etc.). This had no spatial awareness: a distant power plant scored the same as a nearby one, the AI could not evaluate "how dangerous is this sector," and there was no foundation for future director-level decision making (economy director needs value map, defense director needs threat map, battle predictor needs both).

## Decision

Add two new engine-free, deterministic grid classes to the `RA4AI` module:

- **ThreatMap** — per-tile grid showing how dangerous each area is, computed from fog-limited enemy sightings. Threat is derived from weapon DPS (Damage * 100 / CooldownTicks), spread within weapon range using Chebyshev-distance linear falloff, and weighted by `EnemyMemory.Confidence` (stale sightings contribute less). Tracks TotalThreat, AirThreat, AntiArmorThreat, and StructuralThreat per cell.

- **ValueMap** — per-tile grid showing strategic value from the owning player's perspective. Combines economic value (resource nodes, refineries, harvesters), military value (production buildings, construction yard), and proximity bonus (distance to own base) into a single integer score. Includes `FindBestAttackTarget()` that balances value against threat to favor undefended high-value targets.

Both are owned by `AICommander` as part of its knowledge state and updated on decision ticks after `UpdateMemory()`.

## Rationale

- **Same pattern as FogOfWarGrid**: flat `std::vector<Cell>` sized `Width * Height`, indexed by `TileCoord`. Proven to work in the headless build and replay system.
- **Deterministic**: all arithmetic is integer-only (no float, no sqrt). Same seed → same threat/value maps → same decisions → same match outcome.
- **Engine-free**: reads SimWorld read-only through `EnemyMemory` and `ContentDatabase`. No dependency on Unreal, rendering, or audio.
- **Composable**: ThreatMap feeds into ValueMap's `FindBestAttackTarget()`, both feed into AICommander's target selection, and both are available for the future Director subsystem and Battle Predictor.
- **Confidence-weighted**: stale sightings contribute less threat/value, preventing the AI from reacting to ghosts.

## Consequences

- `AICommander::FindKnownEnemyTarget()` now includes a spatial awareness bonus from ValueMap (strategic value / 100), making the AI prefer high-value targets over arbitrary nearest-enemy logic.
- `AICommander::UpdateKnowledge()` recomputes both maps after each memory update cycle.
- New files: `ThreatMap.h`, `ThreatMap.cpp`, `ValueMap.h`, `ValueMap.cpp` in the `RA4AI` module.
- New unit tests: `ThreatMap` and `ValueMap` suites in `TestAI.cpp` (13 tests total).
- CMakeLists.txt updated to compile new source files.

## Status

**ACCEPTED / IMPLEMENTED**.
