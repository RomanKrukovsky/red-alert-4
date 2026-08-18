# ADR-0028 Implementation Plan (Telemetry Unified)

## Packages (independent worktrees)

1. **telemetry-schema** (this worktree)
   - Files created:
     - `Source/RA4Simulation/Public/RA4Simulation/TelemetryEvent.h`
     - `Source/RA4Simulation/Public/RA4Simulation/TelemetryHub.h`
     - `Source/RA4Simulation/Public/RA4Simulation/MatchAnalytics.h`
     - `Docs/Architecture/ADR/ADR-0028-unified-event-telemetry-architecture.md`
   - Status: Schema + hub interfaces defined. No implementation yet.

2. **telemetry-collectors** (next)
   - CommandBus collector
   - SimWorld entity lifecycle collector
   - Combat damage collector
   - Research queue collector
   - Vision / map control collector
   - Production / economy collector (wraps existing Economy API)
   - Pathfinding & stuck-event collector
   - Perf / frame collector
   - All collectors must be pure (no side effects) and deterministic.

3. **telemetry-exporters**
   - Binary writer (ring buffer → {MatchId}_telemetry_v1.bin)
   - JSON exporter (stable key order, indented)
   - CSV exporter (wide table)
   - Privacy filter (ProdAnonymized mode strips PII and coordinates)

4. **telemetry-analytics**
   - MatchAnalytics computation from event log
   - Timeline sampling (map control, army value)
   - Efficiency and timing metrics

5. **telemetry-dashboards**
   - Extend MatchViewer (dump_match.cpp + render.py) to read new format
   - Timeline charts, efficiency heatmaps, tech timing table
   - Regression dashboard for CI

## Determinism Verification

- CI compares telemetry hash between Windows / macOS / Linux for identical seeds.
- Any difference in EconomyTickRecord or StateHash at same tick fails the build.

## Privacy & Toggle

- Single project setting `Telemetry.PrivacyMode` (Off / ProdAnonymized / DevFull)
- Production defaults to Off.
- No hardware IDs or persistent player identifiers in anonymized mode.

## Next Immediate Steps (after schema review)

- Implement TelemetryHub ring buffer + file writer (deterministic, < 0.2 µs per event)
- Wire CommandBus and SimWorld collectors (first two event types)
- Add JSON export and verify round-trip with MatchViewer
- Independent review by another agent before any main merge

All packages follow CLAUDE.md: worktree, no TODOs in release, full test passage, independent review.