# ADR-0028: Unified Event Telemetry Architecture

> **Provenance note (2026-08-13)**: Extends ADR-0020 (EconomyTickRecord / EconomyMatchSummary) to a full match timeline event system. Preserves determinism invariant: identical seeds + command streams must produce bit-identical telemetry output.

## Context

ADR-0020 established per-player economic snapshots every 60 ticks plus a post-match EconomyMatchSummary. Production requires broader observability for:

- Commands issued (player and AI)
- Unit lifecycle (create / death / veterancy)
- Damage (dealt / taken / by type / by source)
- Research and tech timing
- Map control / vision / objective state
- Faction strategy metadata (doctrine choices, aggression profile)
- Production queue state and starvation
- Pathfinding failures and stuck-unit events
- Frame / perf metrics (tick duration, hash mismatches)

Existing match.json frame logs and MatchViewer tooling provide partial coverage. A single stable schema, dual-mode (dev full-fidelity vs optional anonymized prod), privacy toggle, and JSON/CSV export are required.

## Decision

### 1. Core Event Schema

All events share a fixed header and are stored as a flat append-only log.

```
enum class EventType : uint16_t {
  CommandIssued = 1,
  UnitCreated,
  UnitDestroyed,
  DamageDealt,
  EconomyTick,           // re-uses EconomyTickRecord payload
  ResearchCompleted,
  MapControlChanged,
  ObjectiveUpdated,
  FactionStrategy,
  ProductionQueue,
  PathFailure,
  UnitStuck,
  FramePerf
};

struct TelemetryEventHeader {
  uint64_t   MatchId;        // stable match identifier
  TickIndex  Tick;           // deterministic simulation tick
  uint64_t   WallTimeMs;     // wall-clock since match start (for human analysis)
  uint8_t    PlayerId;       // 0..3 or 0xFF for neutral/system
  EventType  Type;
  uint16_t   PayloadSize;    // bytes following header
  uint32_t   EventSeq;       // monotonic per-match sequence for ordering
};
```

Payloads are packed structs (little-endian, fixed layout, versioned by ADR revision). No pointers, no STL containers inside payloads.

Example payloads (abbreviated):

```
struct CommandIssuedPayload {
  uint64_t CommandId;
  uint8_t  CommandKind;      // move, attack, build, etc.
  uint16_t TargetCount;
  // variable-length target list follows if needed (encoded in PayloadSize)
};

struct UnitLifecyclePayload {
  EntityId UnitId;
  uint16_t UnitTypeId;       // from content bible
  uint8_t  Cause;            // created / destroyed / recycled
  int32_t  Value;            // build cost at time of event
};

struct DamagePayload {
  EntityId SourceId;
  EntityId TargetId;
  uint16_t WeaponId;
  int32_t  DamageAmount;
  uint8_t  DamageType;       // kinetic, energy, etc.
  uint8_t  WasLethal;
};

struct ResearchPayload {
  uint16_t TechId;
  TickIndex ResearchStartTick;
  TickIndex ResearchEndTick;
};

struct MapControlPayload {
  uint16_t RegionId;
  uint8_t  Owner;            // player or neutral
  float    ControlPercent;   // 0..100
};

struct PerfPayload {
  uint32_t TickDurationUs;
  uint32_t Hash;             // state hash after tick
  uint16_t EntityCount;
  uint16_t PathQueryCount;
};
```

Economy events reuse the existing `EconomyTickRecord` (ADR-0020) as the payload for `EconomyTick` events. Recording frequency remains every 60 ticks.

### 2. Recording & Collectors

- Telemetry is an **output-only** subsystem. It never mutates simulation state.
- Collectors are registered at simulation start via a central `TelemetryHub`.
- Public Economy API (already defined) is the single source of truth for all economy fields; collectors read from it rather than duplicating logic.
- Determinism guarantee: collectors are pure functions of simulation state + command stream. Two runs with identical inputs produce identical event logs (byte-for-byte).

Collector hooks (implemented in later packages):

- `CommandBus` → `CommandIssued`
- `SimWorld` entity creation/destruction → `UnitCreated` / `UnitDestroyed`
- Combat resolver → `DamageDealt`
- Research queue → `ResearchCompleted`
- Vision / fog system → `MapControlChanged`
- Objective manager → `ObjectiveUpdated`
- AI doctrine system → `FactionStrategy`
- Production manager → `ProductionQueue`
- Navigation system → `PathFailure` / `UnitStuck`
- `SimWorld::Tick()` → `FramePerf` + state hash

### 3. Storage & Export

File naming: `{MatchId}_telemetry_v1.bin` (binary, compact) and optional sidecar `{MatchId}_telemetry.json` / `.csv`.

Binary format mirrors ADR-0020:
- 16-byte global header (magic, version=1, player count, event count)
- Per-player or global event stream (header + payload repeated)
- Optional summary block at end (see below)

JSON export (human-readable, indented, stable key order):

```json
{
  "match_id": "m20260813_001",
  "schema_version": 1,
  "events": [
    {"tick": 120, "wall_ms": 6000, "player": 0, "type": "EconomyTick", "payload": { ... }},
    {"tick": 121, "wall_ms": 6050, "player": 1, "type": "CommandIssued", "payload": { ... }}
  ]
}
```

CSV export uses a wide table with columns for common header fields plus flattened payload fields (null for inapplicable columns). One row per event.

### 4. Post-Match Analytics

A `MatchAnalytics` struct is computed from the event log at match end (or offline):

```
struct MatchAnalytics {
  // Losses & value
  int32_t TotalUnitsLost[4];
  int32_t TotalValueLost[4];
  int32_t DamageDealt[4];
  int32_t DamageTaken[4];
  float   DamageEfficiency[4];   // dealt / taken ratio

  // Build & tech timing
  TickIndex FirstBuildTick[4][MaxUnitTypes];
  TickIndex Tech2Tick[4];
  TickIndex Tech3Tick[4];

  // Economy (extended from ADR-0020)
  EconomyMatchSummary Economy[4];

  // Map control timeline (sampled every 300 ticks)
  struct MapControlSample { TickIndex Tick; float ControlPercent[4]; };
  std::vector<MapControlSample> MapTimeline;

  // Army value timeline (sampled every 60 ticks)
  struct ArmyValueSample { TickIndex Tick; int32_t Value[4]; };
  std::vector<ArmyValueSample> ArmyTimeline;

  // Path & stuck metrics
  int32_t TotalPathFailures[4];
  int32_t TotalStuckEvents[4];
  float   AveragePathRetryCount;

  // Perf
  float AverageTickDurationUs;
  uint32_t MaxTickDurationUs;
  uint32_t DesyncCount;          // hash mismatches detected
};
```

These aggregates feed dashboards (MatchViewer extension) and balance regression tests.

### 5. Privacy & Production Toggle

Telemetry operates in three modes (configurable per match or via project setting):

- **Dev** (default in editor / headless test runs): full fidelity, all events, player identifiers, exact positions if needed for debugging.
- **ProdAnonymized** (opt-in, default off): 
  - No persistent player IDs or names.
  - Faction and aggregated statistics only.
  - No coordinate-level position data.
  - Events that could identify a specific user (chat, custom names) are stripped.
  - Data is used only for aggregate balance and performance reporting.
- **Off**: no telemetry written.

A single boolean `bTelemetryEnabled` and enum `TelemetryPrivacyMode` gate all collectors. Production builds default to Off; players must explicitly opt in via settings.

No invasive user tracking (no hardware IDs, no IP logging inside simulation telemetry).

### 6. Integration Points

- Headless simulator (`SimHeadless`) instantiates `TelemetryHub` when `--telemetry` flag is present.
- MatchViewer (`dump_match.cpp`, `render.py`) extended to read the new unified binary/JSON format and render timelines.
- CI pipeline compares telemetry hashes between platforms for determinism verification (extending ADR-0020 rule).
- Future: optional live telemetry overlay in-game (dev only).

### 7. Performance & Memory Budget

- Event allocation: pre-allocated ring buffer (configurable size, default 64 MiB per match).
- Write cost: < 0.2 µs per event (memcpy into buffer).
- File I/O: buffered, flushed only at match end.
- Typical match (30 min, 4 players): ~15–25 MiB binary. Acceptable for dev analysis; ProdAnonymized mode further reduces volume by aggregation.

## Rationale

- Single schema eliminates fragmentation between economic-only (ADR-0020), frame logs (match.json), and ad-hoc debug prints.
- Event-based design naturally supports both real-time dashboards and offline analysis.
- Explicit privacy modes satisfy "no invasive user tracking" and "production telemetry toggleable" requirements.
- Determinism invariant is preserved because telemetry is a pure observation of the command + seed stream.
- Re-use of EconomyTickRecord avoids duplication and keeps existing balance tooling compatible.

## Status

**ACCEPTED**. Implementation will proceed in worktree `telemetry-unified` as SUBAGENT 12 packages (schema library, collectors, exporters, MatchViewer integration, documentation). No changes land in `main` until independent review and full test passage.