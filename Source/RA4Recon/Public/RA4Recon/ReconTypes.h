// Copyright (c) Red Alert 4 project. Data types for the Unreliable Intelligence layer.
//
// The player commands their *belief* about the battlefield, not the battlefield
// itself (ADR-0021, ADR-0026). These types describe that belief: what a unit
// observed, the report it sent up the chain of command, and the track the HQ map
// keeps for it. Everything here is deterministic simulation state: fixed-point
// only, no floats, no engine types, serialized and checksummed with the match.
#pragma once

#include <cstdint>
#include <vector>

#include "RA4Core/Fixed.h"
#include "RA4Core/Ids.h"
#include "RA4Core/Vector.h"

#include "RA4Recon/ReconCategories.h"

#ifndef RA4RECON_API
#define RA4RECON_API
#endif

namespace RA4
{
namespace Recon
{

// --- Handles ----------------------------------------------------------------

// Generational handle for a perceived track. An index alone is unsafe because
// track slots are recycled; a stale UI or AI reference to a released slot must
// read as "gone", not as whatever moved into the slot afterwards.
struct TrackId
{
    static constexpr uint32_t kInvalidIndex = 0xFFFFFFFFu;

    uint32_t Index = kInvalidIndex;
    uint32_t Generation = 0;

    constexpr bool IsValid() const { return Index != kInvalidIndex; }
    constexpr bool operator==(const TrackId& O) const { return Index == O.Index && Generation == O.Generation; }
    constexpr bool operator!=(const TrackId& O) const { return !(*this == O); }
};

// --- Per-entity components (owned by SimWorld's component vectors, M1) -------

// The ability of an entity to act as an intelligence source. Values come from
// the entity's JSON definition; the simulation never invents them.
struct ReconReporterComp
{
    Fixed Competence = Fixed::FromInt(1);   // 0..1: how accurately it observes
    Fixed Discipline = Fixed::FromInt(1);   // 0..1: how honestly it self-reports
    int32_t ObservationRadiusTiles = 0;     // 0 = uses the entity's vision range
    int32_t ReportIntervalTicks = 20;       // one report per second by default
    TickIndex LastReportTick = 0;
    // UNUSED since M3, kept only so a save written by an M2 build still reads.
    // Node attachment is computed per tick in SimWorld::SystemRecon and blackout is
    // derived from node power, so these two are dead weight that would mislead a
    // future contributor into wiring the wrong one (finding n2). Remove with the
    // next save-format break.
    uint16_t ChainNodeId = 0;
    bool bBlackout = false;
};

// Psychological state feeding the distortion model. Fear exaggerates, fatigue
// omits, broken units lie about themselves -- see the distortion pipeline (M2).
struct MoraleComp
{
    Fixed Morale = Fixed::FromInt(1);       // 0..1
    Fixed Fatigue = Fixed::Zero();          // 0..1
    Fixed Suppression = Fixed::Zero();      // 0..1
    int32_t TicksUnderFire = 0;
};

// --- Observation and report ---------------------------------------------------

// One thing one observer saw during one tick, before any distortion.
// Subject is the ground-truth entity and NEVER leaves the simulation core:
// reports and tracks carry only believed data (see PerceivedTrack).
struct Observation
{
    // Chain node of the observer that produced this (M3). Decides how long the
    // report takes to reach the staff map; kNoChainNode = outside the network.
    uint16_t ObserverNodeId = 0;
    EntityId Subject;                       // GT association, core-internal only
    ContentId ObservedClass;                // exact believed type; invalid when misidentified
    ObservedCategory Category = ObservedCategory::LightVehicle; // coarse believed category
    Vec2 ObservedPosition;
    int32_t ObservedCount = 1;
    TickIndex Tick = 0;
    Fixed Clarity = Fixed::Zero();          // 0..1, gate and error scale
    bool bPhantom = false;                  // fabricated by a panicking observer (M4)
    // Contact seen but not identified (clarity below the identify threshold, or a
    // future radar return): position without class. "Something is moving out there."
    bool bAnonymous = false;
};

// One report travelling up the chain of command. Created at emission time,
// applied to the HQ map when the simulation reaches ArrivalTick.
struct ReconReport
{
    uint32_t ReportId = 0;
    PlayerId OwnerPlayer = kInvalidPlayer;  // whose HQ map this report feeds
    EntityId Author;
    TickIndex EmitTick = 0;
    TickIndex ArrivalTick = 0;
    uint8_t HopsRemaining = 0;
    uint16_t NodeId = 0;                    // chain node that filed it (M3)
    Fixed Reliability = Fixed::FromInt(1);  // product of author competence and chain bias
    std::vector<Observation> Payload;       // pooled/reused by the in-flight queue
};

// --- Perceived track -----------------------------------------------------------

// How many report ids a track remembers for post-match explainability ("why did
// I believe this?"). A small fixed ring keeps the struct POD-sized and allocation
// free; the full report history lives in the match replay anyway.
constexpr uint32_t kTrackProvenanceSize = 4;

// One entry on the HQ map. This is the ONLY shape of enemy information that the
// UI and the AI commander are allowed to read. It deliberately has no EntityId
// and no phantom flag: both the track<->entity association and the "is this
// contact real" truth live in core-internal side tables inside PerceivedWorld,
// because a field in this struct IS the read surface -- a comment calling it
// internal would not make it internal (INVARIANT 10, ADR-0026 review BLOCKER 1).
struct PerceivedTrack
{
    TrackId Id;
    ContentId BelievedClass;                // invalid when only the category is known
    ObservedCategory BelievedCategory = ObservedCategory::LightVehicle;
    Vec2 BelievedPosition;
    Fixed PositionErrorRadius = Fixed::Zero();
    int32_t BelievedCountMin = 0;           // count is an interval, never one number
    int32_t BelievedCountMax = 0;
    TickIndex LastUpdateTick = 0;
    Fixed Confidence = Fixed::Zero();       // 0..1, decays over time
    uint8_t IndependentSourceCount = 0;
    // Chain node whose report last updated this track (M3). Lets aggregation tell
    // "a second source agrees" from "the same source repeating itself" -- only the
    // former is corroboration. Belief about our own sources, not ground truth.
    uint16_t LastReportNodeId = 0;
    // Tick this track was last charged for decay. The amortized sweep visits each
    // slot every few ticks, so the elapsed interval must be measured rather than
    // assumed from the budget (review M4). Simulation state: serialized and hashed.
    TickIndex LastDecayTick = 0;
    // Strength the most recent report claimed, as opposed to the interval shown to
    // the player. Contest detection compares claim against claim; comparing against
    // the widened interval let one early over-count poison the test permanently
    // (review M3). Belief about our own reporting, not ground truth.
    int32_t LastClaimedCount = 0;
    // Tick a fabricated contact first appeared, 0 if this track never was one.
    // The refutation deadline is measured from here: §4.5 requires that a phantom
    // ALWAYS has a path to being disproved, or players stop trusting the map and
    // the whole feature becomes noise. Simulation state: serialized and hashed.
    TickIndex PhantomBornTick = 0;
    bool bStale = false;                    // no fresh reports for a while
    bool bContested = false;                // independent sources disagree (ADR-0026)
    // Unidentified contact: BelievedClass is meaningless, UI shows "unknown".
    // Belief about our own knowledge, not ground truth -- fine on the read surface.
    bool bAnonymous = false;
    uint32_t ProvenanceReportIds[kTrackProvenanceSize] = {0, 0, 0, 0};
    uint8_t ProvenanceCount = 0;            // ring write cursor lives in ProvenanceCount % size

    bool bAlive = false;                    // slot occupancy, managed by PerceivedWorld
};

} // namespace Recon
} // namespace RA4
