#pragma once

#include "CoreMinimal.h"
#include "SimTypes.h"

/**
 * ADR-0028: Unified Event Telemetry Architecture
 * Stable, deterministic, versioned event schema extending ADR-0020 EconomyTickRecord.
 * All events are outputs only; they never mutate simulation state.
 * Determinism invariant: identical seeds + command streams produce bit-identical logs.
 */

namespace RA4
{

enum class ETelemetryEventType : uint16_t
{
    Invalid = 0,
    CommandIssued = 1,
    UnitCreated,
    UnitDestroyed,
    DamageDealt,
    EconomyTick,           // Payload re-uses EconomyTickRecord (ADR-0020)
    ResearchCompleted,
    MapControlChanged,
    ObjectiveUpdated,
    FactionStrategy,
    ProductionQueue,
    PathFailure,
    UnitStuck,
    FramePerf
};

struct FTelemetryEventHeader
{
    uint64_t   MatchId;
    TickIndex  Tick;
    uint64_t   WallTimeMs;
    uint8_t    PlayerId;       // 0..3 or 0xFF neutral/system
    ETelemetryEventType Type;
    uint16_t   PayloadSize;
    uint32_t   EventSeq;
};

static_assert(sizeof(FTelemetryEventHeader) == 32, "Telemetry header must be 32 bytes for alignment and determinism");

// Example payloads (fixed layout, little-endian). Variable data is encoded within PayloadSize.

struct FCommandIssuedPayload
{
    uint64_t CommandId;
    uint8_t  CommandKind;
    uint16_t TargetCount;
    // Followed by TargetCount * sizeof(EntityId) if needed
};

struct FUnitLifecyclePayload
{
    EntityId UnitId;
    uint16_t UnitTypeId;
    uint8_t  Cause;
    int32_t  Value;
};

struct FDamagePayload
{
    EntityId SourceId;
    EntityId TargetId;
    uint16_t WeaponId;
    int32_t  DamageAmount;
    uint8_t  DamageType;
    uint8_t  WasLethal;
};

struct FResearchPayload
{
    uint16_t TechId;
    TickIndex ResearchStartTick;
    TickIndex ResearchEndTick;
};

struct FMapControlPayload
{
    uint16_t RegionId;
    uint8_t  Owner;
    float    ControlPercent;
};

struct FPerfPayload
{
    uint32_t TickDurationUs;
    uint32_t StateHash;
    uint16_t EntityCount;
    uint16_t PathQueryCount;
};

// EconomyTick uses existing EconomyTickRecord (see ADR-0020)
// Other payloads (ProductionQueue, PathFailure, UnitStuck, FactionStrategy, ObjectiveUpdated) follow the same pattern.

} // namespace RA4