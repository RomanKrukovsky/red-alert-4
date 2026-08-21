// Copyright (c) Red Alert 4 project. Rollback (GGPO-like) networking session implementation.
#include "RA4Simulation/RollbackSession.h"

#include <algorithm>

namespace RA4
{
namespace Net
{

void RollbackSession::Initialize(PlayerId InLocalPlayer, uint8_t InNumPlayers, bool bInIsAuthority,
                                TickIndex InLocalInputDelay,
                                TickIndex InMaxPredictionTicks,
                                RollbackMode InMode)
{
    Reset();
    LocalPlayer = InLocalPlayer;
    NumPlayers = InNumPlayers > 0 ? InNumPlayers : 1;
    bAuthority = bInIsAuthority;
    LocalInputDelay = InLocalInputDelay;
    MaxPredictionTicks = InMaxPredictionTicks;
    Mode = InMode;
    bInitialized = true;
    ConfirmedTick = 0;
}

void RollbackSession::Reset()
{
    bInitialized = false;
    bAuthority = false;
    LocalPlayer = kInvalidPlayer;
    NumPlayers = 0;
    LocalInputDelay = 0;
    MaxPredictionTicks = 15;
    Mode = RollbackMode::SpeculativeRollback;
    ConfirmedTick = 0;

    OutgoingCommands.clear();
    InputsPerTick.clear();
    ExecutedFrames.clear();
    ReferenceChecksums.clear();
    Desync = DesyncReport{};
    Stats = NetworkStats{};
}

TickIndex RollbackSession::SubmitLocalCommand(TickIndex CurrentTick, const Command& Cmd)
{
    if (!bInitialized || LocalPlayer >= NumPlayers)
    {
        return CurrentTick;
    }

    const TickIndex ExecutionTick = CurrentTick + LocalInputDelay;

    // Record locally for transmission
    OutgoingCommands[ExecutionTick].push_back(Cmd);

    // Record in local confirmed input map
    TickInputs& Inputs = GetOrCreateTickInputs(ExecutionTick);
    if (LocalPlayer < Inputs.Players.size())
    {
        Inputs.Players[LocalPlayer].bReceived = true;
        Inputs.Players[LocalPlayer].Commands.push_back(Cmd);
    }

    UpdateConfirmedTick();
    return ExecutionTick;
}

CommandFrame RollbackSession::TakeOutgoingFrame(TickIndex TargetTick)
{
    CommandFrame Frame;
    Frame.Tick = TargetTick;

    auto It = OutgoingCommands.find(TargetTick);
    if (It != OutgoingCommands.end())
    {
        Frame.Commands = std::move(It->second);
        OutgoingCommands.erase(It);
    }

    return Frame;
}

RollbackSession::TickInputs& RollbackSession::GetOrCreateTickInputs(TickIndex Tick)
{
    auto It = InputsPerTick.find(Tick);
    if (It == InputsPerTick.end())
    {
        TickInputs NewInputs;
        NewInputs.Players.resize(NumPlayers);
        auto Inserted = InputsPerTick.emplace(Tick, std::move(NewInputs));
        return Inserted.first->second;
    }
    return It->second;
}

const RollbackSession::TickInputs* RollbackSession::FindTickInputs(TickIndex Tick) const
{
    auto It = InputsPerTick.find(Tick);
    if (It != InputsPerTick.end())
    {
        return &It->second;
    }
    return nullptr;
}

void RollbackSession::UpdateConfirmedTick()
{
    while (true)
    {
        const TickIndex NextTickToCheck = ConfirmedTick;
        auto It = InputsPerTick.find(NextTickToCheck);
        if (It == InputsPerTick.end())
        {
            break;
        }

        bool bAllReported = true;
        for (uint8_t P = 0; P < NumPlayers; ++P)
        {
            if (P < It->second.Players.size() && !It->second.Players[P].bReceived)
            {
                bAllReported = false;
                break;
            }
        }

        if (bAllReported)
        {
            It->second.bAllConfirmed = true;
            ConfirmedTick++;
        }
        else
        {
            break;
        }
    }
}

bool RollbackSession::DoesFrameDiffer(const CommandFrame& Executed, PlayerId From,
                                      const std::vector<Command>& Actual) const
{
    std::vector<Command> ExecutedForPlayer;
    for (const auto& Cmd : Executed.Commands)
    {
        if (Cmd.Issuer == From)
        {
            ExecutedForPlayer.push_back(Cmd);
        }
    }

    if (ExecutedForPlayer.size() != Actual.size())
    {
        return true;
    }

    for (size_t I = 0; I < Actual.size(); ++I)
    {
        if (ExecutedForPlayer[I].Type != Actual[I].Type ||
            ExecutedForPlayer[I].Primary != Actual[I].Primary ||
            ExecutedForPlayer[I].Target != Actual[I].Target ||
            ExecutedForPlayer[I].Location != Actual[I].Location ||
            ExecutedForPlayer[I].Param != Actual[I].Param)
        {
            return true;
        }
    }

    return false;
}

RollbackEvent RollbackSession::ReceiveRemoteFrame(PlayerId From, const CommandFrame& Frame, SimWorld* World)
{
    RollbackEvent Event;
    if (!bInitialized || From >= NumPlayers || From == LocalPlayer)
    {
        return Event;
    }

    TickInputs& Inputs = GetOrCreateTickInputs(Frame.Tick);
    if (From < Inputs.Players.size())
    {
        Inputs.Players[From].bReceived = true;
        Inputs.Players[From].Commands = Frame.Commands;
    }

    UpdateConfirmedTick();

    // Check if this incoming frame is for a tick that has already been simulated
    if (World != nullptr && Frame.Tick < World->GetTick())
    {
        auto ExecutedIt = ExecutedFrames.find(Frame.Tick);
        if (ExecutedIt != ExecutedFrames.end())
        {
            const bool bNeedsRollback = DoesFrameDiffer(ExecutedIt->second, From, Frame.Commands);
            if (bNeedsRollback)
            {
                SimSnapshot Snapshot;
                if (World->GetSnapshotHistory().GetSnapshot(Frame.Tick, Snapshot))
                {
                    const TickIndex CurrentSimTick = World->GetTick();
                    const uint32_t ResimCount = static_cast<uint32_t>(CurrentSimTick - Frame.Tick);

                    // Restore state to the beginning of the divergent tick
                    World->RestoreFromSnapshot(Snapshot);

                    // Resimulate forward from Frame.Tick to CurrentSimTick
                    for (TickIndex T = Frame.Tick; T < CurrentSimTick; ++T)
                    {
                        World->RecordSnapshot();
                        CommandFrame ResimFrame = BuildFrameForTick(T);
                        World->Tick(&ResimFrame);
                        ExecutedFrames[T] = ResimFrame;
                    }

                    Stats.RollbackCount++;
                    Stats.MaxRollbackDepth = std::max(Stats.MaxRollbackDepth, ResimCount);

                    Event.bOccurred = true;
                    Event.RolledBackToTick = Frame.Tick;
                    Event.ResimulatedToTick = CurrentSimTick;
                    Event.ResimulatedTickCount = ResimCount;
                    Event.CausingPlayer = From;
                }
            }
        }
    }

    return Event;
}

bool RollbackSession::CanAdvance(TickIndex TargetTick) const
{
    if (!bInitialized)
    {
        return false;
    }

    if (Mode == RollbackMode::PureLockstep)
    {
        return TargetTick < ConfirmedTick;
    }

    // Speculative Rollback: allow running ahead up to MaxPredictionTicks beyond ConfirmedTick
    return TargetTick <= ConfirmedTick + MaxPredictionTicks;
}

CommandFrame RollbackSession::BuildFrameForTick(TickIndex Tick) const
{
    CommandFrame Frame;
    Frame.Tick = Tick;

    const TickInputs* Inputs = FindTickInputs(Tick);
    if (Inputs != nullptr)
    {
        for (uint8_t P = 0; P < NumPlayers; ++P)
        {
            if (P < Inputs->Players.size() && Inputs->Players[P].bReceived)
            {
                for (const auto& Cmd : Inputs->Players[P].Commands)
                {
                    Frame.Commands.push_back(Cmd);
                }
            }
        }
    }

    return Frame;
}

bool RollbackSession::AdvanceSimulation(SimWorld& World)
{
    const TickIndex SimTick = World.GetTick();
    if (!CanAdvance(SimTick))
    {
        Stats.StalledTickCount++;
        return false;
    }

    // Save pre-tick snapshot for potential future rollbacks
    World.RecordSnapshot();

    // Assemble and execute CommandFrame (with actual or predicted inputs)
    CommandFrame Frame = BuildFrameForTick(SimTick);
    World.Tick(&Frame);

    // Cache executed frame
    ExecutedFrames[SimTick] = Frame;

    UpdateConfirmedTick();
    return true;
}

void RollbackSession::UpdateNetworkLatency(PlayerId /*Peer*/, uint32_t RttMs, uint32_t JitterMs)
{
    Stats.RoundTripTimeMs = RttMs;
    Stats.JitterMs = JitterMs;

    // Dynamically calculate recommended input delay: 1 tick per 50ms one-way + 2*jitter safety
    const uint32_t OneWayLatencyMs = (RttMs / 2) + (JitterMs * 2);
    const TickIndex RecommendedDelay = static_cast<TickIndex>((OneWayLatencyMs + 49) / 50);
    Stats.RecommendedInputDelay = std::clamp(RecommendedDelay, TickIndex(0), TickIndex(5));
}

void RollbackSession::SubmitChecksum(PlayerId From, TickIndex Tick, uint64_t Checksum)
{
    if (!bInitialized)
    {
        return;
    }

    auto It = ReferenceChecksums.find(Tick);
    if (It == ReferenceChecksums.end())
    {
        ReferenceChecksums[Tick] = Checksum;
    }
    else if (It->second != Checksum)
    {
        if (!Desync.bDetected)
        {
            Desync.bDetected = true;
            Desync.Tick = Tick;
            Desync.Player = From;
            Desync.Expected = It->second;
            Desync.Actual = Checksum;
        }
    }
}

void RollbackSession::PruneUpToTick(TickIndex PruneTick)
{
    while (!OutgoingCommands.empty() && OutgoingCommands.begin()->first < PruneTick)
    {
        OutgoingCommands.erase(OutgoingCommands.begin());
    }

    while (!InputsPerTick.empty() && InputsPerTick.begin()->first < PruneTick)
    {
        InputsPerTick.erase(InputsPerTick.begin());
    }

    while (!ExecutedFrames.empty() && ExecutedFrames.begin()->first < PruneTick)
    {
        ExecutedFrames.erase(ExecutedFrames.begin());
    }

    while (!ReferenceChecksums.empty() && ReferenceChecksums.begin()->first < PruneTick)
    {
        ReferenceChecksums.erase(ReferenceChecksums.begin());
    }
}

} // namespace Net
} // namespace RA4
