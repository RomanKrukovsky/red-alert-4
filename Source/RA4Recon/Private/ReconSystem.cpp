// Copyright (c) Red Alert 4 project.
#include "RA4Recon/ReconSystem.h"

#include "RA4Core/ByteStream.h"

#include <chrono>
#include <cstdio>
#include "RA4Core/Checksum.h"
#include "RA4Core/SimConfig.h"
#include "RA4Recon/DistortionPipeline.h"

namespace RA4
{
namespace Recon
{

namespace
{
constexpr uint32_t kReconSystemVersion = 5; // v5: OwnerPlayer + ObserverNodeId serialized (M3 review B1/B2) // v4: report NodeId, chain latency (M3); v3: category/anonymous (M2); v2: association tables (M1)
} // namespace

const char* PhaseName(Phase P)
{
    switch (P)
    {
        case Phase::MoraleUpdate: return "MoraleUpdate";
        case Phase::Observation: return "Observation";
        case Phase::Distortion: return "Distortion";
        case Phase::ReportEmission: return "ReportEmission";
        case Phase::Propagation: return "Propagation";
        case Phase::Aggregation: return "Aggregation";
        case Phase::TrackUpdate: return "TrackUpdate";
        default: return "Unknown";
    }
}

void ReconSystem::Initialize(const ReconSettings* InSettings, int32_t MapWidthTiles, int32_t MapHeightTiles)
{
    Reset();
    Settings = InSettings;
    if (!IsEnabled())
    {
        // Disabled means genuinely absent: no per-player worlds are allocated and
        // Tick returns immediately. This is the §4.7 kill switch -- the rest of the
        // game must behave exactly as it did before this module existed.
        return;
    }
    for (int32_t P = 0; P < kMaxPlayers; ++P)
    {
        Worlds[P] = std::make_unique<PerceivedWorld>();
        Worlds[P]->Initialize(MapWidthTiles, MapHeightTiles, Settings->Tracks.MaxTracksPerPlayer);
    }
}

void ReconSystem::Reset()
{
    Settings = nullptr;
    for (int32_t P = 0; P < kMaxPlayers; ++P)
    {
        Worlds[P].reset();
    }
    InFlightReports.clear();
    NextReportId = 1;
    for (int32_t P = 0; P < kMaxPlayers; ++P)
    {
        PendingObservations[P].clear();
        AssociationTrack[P].clear();
        AssociationGeneration[P].clear();
    }
    EntityCapacity = 0;
    Stats = PhaseStats{};
}

void ReconSystem::Tick(TickIndex CurrentTick, const ObservationInput& Input, Random& Rng)
{
    if (!IsEnabled())
    {
        return;
    }

    TickRng = &Rng;
    TickInput = &Input;
    EnsureAssociationCapacity(Input.EntityCapacity);

    // Fixed phase order -- part of the replay compatibility contract, do not
    // reorder without a format version bump (same rule as SimWorld::Tick).
    //
    // Phase timing fills PhaseStats, which existed since M0 but was never
    // written (found by the P-7 benchmark reading zeros). Wall-clock reads are
    // observation only -- nothing here feeds back into simulation state, so
    // determinism is untouched.
    const auto TimePhase = [this](Phase Ph, auto&& Fn)
    {
        const auto T0 = std::chrono::steady_clock::now();
        Fn();
        const auto T1 = std::chrono::steady_clock::now();
        const int64_t Us = std::chrono::duration_cast<std::chrono::microseconds>(T1 - T0).count();
        Stats.LastTickMicroseconds[int32_t(Ph)] = Us;
        Stats.TotalMicroseconds[int32_t(Ph)] += Us;
    };
    TimePhase(Phase::MoraleUpdate, [&] { PhaseMoraleUpdate(CurrentTick); });
    TimePhase(Phase::Observation, [&] { PhaseObservation(CurrentTick, Input); });
    TimePhase(Phase::Distortion, [&] { PhaseDistortion(CurrentTick); });
    TimePhase(Phase::ReportEmission, [&] { PhaseReportEmission(CurrentTick); });
    TimePhase(Phase::Propagation, [&] { PhasePropagation(CurrentTick); });
    TimePhase(Phase::Aggregation, [&] { PhaseAggregation(CurrentTick); });
    TimePhase(Phase::TrackUpdate, [&] { PhaseTrackUpdate(CurrentTick); });

    Stats.TicksMeasured += 1;
}

const PerceivedWorld& ReconSystem::GetPerceivedWorld(PlayerId PlayerIdx) const
{
    // A caller asking for belief state while the feature is disabled is a logic
    // error upstream; returning an empty static keeps the read surface total.
    static const PerceivedWorld Empty;
    if (PlayerIdx >= kMaxPlayers || Worlds[PlayerIdx] == nullptr)
    {
        return Empty;
    }
    return *Worlds[PlayerIdx];
}

// --- Phases ---------------------------------------------------------------------
//
// M1 implements the TRUTHFUL pipeline: what fog sees becomes an observation,
// the observation becomes a report that arrives the same tick, and the report
// becomes/updates a track that mirrors ground truth exactly. Zero distortion,
// zero delay -- the baseline every distortion milestone is measured against,
// and the proof that the architecture moves data end to end at all.

void ReconSystem::EnsureAssociationCapacity(uint32_t NewEntityCapacity)
{
    if (NewEntityCapacity <= EntityCapacity)
    {
        return;
    }
    EntityCapacity = NewEntityCapacity;
    for (int32_t P = 0; P < kMaxPlayers; ++P)
    {
        if (Worlds[P] != nullptr)
        {
            AssociationTrack[P].resize(EntityCapacity, TrackId{});
            AssociationGeneration[P].resize(EntityCapacity, 0);
        }
    }
}

void ReconSystem::PhaseMoraleUpdate(TickIndex)
{
    // M2 territory (fear/fatigue inputs to distortion). Deliberately empty.
}

void ReconSystem::PhaseObservation(TickIndex CurrentTick, const ObservationInput& Input)
{
    // Truthful observation: every entity the player's fog currently sees becomes
    // one observation with perfect clarity. Distortion (M2) will transform these
    // in PhaseDistortion; this phase must stay honest so the disable-flags path
    // always has a truth to fall back to.
    for (int32_t P = 0; P < kMaxPlayers; ++P)
    {
        if (Worlds[P] == nullptr)
        {
            continue;
        }
        PendingObservations[P].clear();
        PendingCategories[P].clear();
        for (const ObservedEntity& Seen : Input.VisibleToPlayer[P])
        {
            Observation Obs;
            Obs.ObserverNodeId = Seen.ObserverNodeId; // M3: who is reporting this
            Obs.Subject = Seen.Id;
            Obs.ObservedClass = Seen.Class;
            Obs.ObservedPosition = Seen.Position;
            Obs.ObservedCount = 1;
            Obs.Tick = CurrentTick;
            Obs.Clarity = Fixed::FromInt(1);
            Obs.Category = Seen.Category;
            if (Seen.bRadarContact)
            {
                // A radar return is a position, not an identification. Anonymous
                // from birth; identity can only come from a later visual contact.
                Obs.bAnonymous = true;
                Obs.ObservedClass = ContentId();
            }
            PendingObservations[P].push_back(Obs);
            PendingCategories[P].push_back(Seen.Category);

            Worlds[P]->SetLastObservedTick(Seen.TileX, Seen.TileY, CurrentTick);
        }
    }
}

void ReconSystem::PhaseDistortion(TickIndex CurrentDistortionTick)
{
    // Stages 1-5 of ADR-0026 §4.3, per observation, all math on Fixed and the
    // isolated ReconRng. The truthful M1 path is exactly this function with the
    // active profile's stages disabled -- there is no second code path to drift.
    const DistortionProfile* Profile = Settings->FindDistortionProfile(Settings->ActiveDistortionProfile);
    if (Profile == nullptr || TickRng == nullptr || TickInput == nullptr)
    {
        return; // validated at load; belt and braces at runtime
    }

    const Fixed MinClarity = PerMilleToFixed(Profile->MinClarityPerMille);
    const Fixed IdentifyClarity = PerMilleToFixed(Profile->IdentifyClarityPerMille);

    for (int32_t P = 0; P < kMaxPlayers; ++P)
    {
        // NOT skipped when the player sees nothing: fabrication (stage 6, below)
        // exists precisely for the case where a frightened unit reports a contact
        // where there is none, and an early-out on an empty observation list made
        // the whole stage unreachable in exactly that situation -- found by the
        // refutation test never producing a phantom.
        if (Worlds[P] == nullptr)
        {
            continue;
        }

        // One virtual observer per player in M2 (mean of its live units);
        // per-unit authorship arrives with M3's report chains.
        ObserverState Observer;
        Observer.Competence = TickInput->Observers[P].Competence;
        Observer.Morale = TickInput->Observers[P].Morale;
        Observer.Fatigue = TickInput->Observers[P].Fatigue;
        Observer.Suppression = TickInput->Observers[P].Suppression;
        Observer.DistanceRatio = Fixed::FromRatio(1, 2); // aggregate: mid-range until M3 geometry

        size_t Write = 0;
        for (size_t I = 0; I < PendingObservations[P].size(); ++I)
        {
            Observation Obs = PendingObservations[P][I];
            const ObservedCategory TrueCategory = PendingCategories[P][I];

            if (Obs.bAnonymous)
            {
                // Radar contact: machines do not panic and do not misname what
                // they cannot name. Position is already coarse (the blip), so
                // the visual stages 1-3/5 do not apply; keep it as-is for M2.
                PendingObservations[P][Write] = Obs;
                PendingCategories[P][Write] = TrueCategory;
                Write += 1;
                continue;
            }

            // Stage 1: clarity gate. Below MinClarity the observation dies here.
            const Fixed Clarity = StageClarity(Observer, *Profile);
            if (Clarity < MinClarity)
            {
                continue;
            }
            Obs.Clarity = Clarity;

            // Stage 2: numbers, inflated by fear.
            Obs.ObservedCount = StageCountDistortion(Obs.ObservedCount, Observer, *Profile, *TickRng);

            // Stage 3: identity, rolled against the confusion matrix.
            const ObservedCategory Rolled =
                StageClassification(TrueCategory, Clarity, Settings->Confusion, *Profile, *TickRng);
            Obs.Category = Rolled;
            if (Rolled != TrueCategory)
            {
                // Misidentified: the observer names a CATEGORY he believes
                // ("heavy armour!") but no exact type. An invalid class id is the
                // honest encoding -- inventing a concrete wrong ContentId would
                // leak content-table knowledge the observer does not have.
                Obs.ObservedClass = ContentId();
            }

            // Stage 4: position, smeared inside the error circle.
            const Vec2 Offset = StagePositionError(Clarity, Observer, *Profile, *TickRng);
            Obs.ObservedPosition = Vec2(Obs.ObservedPosition.X + Offset.X, Obs.ObservedPosition.Y + Offset.Y);

            // Stage 5: or the sighting is simply never written down.
            if (StageOmission(Clarity, Observer, *Profile, *TickRng))
            {
                continue;
            }

            // Identify threshold: a contact seen too dimly to name stays a
            // contact -- "something is moving out there".
            if (Clarity < IdentifyClarity)
            {
                Obs.bAnonymous = true;
            }

            PendingObservations[P][Write] = Obs;
            PendingCategories[P][Write] = TrueCategory;
            Write += 1;
        }
        PendingObservations[P].resize(Write);
        PendingCategories[P].resize(Write);

        // --- Stage 6: fabrication (M4) --------------------------------------------
        // A shaken unit still under fire invents a contact. Appended AFTER the real
        // observations so the loop above never sees it: a phantom must not be
        // distorted twice, and it has no true category to misidentify.
        //
        // At most one phantom per player per tick. The cap is not thrift, it is
        // readability: a map that sprouts five imaginary companies in one second
        // reads as a bug, and §4.6 puts readability above realism.
        Observer.bIsUnderFire = TickInput->Observers[P].bAnyUnitUnderFire;
        LastObserver[P] = TickInput->Observers[P];
        if (StageFabrication(Observer, *Profile, *TickRng))
        {
            // Anchor the phantom on something the player's own side can plausibly
            // be looking at. Without an anchor a phantom would need a map-wide
            // random position, which reads as noise rather than as fear.
            Vec2 Anchor;
            bool bHaveAnchor = false;
            if (!PendingObservations[P].empty())
            {
                Anchor = PendingObservations[P].front().ObservedPosition;
                bHaveAnchor = true;
            }
            else if (TickInput->ObserverAnchorValid[P])
            {
                Anchor = TickInput->ObserverAnchor[P];
                bHaveAnchor = true;
            }

            if (bHaveAnchor)
            {
                const Vec2 Offset = StageFabricationOffset(Observer, *Profile, *TickRng);
                Observation Ghost;
                Ghost.ObserverNodeId = PendingObservations[P].empty()
                                           ? kNoChainNode
                                           : PendingObservations[P].front().ObserverNodeId;
                Ghost.Subject = EntityId{};       // nothing real underneath
                Ghost.ObservedClass = ContentId();
                Ghost.ObservedPosition = Vec2(Anchor.X + Offset.X, Anchor.Y + Offset.Y);
                Ghost.ObservedCount = 1;
                Ghost.Tick = CurrentDistortionTick;
                Ghost.Clarity = PerMilleToFixed(Profile->MinClarityPerMille);
                Ghost.bPhantom = true;
                // A phantom is BY DEFINITION unidentified: the observer saw
                // movement, not a vehicle. Claiming a category would be inventing
                // detail nobody perceived.
                Ghost.bAnonymous = true;
                Ghost.Category = ObservedCategory::LightVehicle; // unused while anonymous
                PendingObservations[P].push_back(Ghost);
                PendingCategories[P].push_back(ObservedCategory::LightVehicle);
            }
        }
    }
}

void ReconSystem::PhaseReportEmission(TickIndex CurrentTick)
{
    // M3: observations are grouped BY REPORTING NODE, and each group becomes one
    // report whose arrival is pushed into the future by the chain latency of that
    // node. Grouping by node rather than per observation is what makes a report a
    // report: a squad radios in what it saw, it does not send one telegram per
    // enemy tank.
    //
    // Reports from a blacked-out node are never created -- that node has no
    // working comms. Its existing tracks freeze, which PhaseTrackUpdate handles.
    const ChainTuning& CT = Settings->Chain;
    const CommsProfile* Comms = Settings->FindCommsProfile(Settings->ActiveCommsProfile);

    for (int32_t P = 0; P < kMaxPlayers; ++P)
    {
        if (Worlds[P] == nullptr || PendingObservations[P].empty())
        {
            continue;
        }

        // Node lookup for this player. Node ids are 1-based and assigned in
        // ascending order by the input builder, so this is a direct index.
        const std::vector<ChainNode>& Nodes = TickInput->ChainNodes[P];

        // Distinct nodes present in this tick's observations, in first-seen order
        // (deterministic: PendingObservations is built in entity-slot order).
        NodeBatchIds.clear();
        for (const Observation& Obs : PendingObservations[P])
        {
            bool bKnown = false;
            for (uint16_t Existing : NodeBatchIds)
            {
                if (Existing == Obs.ObserverNodeId) { bKnown = true; break; }
            }
            if (!bKnown)
            {
                NodeBatchIds.push_back(Obs.ObserverNodeId);
            }
        }

        for (uint16_t NodeId : NodeBatchIds)
        {
            const ChainNode* Node = nullptr;
            if (NodeId != kNoChainNode && size_t(NodeId - 1) < Nodes.size())
            {
                Node = &Nodes[NodeId - 1];
            }

            // A blacked-out node emits nothing at all: no comms, no report.
            if (Node != nullptr && Node->bBlackout)
            {
                continue;
            }

            // Two different reasons an observer can be node-less, with two
            // different meanings (found by the M5 blackout test: the veto above was
            // UNREACHABLE, because attachment already skips blacked-out nodes, so
            // every blackout silently degraded into an orphan report that arrived
            // anyway -- the opposite of §4.4).
            //
            //  * The player has no command structure at all (headless fixtures, and
            //    a player who has lost every command building): reports still walk
            //    in by runner at OrphanDelayTicks. Slow, but a staff of some kind
            //    is assumed to exist.
            //  * The player HAS command buildings but none of them can receive --
            //    every node dark, or no HQ standing: there is nobody to report TO,
            //    so nothing arrives at all. This is the blackout §4.4 describes.
            const bool bHasAnyNode = !Nodes.empty();
            if (bHasAnyNode && !TickInput->HasHqNode[P])
            {
                continue; // network dark: the staff map freezes rather than updates
            }

            // Latency and hop count. An observer attached to the HQ itself is one
            // hop from the staff map; attached to a subordinate node it is
            // HopsFromNodeToHq; with no command structure at all, courier speed.
            int32_t Hops = 0;
            int32_t DelayTicks = 0;
            if (Node == nullptr)
            {
                Hops = 1;
                DelayTicks = CT.OrphanDelayTicks;
            }
            else
            {
                Hops = Node->bIsHq ? 1 : CT.HopsFromNodeToHq;
                int32_t PerHop = 0;
                if (Comms != nullptr && !Comms->HopDelayTicksByLevel.empty())
                {
                    const size_t Level = size_t(CT.CommsLevel) < Comms->HopDelayTicksByLevel.size()
                                             ? size_t(CT.CommsLevel)
                                             : Comms->HopDelayTicksByLevel.size() - 1;
                    PerHop = Comms->HopDelayTicksByLevel[Level];
                }
                DelayTicks = PerHop * Hops;
            }

            // Reliability degrades per hop: each relay summarises, rounds and
            // loses detail. Clamped at zero rather than going negative.
            const Fixed Reliability = ReliabilityAfterHops(CT, Hops);

            ReconReport Report;
            Report.ReportId = NextReportId;
            NextReportId += 1;
            Report.OwnerPlayer = PlayerId(P);
            Report.EmitTick = CurrentTick;
            Report.ArrivalTick = CurrentTick + TickIndex(DelayTicks);
            Report.HopsRemaining = uint8_t(Hops);
            Report.Reliability = Reliability;
            Report.NodeId = NodeId;
            for (const Observation& Obs : PendingObservations[P])
            {
                if (Obs.ObserverNodeId == NodeId)
                {
                    Report.Payload.push_back(Obs);
                }
            }
            if (!Report.Payload.empty())
            {
                InFlightReports.push_back(std::move(Report));
            }
        }
    }
}

void ReconSystem::PhasePropagation(TickIndex)
{
    // Intentionally empty, and kept.
    //
    // M3 put hop counting and latency in PhaseReportEmission, because a report's
    // route is known the moment it is filed: the node that saw the contact decides
    // both the hop count and the arrival tick, so walking the queue every tick to
    // advance it would be bookkeeping with no decision in it.
    //
    // The phase stays in the pipeline as the seat for propagation that genuinely
    // needs per-tick evaluation -- a relay destroyed mid-flight re-routing its
    // queued traffic, or jamming applied to reports already on the wire (finding
    // m3: today a report survives its node's destruction, which is defensible but
    // undocumented). Deleting the phase would mean renumbering the pipeline and
    // its profiling counters to add it back.
}

// --- M3 aggregation helpers ------------------------------------------------------
//
// A staff map does not hold "tank #4713 at (3412, 2988)". It holds "about a
// company of armour around the crossroads". Grouping is therefore not an
// optimisation, it is the unit of belief the player is supposed to reason about
// -- and it is what gives fear something to exaggerate (owner decision Q3: group
// in M3, together with report merging, rather than earlier and twice).
//
// The rule: observations from ONE report that share a category and sit within
// MergeRadiusTiles of an existing group join it; otherwise they open a new one.
// Both steps run in first-seen order, which is entity-slot order, so the outcome
// is deterministic without sorting.

namespace
{

// Squared tile distance between two world positions, in tiles^2, using integer
// math only (Fixed multiply would overflow at map scale for no benefit here).
int64_t TileDistanceSquared(const Vec2& A, const Vec2& B)
{
    const int64_t Ax = A.X.Raw / (kFixedOne * kTileSizeUnits);
    const int64_t Ay = A.Y.Raw / (kFixedOne * kTileSizeUnits);
    const int64_t Bx = B.X.Raw / (kFixedOne * kTileSizeUnits);
    const int64_t By = B.Y.Raw / (kFixedOne * kTileSizeUnits);
    const int64_t Dx = Ax - Bx;
    const int64_t Dy = Ay - By;
    return Dx * Dx + Dy * Dy;
}

// Whether two believed counts disagree enough to call the track contested.
// Proportional, not absolute: 3 vs 4 is a rounding difference, 3 vs 30 is a
// genuine contradiction, and a fixed threshold would confuse the two.
bool CountsMateriallyDiffer(int32_t A, int32_t B, int32_t TolerancePerMille)
{
    // One definition, shared with the tests (see ReconConfig.h).
    return CountsMateriallyDifferForTest(A, B, TolerancePerMille);
}

} // namespace

void ReconSystem::ApplyGroupedReport(PerceivedWorld& World, PlayerId P, const ReconReport& Report,
                                     TickIndex CurrentTick)
{
    const TrackTuning& TT = Settings->Tracks;
    const int64_t MergeRadiusSq = int64_t(TT.MergeRadiusTiles) * int64_t(TT.MergeRadiusTiles);

    // 1. Bucket this report's observations into groups by category + proximity.
    GroupScratch.clear();
    for (const Observation& Obs : Report.Payload)
    {
        int32_t Found = -1;
        for (size_t G = 0; G < GroupScratch.size(); ++G)
        {
            ObservationGroup& Group = GroupScratch[G];
            // Anonymous contacts never merge with identified ones: "something is
            // out there" and "four tanks are out there" are different claims and
            // merging them would invent an identity the player never earned.
            if (Group.Category != Obs.Category || Group.bAnonymous != Obs.bAnonymous ||
                Group.bPhantom != Obs.bPhantom)
            {
                // Phantoms never merge with real contacts inside one report: a
                // genuine blip must not absorb an invented one, because then the
                // phantom would be cleared without anybody having gone to look.
                continue;
            }
            if (TileDistanceSquared(Group.Centre, Obs.ObservedPosition) <= MergeRadiusSq)
            {
                Found = int32_t(G);
                break;
            }
        }
        if (Found < 0)
        {
            ObservationGroup Group;
            Group.Category = Obs.Category;
            Group.bAnonymous = Obs.bAnonymous;
            Group.Centre = Obs.ObservedPosition;
            Group.SumX = Obs.ObservedPosition.X;
            Group.SumY = Obs.ObservedPosition.Y;
            Group.Count = Obs.ObservedCount;
            Group.Members = 1;
            Group.RepresentativeSlot = Obs.Subject.Index;
            Group.RepresentativeGeneration = Obs.Subject.Generation;
            Group.ObservedClass = Obs.ObservedClass;
            Group.bPhantom = Obs.bPhantom;
            GroupScratch.push_back(Group);
        }
        else
        {
            ObservationGroup& Group = GroupScratch[size_t(Found)];
            Group.SumX += Obs.ObservedPosition.X;
            Group.SumY += Obs.ObservedPosition.Y;
            Group.Members += 1;
            Group.Count += Obs.ObservedCount;
            // Centroid, so the marker sits in the middle of the formation rather
            // than on whichever member happened to be first in slot order.
            Group.Centre = Vec2(Group.SumX / int64_t(Group.Members), Group.SumY / int64_t(Group.Members));
            // A mixed group loses its exact class: the staff knows the category,
            // not which specific model each vehicle was.
            if (Group.ObservedClass != Obs.ObservedClass)
            {
                Group.ObservedClass = ContentId{};
            }
            // One real sighting redeems the whole group: if anything genuine is
            // in there, the contact is no longer imaginary.
            Group.bPhantom = Group.bPhantom && Obs.bPhantom;
        }
    }

    // 2. Fold each group into belief, reusing the association of its
    //    representative so a moving formation keeps one track across ticks.
    for (const ObservationGroup& Group : GroupScratch)
    {
        // A phantom has NO ground-truth subject, so it has no association slot to
        // live in -- slot 0 would collide with a real entity. It gets a fresh track
        // every time and is then owned by the refutation rules, which is also why it
        // cannot be "updated" by later reports: an invented contact does not move.
        if (Group.bPhantom)
        {
            const TrackId GhostId = World.AllocateTrack();
            if (!GhostId.IsValid())
            {
                continue; // at the hard cap: dropping an imaginary contact is fine
            }
            PerceivedTrack* Ghost = World.GetTrackMutable(GhostId);
            Ghost->BelievedCategory = Group.Category;
            Ghost->bAnonymous = true;          // fear reports movement, not vehicles
            Ghost->BelievedClass = ContentId{};
            Ghost->BelievedPosition = Group.Centre;
            Ghost->PositionErrorRadius = Fixed::Zero();
            Ghost->BelievedCountMin = Group.Count;
            Ghost->BelievedCountMax = Group.Count;
            Ghost->LastClaimedCount = Group.Count;
            Ghost->LastUpdateTick = CurrentTick;
            Ghost->LastReportNodeId = Report.NodeId;
            Ghost->IndependentSourceCount = 1;
            Ghost->bStale = false;
            Ghost->bContested = false;
            Ghost->Confidence = ConfidenceForSources(TT, 1, false);
            Ghost->PhantomBornTick = CurrentTick;
            Ghost->ProvenanceReportIds[0] = Report.ReportId;
            Ghost->ProvenanceCount = 1;
            World.SetTrackPhantomInternal(GhostId, true);
            continue;
        }

        if (Group.RepresentativeSlot >= EntityCapacity)
        {
            continue;
        }
        TrackId& Assoc = AssociationTrack[P][Group.RepresentativeSlot];
        uint32_t& AssocGen = AssociationGeneration[P][Group.RepresentativeSlot];
        if (Assoc.IsValid() && AssocGen != Group.RepresentativeGeneration)
        {
            Assoc = TrackId{}; // GT slot reused: the HQ cannot know, so a new track
        }

        PerceivedTrack* Track = Assoc.IsValid() ? World.GetTrackMutable(Assoc) : nullptr;
        if (Track == nullptr)
        {
            // Before opening a track, look for an existing one this group plainly
            // refers to: the same area, same category, still fresh. That is what
            // makes two DIFFERENT nodes reporting one formation converge instead
            // of littering the map with duplicates.
            TrackId Existing{};
            for (uint32_t I = 0; I < World.GetTrackCapacity(); ++I)
            {
                const PerceivedTrack& Candidate = World.Tracks[I];
                if (!Candidate.bAlive || Candidate.bAnonymous != Group.bAnonymous ||
                    Candidate.BelievedCategory != Group.Category)
                {
                    continue;
                }
                if (CurrentTick < Candidate.LastUpdateTick ||
                    int32_t(CurrentTick - Candidate.LastUpdateTick) > TT.MergeWindowTicks)
                {
                    continue;
                }
                if (TileDistanceSquared(Candidate.BelievedPosition, Group.Centre) <= MergeRadiusSq)
                {
                    Existing = Candidate.Id;
                    break;
                }
            }
            if (Existing.IsValid())
            {
                Assoc = Existing;
                AssocGen = Group.RepresentativeGeneration;
                Track = World.GetTrackMutable(Existing);
            }
        }

        if (Track == nullptr)
        {
            const TrackId NewId = World.AllocateTrack();
            if (!NewId.IsValid())
            {
                continue; // at the hard cap; losing the report is the honest outcome
            }
            Assoc = NewId;
            AssocGen = Group.RepresentativeGeneration;
            Track = World.GetTrackMutable(NewId);
            Track->IndependentSourceCount = 0;
        }

        // Corroboration: a report from a node we have not heard from on this track
        // is an independent source. Same node again is the same source saying the
        // same thing, which is not evidence.
        const bool bNewSource = Track->LastReportNodeId != Report.NodeId;
        // "Has anyone reported this before?" A track written on tick 0 would read as
        // never-observed, which is unreachable today because aggregation runs after
        // emission so the earliest arrival is tick 1 -- but the coupling to phase
        // order was implicit, so IndependentSourceCount carries the real answer and
        // the tick test is only a fallback (finding m6).
        const bool bWasObserved = Track->IndependentSourceCount > 0 || Track->LastUpdateTick != 0;
        // Compare against the PREVIOUS CLAIM, not the accumulated interval maximum.
        // Comparing to the max let one early over-count poison the tolerance test
        // forever: a track that once read [1,30] would call every later 20-ish
        // report "agreeing" and every 5-ish report "contested" (review M3).
        const bool bContestedNow =
            bWasObserved && bNewSource &&
            CountsMateriallyDiffer(Track->LastClaimedCount, Group.Count, TT.ContestedCountTolerancePerMille);

        Track->BelievedCategory = Group.Category;
        Track->bAnonymous = Group.bAnonymous;
        Track->BelievedClass = Group.bAnonymous ? ContentId{} : Group.ObservedClass;
        Track->BelievedPosition = Group.Centre;
        Track->PositionErrorRadius = Fixed::Zero();
        Track->LastUpdateTick = CurrentTick;
        Track->LastReportNodeId = Report.NodeId;
        Track->bStale = false;

        if (bContestedNow)
        {
            // Sources disagree: keep BOTH claims by widening the interval, and say
            // so. Picking a winner here would hide exactly the uncertainty the
            // player needs in order to send someone to look again.
            //
            // The interval spans the two CONFLICTING CLAIMS only -- the previous
            // claim and this one -- rather than ratcheting outward across the whole
            // history of the track. An interval that only ever grows would drift to
            // the historical extreme and stop describing the present (review M3).
            Track->bContested = true;
            const int32_t Low = Track->LastClaimedCount < Group.Count ? Track->LastClaimedCount : Group.Count;
            const int32_t High = Track->LastClaimedCount > Group.Count ? Track->LastClaimedCount : Group.Count;
            Track->BelievedCountMin = Low;
            Track->BelievedCountMax = High;
        }
        else
        {
            Track->bContested = false;
            Track->BelievedCountMin = Group.Count;
            Track->BelievedCountMax = Group.Count;
        }

        if (bNewSource && Track->IndependentSourceCount < 255)
        {
            Track->IndependentSourceCount = uint8_t(Track->IndependentSourceCount + 1);
        }

        // Confidence. Agreement between independent sources is worth MORE than the
        // sum of its parts (§4.4): two eyes on the same formation is qualitatively
        // better evidence than one looking twice. Contested data does not earn the
        // bonus -- the sources cancel rather than reinforce.
        Track->Confidence =
            ConfidenceForSources(TT, Track->IndependentSourceCount, Track->bContested);

        // The claim this report actually made, kept apart from the interval shown
        // to the player: the next contest test compares against this.
        Track->LastClaimedCount = Group.Count;

        // Phantom truth rides in the private side table, never on the track (that
        // is INVARIANT 10). It is what makes refutation possible: only the core
        // knows this contact was invented, and only the core may clear it.
        if (Group.bPhantom)
        {
            World.SetTrackPhantomInternal(Track->Id, true);
            if (Track->PhantomBornTick == 0)
            {
                Track->PhantomBornTick = CurrentTick;
            }
        }

        Track->ProvenanceReportIds[Track->ProvenanceCount % kTrackProvenanceSize] = Report.ReportId;
        Track->ProvenanceCount = uint8_t((Track->ProvenanceCount + 1) % (kTrackProvenanceSize * 2));
    }
}

void ReconSystem::PhaseAggregation(TickIndex CurrentTick)
{
    // Applies every report whose ArrivalTick has come.
    //
    // M3 adds two things on top of M1's per-entity association:
    //  * grouping -- nearby same-category contacts in one report become ONE track
    //    carrying a count interval, which is how a staff map actually reads;
    //  * agreement/contest -- a second independent node reporting the same area
    //    raises confidence superlinearly, while a materially different count sets
    //    bContested and widens the interval instead of silently picking a winner.
    //
    // Contested rather than competing hypotheses: chosen in the M2 design review
    // and recorded in ADR-0026. Splitting a track in two doubles the state, the
    // decay bookkeeping and the phantom-refutation surface for the same gameplay
    // signal -- "your sources disagree, go look again".
    size_t WriteBack = 0;
    for (size_t I = 0; I < InFlightReports.size(); ++I)
    {
        ReconReport& Report = InFlightReports[I];
        if (Report.ArrivalTick > CurrentTick)
        {
            // Not yet arrived: keep in flight (stable order preserved).
            if (WriteBack != I)
            {
                InFlightReports[WriteBack] = std::move(Report);
            }
            WriteBack += 1;
            continue;
        }

        const PlayerId P = Report.OwnerPlayer;
        PerceivedWorld* World = (P < kMaxPlayers) ? Worlds[P].get() : nullptr;
        if (World == nullptr)
        {
            continue; // owner slot inactive; drop the report
        }

        // --- Group pass -------------------------------------------------------
        // Collapse this report's observations into groups before touching belief,
        // so a single track update carries the whole crowd rather than the last
        // tank in slot order.
        const TrackTuning& TT = Settings->Tracks;
        if (TT.bGroupTracksEnabled)
        {
            ApplyGroupedReport(*World, P, Report, CurrentTick);
            continue;
        }

        for (const Observation& Obs : Report.Payload)
        {
            const uint32_t Slot = Obs.Subject.Index;
            if (Slot >= EntityCapacity)
            {
                continue;
            }

            // GT slot reuse severs the association: the HQ cannot know that the
            // unit it was tracking died and its slot now holds a stranger. The
            // old track stays behind as last-known-position (frozen; decay and
            // GC are TrackUpdate's job), and the stranger gets a fresh track.
            TrackId& Assoc = AssociationTrack[P][Slot];
            uint32_t& AssocGen = AssociationGeneration[P][Slot];
            if (Assoc.IsValid() && AssocGen != Obs.Subject.Generation)
            {
                Assoc = TrackId{};
            }

            PerceivedTrack* Track = Assoc.IsValid() ? World->GetTrackMutable(Assoc) : nullptr;
            if (Track == nullptr)
            {
                const TrackId NewId = World->AllocateTrack();
                if (!NewId.IsValid())
                {
                    continue; // at hard cap; report is lost, which is honest
                }
                Assoc = NewId;
                AssocGen = Obs.Subject.Generation;
                Track = World->GetTrackMutable(NewId);
            }

            // Truthful update: direct observation overwrites, never averages
            // (§4.5 -- and with zero distortion there is nothing to average).
            Track->BelievedClass = Obs.ObservedClass;
            Track->BelievedCategory = Obs.Category;
            Track->bAnonymous = Obs.bAnonymous;
            Track->BelievedPosition = Obs.ObservedPosition;
            Track->PositionErrorRadius = Fixed::Zero();
            Track->BelievedCountMin = Obs.ObservedCount;
            Track->BelievedCountMax = Obs.ObservedCount;
            Track->LastUpdateTick = CurrentTick;
            Track->Confidence = Fixed::FromInt(1);
            Track->IndependentSourceCount = 1;
            Track->bStale = false;
            Track->bContested = false;
            Track->ProvenanceReportIds[Track->ProvenanceCount % kTrackProvenanceSize] = Report.ReportId;
            Track->ProvenanceCount = uint8_t((Track->ProvenanceCount + 1) % (kTrackProvenanceSize * 2));
        }
    }
    InFlightReports.resize(WriteBack);
}

void ReconSystem::ReleaseTrackAndAssociations(PlayerId P, PerceivedWorld& World, TrackId Id)
{
    // Sever associations BEFORE releasing, or the next report about that ground-truth
    // entity would write into a freed slot. Both halves of the pair are cleared: a
    // stale generation beside an invalid handle is harmless only until someone
    // reorders the staleness test (M3 review M1).
    for (uint32_t Slot = 0; Slot < EntityCapacity; ++Slot)
    {
        if (AssociationTrack[P][Slot] == Id)
        {
            AssociationTrack[P][Slot] = TrackId{};
            AssociationGeneration[P][Slot] = 0;
        }
    }
    World.ReleaseTrack(Id);
}

bool ReconSystem::IsTileObservedAndEmpty(PlayerId P, const Vec2& Position, TickIndex CurrentTick) const
{
    // "Someone went and looked, and there was nothing there."
    //
    // Observed: the player's fog shows the tile this tick -- taken from the
    // observation input, which is the same fog the rest of the layer reads, so a
    // phantom cannot be cleared by knowledge the player does not have.
    // Empty: no real observation this tick sits within the refutation radius.
    if (TickInput == nullptr)
    {
        return false;
    }
    const int32_t TileX = int32_t(Position.X.ToIntFloor() / kTileSizeUnits);
    const int32_t TileY = int32_t(Position.Y.ToIntFloor() / kTileSizeUnits);

    // "Is anyone looking there?" must come from the FOG, not from the observation
    // log. LastObservedTick only records tiles where something WAS seen, so asking
    // it here made this path unreachable: a scout standing in an empty clearing
    // could never disprove a phantom, which is exactly the promise of §4.5.
    const uint32_t Packed = (uint32_t(TileX) << 16) | (uint32_t(TileY) & 0xFFFFu);
    bool bWatched = false;
    for (uint32_t Tile : TickInput->VisibleTilesPacked[P])
    {
        if (Tile == Packed)
        {
            bWatched = true;
            break;
        }
    }
    if (!bWatched)
    {
        return false; // nobody is looking there right now
    }
    (void)CurrentTick;

    // "Empty" means nothing real is standing ON the reported spot -- not "nothing
    // real is anywhere nearby". A phantom plotted next to a genuine contact is still
    // a phantom: it claims a SEPARATE force, and the observer looking at that ground
    // can see there is no second force there. Using the merge radius here made every
    // phantom born during a firefight unrefutable, because the enemy being fought
    // was always within it.
    //
    // One tile is the resolution the staff map works at, so it is the honest test.
    for (const ObservedEntity& Seen : TickInput->VisibleToPlayer[P])
    {
        if (Seen.TileX == TileX && Seen.TileY == TileY)
        {
            return false; // something real IS there; the contact was not imaginary
        }
    }
    return true;
}

void ReconSystem::PhaseTrackUpdate(TickIndex CurrentTick)
{
    // Ageing of belief. A track nobody has refreshed does not vanish and does not
    // stay crisp: it BLURS. The staff map keeps the last known position, the error
    // radius grows, confidence falls, and eventually the entry is dropped.
    //
    // That is the mechanic §4.4 calls the main source of tragic mistakes: an
    // attack ordered against a frozen contact that moved twenty seconds ago. It
    // is also why blackout does not erase anything -- erasing would be merciful
    // and wrong.
    //
    // The sweep is amortized round-robin (ADR-0021, I-B4): at most
    // TracksPerTickBudget slots per tick, resuming from a persistent cursor, so a
    // load spike cannot turn decay into a frame-time cliff. The cursor is
    // serialized and hashed because it decides WHICH tick a given track's
    // confidence drops on.
    const TrackTuning& TT = Settings->Tracks;
    const ChainTuning& CT = Settings->Chain;
    const DistortionProfile* Profile = Settings->FindDistortionProfile(Settings->ActiveDistortionProfile);
    const int32_t PhantomLifetime = Profile != nullptr ? Profile->MaxPhantomLifetimeTicks : 0;

    for (int32_t P = 0; P < kMaxPlayers; ++P)
    {
        PerceivedWorld* World = Worlds[P].get();
        if (World == nullptr)
        {
            continue;
        }

        // Whether this player's command network is silent right now. A player with
        // no working HQ has no staff receiving anything, so every one of their
        // tracks ages at the blackout rate.
        const bool bNetworkDown = TickInput != nullptr && !TickInput->HasHqNode[P];

        const uint32_t Capacity = World->GetTrackCapacity();
        if (Capacity == 0)
        {
            continue;
        }
        const uint32_t Budget = TT.TracksPerTickBudget > 0
                                    ? uint32_t(TT.TracksPerTickBudget)
                                    : Capacity;
        const uint32_t Visits = Budget < Capacity ? Budget : Capacity;

        for (uint32_t Step = 0; Step < Visits; ++Step)
        {
            const uint32_t I = (World->DecayCursor + Step) % Capacity;
            PerceivedTrack& T = World->Tracks[I]; // friend access: phases are the writer
            if (!T.bAlive)
            {
                continue;
            }
            if (CurrentTick < T.LastUpdateTick)
            {
                continue; // freshly written this tick; nothing to age
            }
            const int32_t Age = int32_t(CurrentTick - T.LastUpdateTick);
            if (Age <= 0)
            {
                continue;
            }

            if (Age >= TT.StaleAfterTicks)
            {
                T.bStale = true;
            }

            // --- Phantom refutation (M4, §4.5) -------------------------------------
            // A phantom MUST have a guaranteed path to being disproved. Without one,
            // a player who investigates and finds nothing learns that investigating
            // does not work, and from then on treats the whole staff map as noise --
            // which costs more than the phantom ever gained.
            //
            // Two paths, and the second one cannot be avoided:
            //  1. Someone looks. A friendly observer standing where the phantom is
            //     reported, seeing nothing, clears it -- this is the path the player
            //     controls and the one scouting is supposed to reward.
            //  2. The deadline. Every phantom dies at MaxPhantomLifetimeTicks no
            //     matter what, so an unreachable corner of the map cannot host an
            //     immortal ghost.
            if (World->IsTrackPhantomInternal(T.Id))
            {
                const int32_t PhantomAge =
                    T.PhantomBornTick == 0 ? 0 : int32_t(CurrentTick - T.PhantomBornTick);
                const bool bDeadlinePassed = PhantomAge >= PhantomLifetime;
                const bool bLookedAtAndEmpty =
                    TickInput != nullptr &&
                    IsTileObservedAndEmpty(PlayerId(P), T.BelievedPosition, CurrentTick);
                if (bDeadlinePassed || bLookedAtAndEmpty)
                {
                    ReleaseTrackAndAssociations(PlayerId(P), *World, T.Id);
                    continue;
                }
            }

            // How much wall-clock this slot is accountable for. Measured, not
            // estimated: charging a ceil-rounded (Capacity / Visits) made the
            // effective decay rate a function of how many tracks happened to exist,
            // so "per second" meant different things at different moments and any
            // tuning done at one track count was wrong at another (review M4).
            // LastDecayTick records the tick this slot was last charged, so the
            // interval is exact regardless of cap, budget or sweep phase.
            const int32_t TicksPerVisit =
                T.LastDecayTick == 0 ? 1 : int32_t(CurrentTick - T.LastDecayTick);
            T.LastDecayTick = CurrentTick;
            if (TicksPerVisit <= 0)
            {
                continue; // already charged this tick
            }

            // Confidence decay. A blacked-out network loses faith faster: the
            // staff knows it is working from data nobody can confirm.
            const int32_t DecayPerSecond = TT.ConfidenceDecayPerSecondPerMille +
                                           (bNetworkDown ? CT.BlackoutConfidenceDecayPerSecondPerMille : 0);
            const Fixed DecayThisVisit =
                PerMilleToFixed(DecayPerSecond) * int64_t(TicksPerVisit) / int64_t(kTicksPerSecond);
            T.Confidence = FxClamp(T.Confidence - DecayThisVisit, Fixed::Zero(), Fixed::FromInt(1));

            // Error radius growth: the longer since anyone looked, the larger the
            // area the contact could be in. Monotonic by construction -- only a
            // fresh observation resets it, in aggregation.
            const int64_t GrowthUnitsPerMinute =
                int64_t(TT.ErrorRadiusGrowthTilesPerMinute) * kTileSizeUnits;
            const Fixed GrowthThisVisit = Fixed::FromInt(GrowthUnitsPerMinute) *
                                          int64_t(TicksPerVisit) /
                                          int64_t(kTicksPerSecond * 60);
            T.PositionErrorRadius = T.PositionErrorRadius + GrowthThisVisit;

            // Garbage collection: belief nobody has any confidence in is noise on
            // the map. Dropping it deterministically (same tick on every peer) is
            // why the cursor is part of the checksum.
            if (T.Confidence <= PerMilleToFixed(TT.DropBelowConfidencePerMille))
            {
                ReleaseTrackAndAssociations(PlayerId(P), *World, T.Id);
            }
        }
        World->DecayCursor = (World->DecayCursor + Visits) % Capacity;
    }
}

// --- Determinism plumbing ------------------------------------------------------

void ReconSystem::Serialize(ByteWriter& W) const
{
    W.WriteUInt32(kReconSystemVersion);
    W.WriteBool(IsEnabled());
    if (!IsEnabled())
    {
        return;
    }

    W.WriteUInt32(NextReportId);
    W.WriteUInt32(uint32_t(InFlightReports.size()));
    for (const ReconReport& Report : InFlightReports)
    {
        W.WriteUInt32(Report.ReportId);
        // OwnerPlayer decides WHOSE staff map the report feeds. Omitting it made
        // every in-flight report load as kInvalidPlayer and get dropped in
        // aggregation -- invisible in M1 (the queue was empty at end of tick) and a
        // guaranteed post-load desync in M3, where the queue lives for seconds.
        W.WriteUInt8(Report.OwnerPlayer);
        W.WriteUInt32(Report.Author.Index);
        W.WriteUInt32(Report.Author.Generation);
        W.WriteUInt32(Report.EmitTick);
        W.WriteUInt32(Report.ArrivalTick);
        W.WriteUInt8(Report.HopsRemaining);
        W.WriteUInt32(Report.NodeId);
        W.WriteInt64(Report.Reliability.Raw);
        W.WriteUInt32(uint32_t(Report.Payload.size()));
        for (const Observation& Obs : Report.Payload)
        {
            // ObserverNodeId drives node batching in emission and is the report's
            // own routing record; a restored report without it is internally
            // inconsistent with its serialized Report.NodeId.
            W.WriteUInt32(Obs.ObserverNodeId);
            W.WriteUInt32(Obs.Subject.Index);
            W.WriteUInt32(Obs.Subject.Generation);
            W.WriteUInt32(Obs.ObservedClass.Value);
            W.WriteInt64(Obs.ObservedPosition.X.Raw);
            W.WriteInt64(Obs.ObservedPosition.Y.Raw);
            W.WriteInt32(Obs.ObservedCount);
            W.WriteUInt32(Obs.Tick);
            W.WriteInt64(Obs.Clarity.Raw);
            W.WriteBool(Obs.bPhantom);
            W.WriteUInt8(uint8_t(Obs.Category));
            W.WriteBool(Obs.bAnonymous);
        }
    }

    for (int32_t P = 0; P < kMaxPlayers; ++P)
    {
        const bool bHasWorld = Worlds[P] != nullptr;
        W.WriteBool(bHasWorld);
        if (bHasWorld)
        {
            Worlds[P]->Serialize(W);
        }
    }

    // The track<->entity association decides whether the next report UPDATES a
    // track or ALLOCATES a duplicate -- that is future-state-shaping, so it must
    // survive a save exactly (a post-load duplicate contact is a desync AND a
    // visible bug: two blips for one tank).
    W.WriteUInt32(EntityCapacity);
    for (int32_t P = 0; P < kMaxPlayers; ++P)
    {
        if (Worlds[P] == nullptr)
        {
            continue;
        }
        for (uint32_t I = 0; I < EntityCapacity; ++I)
        {
            W.WriteUInt32(AssociationTrack[P][I].Index);
            W.WriteUInt32(AssociationTrack[P][I].Generation);
            W.WriteUInt32(AssociationGeneration[P][I]);
        }
    }
}

bool ReconSystem::Deserialize(ByteReader& R)
{
    if (R.ReadUInt32() != kReconSystemVersion)
    {
        return false;
    }
    const bool bWasEnabled = R.ReadBool();
    // Enabled-ness comes from content settings at Initialize time; a save made
    // with the feature on cannot load into a session with it off, because the
    // belief state it contains would be unreachable and the checksum would lie.
    if (bWasEnabled != IsEnabled())
    {
        return false;
    }
    if (!bWasEnabled)
    {
        return true;
    }

    NextReportId = R.ReadUInt32();
    const uint32_t ReportCount = R.ReadUInt32();
    InFlightReports.clear();
    InFlightReports.reserve(ReportCount);
    for (uint32_t I = 0; I < ReportCount; ++I)
    {
        ReconReport Report;
        Report.ReportId = R.ReadUInt32();
        Report.OwnerPlayer = R.ReadUInt8();
        Report.Author.Index = R.ReadUInt32();
        Report.Author.Generation = R.ReadUInt32();
        Report.EmitTick = R.ReadUInt32();
        Report.ArrivalTick = R.ReadUInt32();
        Report.HopsRemaining = R.ReadUInt8();
        Report.NodeId = uint16_t(R.ReadUInt32());
        Report.Reliability = Fixed::FromRaw(R.ReadInt64());
        const uint32_t PayloadCount = R.ReadUInt32();
        Report.Payload.reserve(PayloadCount);
        for (uint32_t J = 0; J < PayloadCount; ++J)
        {
            Observation Obs;
            Obs.ObserverNodeId = uint16_t(R.ReadUInt32());
            Obs.Subject.Index = R.ReadUInt32();
            Obs.Subject.Generation = R.ReadUInt32();
            Obs.ObservedClass = ContentId(R.ReadUInt32());
            Obs.ObservedPosition.X = Fixed::FromRaw(R.ReadInt64());
            Obs.ObservedPosition.Y = Fixed::FromRaw(R.ReadInt64());
            Obs.ObservedCount = R.ReadInt32();
            Obs.Tick = R.ReadUInt32();
            Obs.Clarity = Fixed::FromRaw(R.ReadInt64());
            Obs.bPhantom = R.ReadBool();
            Obs.Category = ObservedCategory(R.ReadUInt8());
            Obs.bAnonymous = R.ReadBool();
            Report.Payload.push_back(Obs);
        }
        InFlightReports.push_back(std::move(Report));
    }

    for (int32_t P = 0; P < kMaxPlayers; ++P)
    {
        const bool bHasWorld = R.ReadBool();
        if (bHasWorld != (Worlds[P] != nullptr))
        {
            return false;
        }
        if (bHasWorld && !Worlds[P]->Deserialize(R))
        {
            return false;
        }
    }

    EntityCapacity = R.ReadUInt32();
    for (int32_t P = 0; P < kMaxPlayers; ++P)
    {
        if (Worlds[P] == nullptr)
        {
            continue;
        }
        AssociationTrack[P].assign(EntityCapacity, TrackId{});
        AssociationGeneration[P].assign(EntityCapacity, 0);
        for (uint32_t I = 0; I < EntityCapacity; ++I)
        {
            AssociationTrack[P][I].Index = R.ReadUInt32();
            AssociationTrack[P][I].Generation = R.ReadUInt32();
            AssociationGeneration[P][I] = R.ReadUInt32();
        }
    }
    return true;
}

void ReconSystem::FeedChecksum(Hash64& H) const
{
    H.FeedBool(IsEnabled());
    if (!IsEnabled())
    {
        return;
    }
    H.FeedUInt32(NextReportId);
    H.FeedUInt32(uint32_t(InFlightReports.size()));
    for (const ReconReport& Report : InFlightReports)
    {
        H.FeedUInt32(Report.ReportId);
        H.FeedUInt32(Report.ArrivalTick);
        // M3: the queue now holds reports for many ticks, so everything that
        // decides what they will DO on arrival is future-influencing state and
        // must be hashed -- otherwise two peers with differently-routed reports
        // agree today and diverge when the reports land.
        H.FeedUInt32(Report.EmitTick);
        H.FeedUInt8(Report.HopsRemaining);
        H.FeedUInt32(Report.NodeId);
        H.FeedInt64(Report.Reliability.Raw);
        H.FeedUInt8(Report.OwnerPlayer);
        H.FeedUInt32(uint32_t(Report.Payload.size()));
    }
    for (int32_t P = 0; P < kMaxPlayers; ++P)
    {
        if (Worlds[P] != nullptr)
        {
            Worlds[P]->FeedChecksum(H);
        }
    }
    H.FeedUInt32(EntityCapacity);
    for (int32_t P = 0; P < kMaxPlayers; ++P)
    {
        if (Worlds[P] == nullptr)
        {
            continue;
        }
        for (uint32_t I = 0; I < EntityCapacity; ++I)
        {
            if (AssociationTrack[P][I].IsValid())
            {
                H.FeedUInt32(I);
                H.FeedUInt32(AssociationTrack[P][I].Index);
                H.FeedUInt32(AssociationTrack[P][I].Generation);
                H.FeedUInt32(AssociationGeneration[P][I]);
            }
        }
    }
}

} // namespace Recon
} // namespace RA4
