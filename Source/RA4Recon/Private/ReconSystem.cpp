// Copyright (c) Red Alert 4 project.
#include "RA4Recon/ReconSystem.h"

#include "RA4Core/ByteStream.h"

#include <chrono>
#include "RA4Core/Checksum.h"
#include "RA4Recon/DistortionPipeline.h"

namespace RA4
{
namespace Recon
{

namespace
{
constexpr uint32_t kReconSystemVersion = 4; // v4: report NodeId, chain latency (M3); v3: category/anonymous (M2); v2: association tables (M1)
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

void ReconSystem::PhaseDistortion(TickIndex)
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
        if (Worlds[P] == nullptr || PendingObservations[P].empty())
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
        NodeBatchStart.clear();
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

            // Latency and hop count. An observer attached to the HQ itself is one
            // hop from the staff map; attached to a subordinate node it is
            // HopsFromNodeToHq; attached to nothing, or reporting to a player with
            // no HQ standing, it takes the orphan delay.
            int32_t Hops = 0;
            int32_t DelayTicks = 0;
            if (Node == nullptr || !TickInput->HasHqNode[P])
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
            Fixed Reliability = Fixed::FromInt(1) -
                                PerMilleToFixed(CT.ReliabilityLossPerHopPerMille * Hops);
            Reliability = FxClamp(Reliability, Fixed::Zero(), Fixed::FromInt(1));

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
    // M3 territory (hops, delays, blackout). With zero-delay reports there is
    // nothing to advance.
}

void ReconSystem::PhaseAggregation(TickIndex CurrentTick)
{
    // Applies every report whose ArrivalTick has come. M1 keeps the merge rule
    // trivial -- one GT entity maps to one track via the association table; the
    // spatial merge of anonymous reports is M3's problem.
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

void ReconSystem::PhaseTrackUpdate(TickIndex CurrentTick)
{
    // M1 scope: stale marking only, so a track whose subject left our vision is
    // visibly old data rather than a live contact. Confidence decay curves,
    // error-radius growth and phantom refutation land with M2/M4 (I-B3/I-B4
    // decide the decay model first).
    for (int32_t P = 0; P < kMaxPlayers; ++P)
    {
        PerceivedWorld* World = Worlds[P].get();
        if (World == nullptr)
        {
            continue;
        }
        const int32_t StaleAfter = Settings->Tracks.StaleAfterTicks;
        for (uint32_t I = 0; I < World->GetTrackCapacity(); ++I)
        {
            PerceivedTrack& T = World->Tracks[I]; // friend access: phases are the writer
            if (!T.bAlive || T.bStale)
            {
                continue;
            }
            if (CurrentTick >= T.LastUpdateTick && int32_t(CurrentTick - T.LastUpdateTick) >= StaleAfter)
            {
                T.bStale = true;
            }
        }
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
