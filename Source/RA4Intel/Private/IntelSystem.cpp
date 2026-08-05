// Copyright (c) Red Alert 4 project.
#include "RA4Intel/IntelSystem.h"

#include "RA4Core/ByteStream.h"
#include "RA4Core/Checksum.h"

namespace RA4
{
namespace Intel
{

namespace
{
constexpr uint32_t kIntelSystemVersion = 1;
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

void IntelSystem::Initialize(const IntelSettings* InSettings, int32_t MapWidthTiles, int32_t MapHeightTiles)
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

void IntelSystem::Reset()
{
    Settings = nullptr;
    for (int32_t P = 0; P < kMaxPlayers; ++P)
    {
        Worlds[P].reset();
    }
    InFlightReports.clear();
    NextReportId = 1;
    Stats = PhaseStats{};
}

void IntelSystem::Tick(TickIndex CurrentTick)
{
    if (!IsEnabled())
    {
        return;
    }

    // Fixed phase order -- part of the replay compatibility contract, do not
    // reorder without a format version bump (same rule as SimWorld::Tick).
    PhaseMoraleUpdate(CurrentTick);
    PhaseObservation(CurrentTick);
    PhaseDistortion(CurrentTick);
    PhaseReportEmission(CurrentTick);
    PhasePropagation(CurrentTick);
    PhaseAggregation(CurrentTick);
    PhaseTrackUpdate(CurrentTick);

    Stats.TicksMeasured += 1;
}

const PerceivedWorld& IntelSystem::GetPerceivedWorld(PlayerId PlayerIdx) const
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

PerceivedWorld& IntelSystem::GetPerceivedWorldMutable(PlayerId PlayerIdx)
{
    // Mutable access is core-internal (phases and tests). Unlike the const
    // accessor there is no safe fallback, so this asserts by contract: callers
    // must check IsEnabled() first.
    return *Worlds[PlayerIdx];
}

// --- Phases (M0: deliberately empty, see header) -----------------------------

void IntelSystem::PhaseMoraleUpdate(TickIndex) {}
void IntelSystem::PhaseObservation(TickIndex) {}
void IntelSystem::PhaseDistortion(TickIndex) {}
void IntelSystem::PhaseReportEmission(TickIndex) {}
void IntelSystem::PhasePropagation(TickIndex) {}
void IntelSystem::PhaseAggregation(TickIndex) {}
void IntelSystem::PhaseTrackUpdate(TickIndex) {}

// --- Determinism plumbing ------------------------------------------------------

void IntelSystem::Serialize(ByteWriter& W) const
{
    W.WriteUInt32(kIntelSystemVersion);
    W.WriteBool(IsEnabled());
    if (!IsEnabled())
    {
        return;
    }

    W.WriteUInt32(NextReportId);
    W.WriteUInt32(uint32_t(InFlightReports.size()));
    for (const IntelReport& Report : InFlightReports)
    {
        W.WriteUInt32(Report.ReportId);
        W.WriteUInt32(Report.Author.Index);
        W.WriteUInt32(Report.Author.Generation);
        W.WriteUInt32(Report.EmitTick);
        W.WriteUInt32(Report.ArrivalTick);
        W.WriteUInt8(Report.HopsRemaining);
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
}

bool IntelSystem::Deserialize(ByteReader& R)
{
    if (R.ReadUInt32() != kIntelSystemVersion)
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
        IntelReport Report;
        Report.ReportId = R.ReadUInt32();
        Report.Author.Index = R.ReadUInt32();
        Report.Author.Generation = R.ReadUInt32();
        Report.EmitTick = R.ReadUInt32();
        Report.ArrivalTick = R.ReadUInt32();
        Report.HopsRemaining = R.ReadUInt8();
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
    return true;
}

void IntelSystem::FeedChecksum(Hash64& H) const
{
    H.FeedBool(IsEnabled());
    if (!IsEnabled())
    {
        return;
    }
    H.FeedUInt32(NextReportId);
    H.FeedUInt32(uint32_t(InFlightReports.size()));
    for (const IntelReport& Report : InFlightReports)
    {
        H.FeedUInt32(Report.ReportId);
        H.FeedUInt32(Report.ArrivalTick);
    }
    for (int32_t P = 0; P < kMaxPlayers; ++P)
    {
        if (Worlds[P] != nullptr)
        {
            Worlds[P]->FeedChecksum(H);
        }
    }
}

} // namespace Intel
} // namespace RA4
