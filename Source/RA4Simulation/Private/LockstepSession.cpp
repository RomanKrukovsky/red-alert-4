// Copyright (c) Red Alert 4 project. Deterministic lockstep session.
#include "RA4Simulation/LockstepSession.h"

namespace RA4
{
namespace Net
{

void LockstepSession::Initialize(PlayerId InLocalPlayer, uint8_t InNumPlayers, bool bIsAuthority,
                                 TickIndex InInputDelay)
{
    Reset();

    bInitialized = true;
    bAuthority = bIsAuthority;
    LocalPlayer = InLocalPlayer;
    NumPlayers = InNumPlayers;

    // A zero input delay means a command must execute on the tick it was issued,
    // which cannot survive any latency at all: the packet would have to arrive
    // before it was sent. Clamp rather than reject, so a bad config degrades into a
    // playable-but-laggy match instead of a match that silently drops orders.
    InputDelay = (InInputDelay == 0) ? TickIndex(1) : InInputDelay;
}

void LockstepSession::Reset()
{
    bInitialized = false;
    bAuthority = false;
    LocalPlayer = kInvalidPlayer;
    NumPlayers = 0;
    InputDelay = kDefaultInputDelay;

    OutgoingCommands.clear();
    IncomingFrames.clear();
    AuthoritativeFrames.clear();
    ReferenceChecksums.clear();
    Desync = DesyncReport();
}

// --- Local input -----------------------------------------------------------

TickIndex LockstepSession::SubmitLocalCommand(TickIndex CurrentTick, const Command& Cmd)
{
    if (!bInitialized)
    {
        return CurrentTick;
    }

    const TickIndex TargetTick = CurrentTick + InputDelay;
    OutgoingCommands[TargetTick].push_back(Cmd);
    return TargetTick;
}

CommandFrame LockstepSession::TakeOutgoingFrame(TickIndex TargetTick)
{
    CommandFrame Frame;
    Frame.Tick = TargetTick;

    auto It = OutgoingCommands.find(TargetTick);
    if (It != OutgoingCommands.end())
    {
        Frame.Commands = std::move(It->second);
        OutgoingCommands.erase(It);
    }

    // An empty frame is still returned and still has to be sent. See the header:
    // silence is indistinguishable from packet loss, and the server would stall the
    // whole match waiting for a player who simply had nothing to say.
    return Frame;
}

// --- Server side -----------------------------------------------------------

LockstepSession::PendingTick& LockstepSession::TickSlot(TickIndex Tick)
{
    auto It = IncomingFrames.find(Tick);
    if (It == IncomingFrames.end())
    {
        PendingTick Slot;
        Slot.PerPlayer.resize(NumPlayers);
        Slot.bReported.assign(NumPlayers, false);
        It = IncomingFrames.emplace(Tick, std::move(Slot)).first;
    }
    return It->second;
}

void LockstepSession::ReceivePlayerFrame(PlayerId From, const CommandFrame& Frame)
{
    if (!bInitialized || !bAuthority || From >= NumPlayers)
    {
        return;
    }

    PendingTick& Slot = TickSlot(Frame.Tick);
    if (Slot.bReported[From])
    {
        // A retransmitted or duplicated packet. Applying it again would execute the
        // same production orders and credit spends twice on the server's frame while
        // clients saw them once, which is a desync manufactured by the network layer.
        return;
    }

    Slot.PerPlayer[From] = Frame;
    Slot.bReported[From] = true;
    Slot.ReportedCount = uint8_t(Slot.ReportedCount + 1);
}

bool LockstepSession::IsFrameComplete(TickIndex Tick) const
{
    if (!bInitialized || !bAuthority)
    {
        return false;
    }

    auto It = IncomingFrames.find(Tick);
    return It != IncomingFrames.end() && It->second.ReportedCount == NumPlayers;
}

bool LockstepSession::AssembleAuthoritativeFrame(TickIndex Tick, CommandFrame& Out)
{
    if (!IsFrameComplete(Tick))
    {
        return false;
    }

    const PendingTick& Slot = IncomingFrames.find(Tick)->second;

    Out.Tick = Tick;
    Out.Commands.clear();

    // Slot order, never arrival order. This loop is the reason two servers handed the
    // same packets in different sequences still produce byte-identical frames.
    for (size_t P = 0; P < Slot.PerPlayer.size(); ++P)
    {
        const std::vector<Command>& Src = Slot.PerPlayer[P].Commands;
        Out.Commands.insert(Out.Commands.end(), Src.begin(), Src.end());
    }

    // The partial frame is deliberately left in place. Assembly is a pure read, so a
    // dropped broadcast can simply be rebuilt and resent; PruneUpToTick reclaims it
    // once the tick is retired everywhere.
    return true;
}

// --- Client side -----------------------------------------------------------

void LockstepSession::ReceiveAuthoritativeFrame(const CommandFrame& Frame)
{
    if (!bInitialized)
    {
        return;
    }
    AuthoritativeFrames[Frame.Tick] = Frame;
}

StallReason LockstepSession::GetStallReason(TickIndex Tick) const
{
    if (!bInitialized)
    {
        return StallReason::NotStarted;
    }
    if (AuthoritativeFrames.find(Tick) == AuthoritativeFrames.end())
    {
        return StallReason::AwaitingFrame;
    }
    return StallReason::None;
}

bool LockstepSession::TakeAuthoritativeFrame(TickIndex Tick, CommandFrame& Out)
{
    auto It = AuthoritativeFrames.find(Tick);
    if (It == AuthoritativeFrames.end())
    {
        return false;
    }

    Out = std::move(It->second);
    AuthoritativeFrames.erase(It);
    return true;
}

// --- Determinism verification ----------------------------------------------

void LockstepSession::SubmitChecksum(PlayerId From, TickIndex Tick, uint64_t Checksum)
{
    // Only the authority adjudicates. On a client this is a no-op; the transport
    // adapter is what puts the value on the wire.
    if (!bInitialized || !bAuthority || Desync.bDetected)
    {
        return;
    }

    auto It = ReferenceChecksums.find(Tick);
    if (It == ReferenceChecksums.end())
    {
        ReferenceChecksums.emplace(Tick, Checksum);
        return;
    }

    if (It->second != Checksum)
    {
        Desync.bDetected = true;
        Desync.Tick = Tick;
        Desync.Player = From;
        Desync.Expected = It->second;
        Desync.Actual = Checksum;
    }
}

void LockstepSession::PruneUpToTick(TickIndex Tick)
{
    OutgoingCommands.erase(OutgoingCommands.begin(), OutgoingCommands.upper_bound(Tick));
    IncomingFrames.erase(IncomingFrames.begin(), IncomingFrames.upper_bound(Tick));
    AuthoritativeFrames.erase(AuthoritativeFrames.begin(), AuthoritativeFrames.upper_bound(Tick));
    ReferenceChecksums.erase(ReferenceChecksums.begin(), ReferenceChecksums.upper_bound(Tick));
}

size_t LockstepSession::GetPendingOutgoingCount() const
{
    size_t Total = 0;
    for (const auto& Entry : OutgoingCommands)
    {
        Total += Entry.second.size();
    }
    return Total;
}

const char* ToString(StallReason Reason)
{
    switch (Reason)
    {
    case StallReason::None:          return "None";
    case StallReason::AwaitingFrame: return "AwaitingFrame";
    case StallReason::NotStarted:    return "NotStarted";
    }
    return "Unknown";
}

} // namespace Net
} // namespace RA4
