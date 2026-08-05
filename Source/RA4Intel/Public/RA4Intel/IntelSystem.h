// Copyright (c) Red Alert 4 project. Orchestrator for the unreliable-intelligence layer.
//
// Runs as one system inside SimWorld::Tick, immediately after fog of war (fog
// answers "what is physically visible", intel answers "what does the HQ believe").
// The phase order below is fixed and versioned: like SimWorld's system order it is
// part of the replay compatibility contract, changing it changes match results.
//
// M0 ships the skeleton: phases exist, run in order, and do nothing. That is
// deliberate -- the contract (order, ownership, serialization, checksum) must be
// stable and reviewed before any behaviour lands on top of it.
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "RA4Core/Ids.h"
#include "RA4Intel/IntelConfig.h"
#include "RA4Intel/IntelTypes.h"
#include "RA4Intel/PerceivedWorld.h"

#ifndef RA4INTEL_API
#define RA4INTEL_API
#endif

namespace RA4
{

class ByteWriter;
class ByteReader;
class Hash64;

namespace Intel
{

// Fixed phase pipeline. Kept as an enum so profiling counters and debug output
// can name phases without string duplication.
enum class Phase : uint8_t
{
    MoraleUpdate = 0,   // combat events -> Morale/Fatigue/Suppression
    Observation,        // fog + transforms -> raw observations
    Distortion,         // §4.3 pipeline (M2/M4)
    ReportEmission,     // observations -> reports with ArrivalTick
    Propagation,        // chain-of-command hops, delays, blackout (M3)
    Aggregation,        // merge reports into tracks, agreement/contest (M3)
    TrackUpdate,        // decay, stale marking, phantom refutation, GC
    Count,
};

constexpr int32_t kPhaseCount = int32_t(Phase::Count);
RA4INTEL_API const char* PhaseName(Phase P);

// Per-tick cost bookkeeping so the performance budget (≤0.8 ms per sim tick for
// the whole layer) is measured from day one, not asserted at M5.
struct PhaseStats
{
    int64_t LastTickMicroseconds[kPhaseCount] = {};
    int64_t TotalMicroseconds[kPhaseCount] = {};
    uint32_t TicksMeasured = 0;
};

class RA4INTEL_API IntelSystem
{
public:
    // Settings are owned by the content layer and outlive the system, matching how
    // SimWorld holds ContentDatabase. MaxPlayers perceived worlds are created; the
    // inactive ones stay empty and cost nothing.
    void Initialize(const IntelSettings* InSettings, int32_t MapWidthTiles, int32_t MapHeightTiles);
    void Reset();

    bool IsEnabled() const { return Settings != nullptr && Settings->bEnabled; }

    // Advances one tick. Called by SimWorld::SystemIntel with the current tick.
    // When the feature is disabled this returns immediately: zero cost, zero state,
    // classic RTS behaviour (§4.7 requirement).
    void Tick(TickIndex CurrentTick);

    // The only read surface for belief state. PlayerIdx must be < kMaxPlayers.
    // There is deliberately no mutable counterpart: belief is written only by
    // the phases of this system (INVARIANT 9, ADR-0026 review BLOCKER 2).
    const PerceivedWorld& GetPerceivedWorld(PlayerId PlayerIdx) const;

    // --- Determinism plumbing ------------------------------------------------
    void Serialize(ByteWriter& W) const;
    bool Deserialize(ByteReader& R);
    void FeedChecksum(Hash64& H) const;

    const PhaseStats& GetStats() const { return Stats; }

private:
    // Phase bodies. Empty in M0; each milestone fills its own and MUST NOT touch
    // the others (small reviewable packages, CLAUDE.md rule 10).
    void PhaseMoraleUpdate(TickIndex CurrentTick);
    void PhaseObservation(TickIndex CurrentTick);
    void PhaseDistortion(TickIndex CurrentTick);
    void PhaseReportEmission(TickIndex CurrentTick);
    void PhasePropagation(TickIndex CurrentTick);
    void PhaseAggregation(TickIndex CurrentTick);
    void PhaseTrackUpdate(TickIndex CurrentTick);

    const IntelSettings* Settings = nullptr;

    // One belief state per player slot. unique_ptr keeps PerceivedWorld movable
    // out of this header and the empty slots cheap.
    std::unique_ptr<PerceivedWorld> Worlds[kMaxPlayers];

    // Reports in flight, ordered by ArrivalTick (min-heap over a vector, M3).
    std::vector<IntelReport> InFlightReports;
    uint32_t NextReportId = 1;

    PhaseStats Stats;
};

} // namespace Intel
} // namespace RA4
