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
#include "RA4Recon/ReconConfig.h"
#include "RA4Core/Random.h"
#include "RA4Recon/ReconTypes.h"
#include "RA4Recon/PerceivedWorld.h"

#ifndef RA4RECON_API
#define RA4RECON_API
#endif

namespace RA4
{

class ByteWriter;
class ByteReader;
class Hash64;

namespace Recon
{

// What the simulation shows the intel layer each tick. SimWorld owns entity
// storage and fog; RA4Recon must not depend on RA4Simulation (the dependency
// points the other way), so the visible-entity view crosses the boundary as
// plain data. The buffer is owned by SimWorld and reused across ticks -- no
// per-tick allocation in steady state.
struct ObservedEntity
{
    EntityId Id;            // ground truth; never escapes the recon core
    ContentId Class;
    Vec2 Position;
    int32_t TileX = 0;
    int32_t TileY = 0;
    ObservedCategory Category = ObservedCategory::LightVehicle; // for the confusion matrix
    // Radar return, not eyes-on: becomes an anonymous observation (position
    // without identity) and skips the visual distortion stages.
    bool bRadarContact = false;

    // Chain node the OBSERVER is attached to (M3). kNoChainNode means the
    // observer is outside the command network and its report walks in on foot.
    // Radar contacts report through the radar building's own node.
    uint16_t ObserverNodeId = 0;
};

// One friendly observer this tick. M2 uses per-PLAYER aggregate observers (one
// virtual observer per player, the mean state of its live units); per-unit
// observers land with M3's report chains, where authorship starts to matter.
struct ObserverSnapshot
{
    Fixed Competence = Fixed::FromInt(1);
    Fixed Morale = Fixed::FromInt(1);
    Fixed Fatigue = Fixed::Zero();
    Fixed Suppression = Fixed::Zero();
    // Whether any of this player's units is currently in contact, feeding the
    // fabrication gate (M4).
    bool bAnyUnitUnderFire = false;
};

// One node of a player's command chain (M3, §4.4). Built each tick from the
// player's completed command buildings, so losing a headquarters really does
// break the reporting structure that depended on it -- the mechanic is
// structural, not a status effect.
struct ChainNode
{
    uint16_t NodeId = 0;                // 1-based; 0 (kNoChainNode) means "none"
    PlayerId Owner = kInvalidPlayer;
    int32_t TileX = 0;
    int32_t TileY = 0;
    bool bIsHq = false;                 // construction yard: the top of the chain
    bool bBlackout = false;             // comms cut: forwards nothing, freezes belief
};

// NodeId 0 is reserved for "attached to no node".
constexpr uint16_t kNoChainNode = 0;

struct ObservationInput
{
    // Per viewing player: hostile/neutral entities their fog can currently see,
    // in ascending entity-slot order (deterministic by construction).
    std::vector<ObservedEntity> VisibleToPlayer[kMaxPlayers];
    // Per-player aggregate observer state (see ObserverSnapshot).
    ObserverSnapshot Observers[kMaxPlayers];
    // Where a fabricated contact can plausibly appear when the player currently
    // sees nothing: the position of one of its own front-line units. Without an
    // anchor a phantom would need a map-wide random position, which reads as a bug
    // rather than as fear (M4).
    Vec2 ObserverAnchor[kMaxPlayers];
    bool ObserverAnchorValid[kMaxPlayers] = {};
    // Tiles the player's fog shows RIGHT NOW, per player, in ascending order. This
    // is a different question from VisibleToPlayer, which lists what was SEEN:
    // phantom refutation (§4.5) needs "someone is looking there and there is
    // nothing", so it must know about empty-but-watched tiles too. Filled by
    // SimWorld from the fog grid, bounded to tiles near existing belief so the
    // list stays small.
    std::vector<uint32_t> VisibleTilesPacked[kMaxPlayers];
    // Per-player command chain for this tick, in ascending NodeId order.
    std::vector<ChainNode> ChainNodes[kMaxPlayers];
    // Whether the player has any HQ node at all. No HQ means no staff map to
    // report TO: reports still travel, but they arrive at orphan latency.
    bool HasHqNode[kMaxPlayers] = {};
    // Entity slot capacity, so per-entity association tables can size once.
    uint32_t EntityCapacity = 0;

    void Clear()
    {
        for (int32_t P = 0; P < kMaxPlayers; ++P)
        {
            VisibleToPlayer[P].clear();
            Observers[P] = ObserverSnapshot{};
            ChainNodes[P].clear();
            HasHqNode[P] = false;
            ObserverAnchorValid[P] = false;
            VisibleTilesPacked[P].clear();
        }
    }
};

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
RA4RECON_API const char* PhaseName(Phase P);

// Per-tick cost bookkeeping so the performance budget (≤0.8 ms per sim tick for
// the whole layer) is measured from day one, not asserted at M5.
struct PhaseStats
{
    int64_t LastTickMicroseconds[kPhaseCount] = {};
    int64_t TotalMicroseconds[kPhaseCount] = {};
    uint32_t TicksMeasured = 0;
};

class RA4RECON_API ReconSystem
{
public:
    // Settings are owned by the content layer and outlive the system, matching how
    // SimWorld holds ContentDatabase. MaxPlayers perceived worlds are created; the
    // inactive ones stay empty and cost nothing.
    void Initialize(const ReconSettings* InSettings, int32_t MapWidthTiles, int32_t MapHeightTiles);
    void Reset();

    bool IsEnabled() const { return Settings != nullptr && Settings->bEnabled; }

    // Advances one tick. Called by SimWorld::SystemRecon with the current tick
    // and this tick's visibility view. When the feature is disabled this returns
    // immediately: zero cost, zero state, classic RTS behaviour (§4.7).
    void Tick(TickIndex CurrentTick, const ObservationInput& Input, Random& Rng);

    // The only read surface for belief state. PlayerIdx must be < kMaxPlayers.
    // There is deliberately no mutable counterpart: belief is written only by
    // the phases of this system (INVARIANT 9, ADR-0026 review BLOCKER 2).
    const PerceivedWorld& GetPerceivedWorld(PlayerId PlayerIdx) const;

    // --- Determinism plumbing ------------------------------------------------
    void Serialize(ByteWriter& W) const;
    bool Deserialize(ByteReader& R);
    void FeedChecksum(Hash64& H) const;

    const PhaseStats& GetStats() const { return Stats; }

    // Test-only window on the last observer snapshot the layer consumed. Read-only,
    // and it exposes only the player's OWN aggregate state -- no enemy data.
    const ObserverSnapshot& GetLastObserverForTest(PlayerId P) const { return LastObserver[P]; }

    // Test-only audit of a DERIVED index against the data it indexes. Derived state
    // that silently disagrees with its source is the worst failure mode in this
    // layer: queries answer wrongly while checksums still match.
    bool ReverseIndexMatchesForwardTableForTest(PlayerId P) const;

private:
    // Phase bodies. Filled milestone by milestone; a milestone MUST NOT touch
    // phases it does not own (small reviewable packages, CLAUDE.md rule 10).
    // M1 owns Observation/ReportEmission/Aggregation/TrackUpdate (truthful path).
    void PhaseMoraleUpdate(TickIndex CurrentTick);
    void PhaseObservation(TickIndex CurrentTick, const ObservationInput& Input);
    void PhaseDistortion(TickIndex CurrentTick);
    void PhaseReportEmission(TickIndex CurrentTick);
    void PhasePropagation(TickIndex CurrentTick);
    void PhaseAggregation(TickIndex CurrentTick);
    void PhaseTrackUpdate(TickIndex CurrentTick);

    // Folds one arrived report into belief with M3 grouping: nearby same-category
    // observations become one track carrying a count interval, and a second
    // independent node either corroborates (confidence up) or contests it.
    void ApplyGroupedReport(PerceivedWorld& World, PlayerId P, const ReconReport& Report,
                            TickIndex CurrentTick);

    // Releases a track and clears every association pointing at it. One place, so
    // GC and phantom refutation cannot drift apart.
    void ReleaseTrackAndAssociations(PlayerId P, PerceivedWorld& World, TrackId Id);

    // Whether a friendly observer is looking at this position RIGHT NOW and sees
    // nothing real there -- the player-driven half of phantom refutation (§4.5).
    bool IsTileObservedAndEmpty(PlayerId P, const Vec2& Position, TickIndex CurrentTick) const;

    // Grows the per-player association tables to cover EntityCapacity slots.
    void EnsureAssociationCapacity(uint32_t NewEntityCapacity);

    const ReconSettings* Settings = nullptr;

    // One belief state per player slot. unique_ptr keeps PerceivedWorld movable
    // out of this header and the empty slots cheap.
    std::unique_ptr<PerceivedWorld> Worlds[kMaxPlayers];

    // Reports in flight, ordered by ArrivalTick (min-heap over a vector, M3).
    std::vector<ReconReport> InFlightReports;
    uint32_t NextReportId = 1;

    // This tick's truthful observations per player, produced by PhaseObservation
    // and consumed by PhaseReportEmission. Member (not local) so capacity
    // persists across ticks -- no steady-state allocation.
    std::vector<Observation> PendingObservations[kMaxPlayers];
    // TRUE categories ride in a parallel array: they are the input to the
    // confusion roll and must survive next to the (rewritten) observation.
    std::vector<ObservedCategory> PendingCategories[kMaxPlayers];

    // Tick-scoped borrows from SimWorld; never outlive Tick(). Raw pointers by
    // design: storing them longer would be a lifetime bug, and phases already
    // cannot run outside Tick().
    Random* TickRng = nullptr;
    const ObservationInput* TickInput = nullptr;

    // track<->entity association, per player, indexed by GT entity slot. This is
    // exactly the association INVARIANT 10 forbids on the read surface, which is
    // why it lives here and not in PerceivedTrack. The generation table detects
    // GT slot reuse: a recycled slot must get a NEW track, while the old track
    // freezes as last-known-position -- the HQ has no idea the old unit is gone.
    std::vector<TrackId> AssociationTrack[kMaxPlayers];
    std::vector<uint32_t> AssociationGeneration[kMaxPlayers];

    // Reverse index: track SLOT -> the entity slots currently associated with it
    // (M3-perf, review finding M1/M2). Without it, releasing a track meant scanning
    // every entity slot to find the associations pointing at it, and a decay
    // avalanche -- which blackout causes by design, since it raises decay for every
    // track at once -- turned that into tens of millions of comparisons in one tick.
    //
    // Derived state: rebuilt from the association tables on load, never serialized,
    // so there is exactly one source of truth.
    std::vector<std::vector<uint32_t>> AssociationsByTrackSlot[kMaxPlayers];
    void AssociationLink(PlayerId P, uint32_t EntitySlot, TrackId Id);
    void AssociationUnlink(PlayerId P, uint32_t EntitySlot);
    void RebuildAssociationReverseIndex();
    uint32_t EntityCapacity = 0;

    // Scratch for grouping this tick's observations by reporting node. Members
    // rather than locals so capacity survives across ticks (no steady-state
    // allocation, §6). Cleared per player inside PhaseReportEmission.
    std::vector<uint16_t> NodeBatchIds;

    // One bucket of nearby same-category observations from a single report.
    // Tick-scoped scratch; a member so its capacity survives (no steady-state
    // allocation in the hot path, §6).
    struct ObservationGroup
    {
        ObservedCategory Category = ObservedCategory::LightVehicle;
        bool bAnonymous = false;
        Vec2 Centre;
        Fixed SumX;                 // running coordinate sums for the centroid
        Fixed SumY;
        int32_t Count = 0;          // believed strength of the whole group
        int32_t Members = 0;        // observations folded in (for the centroid)
        uint32_t RepresentativeSlot = 0;
        uint32_t RepresentativeGeneration = 0;
        ContentId ObservedClass;    // invalid once the group is mixed
        bool bPhantom = false;      // every member was fabricated (M4)
    };
    std::vector<ObservationGroup> GroupScratch;

    // Last observer snapshot per player, kept for diagnostics and tests.
    ObserverSnapshot LastObserver[kMaxPlayers];

    PhaseStats Stats;
};

} // namespace Recon
} // namespace RA4
