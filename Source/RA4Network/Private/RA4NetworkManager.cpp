// Copyright (c) Red Alert 4 project.
#include "RA4NetworkManager.h"
#include "RA4NetworkChannel.h"

#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogRA4Net, Log, All);

void URA4NetworkManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void URA4NetworkManager::Deinitialize()
{
    EndMatch();
    Super::Deinitialize();
}

void URA4NetworkManager::BeginMatch(uint8 LocalPlayerIndex, uint8 NumPlayers, bool bIsServer,
                                    uint32 InputDelayTicks)
{
    Session.Initialize(LocalPlayerIndex, NumPlayers, bIsServer, InputDelayTicks);
    bDesyncReported = false;

    UE_LOG(LogRA4Net, Log, TEXT("Lockstep match begun: local player %d of %d, %s, input delay %u"),
           int32(LocalPlayerIndex), int32(NumPlayers), bIsServer ? TEXT("server") : TEXT("client"),
           Session.GetInputDelay());

    // Ticks below the input delay have nothing scheduled onto them -- the first order a
    // player can issue lands on tick InputDelay. Without an explicit empty frame for
    // each of them every peer stalls on tick 0 forever, waiting for a slice nobody is
    // ever going to send. Channels must be registered before this runs, or a client's
    // priming frames have no route to the server.
    for (uint32 PrimeTick = 0; PrimeTick < Session.GetInputDelay(); ++PrimeTick)
    {
        FlushLocalFrame(PrimeTick);
    }
}

void URA4NetworkManager::EndMatch()
{
    Session.Reset();
    Channels.Empty();
    bDesyncReported = false;
}

void URA4NetworkManager::RegisterChannel(uint8 PlayerIndex, URA4NetworkChannel* Channel)
{
    if (Channel != nullptr)
    {
        Channels.Add(PlayerIndex, Channel);
    }
}

void URA4NetworkManager::UnregisterChannel(uint8 PlayerIndex)
{
    Channels.Remove(PlayerIndex);
}

namespace
{

/** The channel for a given slot, or null if that player has left. */
URA4NetworkChannel* ResolveChannel(const TMap<uint8, TWeakObjectPtr<URA4NetworkChannel>>& Channels,
                                   uint8 PlayerIndex)
{
    if (const TWeakObjectPtr<URA4NetworkChannel>* Found = Channels.Find(PlayerIndex))
    {
        return Found->Get();
    }
    return nullptr;
}

} // namespace

// --- Called by the simulation subsystem ------------------------------------

uint32 URA4NetworkManager::SendCommandToServer(const RA4::Command& Cmd, uint32 CurrentTick)
{
    return Session.SubmitLocalCommand(CurrentTick, Cmd);
}

void URA4NetworkManager::FlushLocalFrame(uint32 TargetTick)
{
    if (!Session.IsInitialized())
    {
        return;
    }

    const RA4::CommandFrame Frame = Session.TakeOutgoingFrame(TargetTick);

    if (Session.IsAuthority())
    {
        // The server is a player too on a listen host, and its own slice never
        // touches the wire.
        HandlePlayerFrame(Session.GetLocalPlayer(), Frame);
        return;
    }

    if (URA4NetworkChannel* Channel = ResolveChannel(Channels, Session.GetLocalPlayer()))
    {
        Channel->SendFrameToServer(Frame);
    }
}

bool URA4NetworkManager::ConsumeFrameForTick(uint32 Tick, RA4::CommandFrame& OutFrame)
{
    return Session.TakeAuthoritativeFrame(Tick, OutFrame);
}

void URA4NetworkManager::SubmitStateChecksum(uint32 TickIndex, uint64 Checksum)
{
    if (!Session.IsInitialized())
    {
        return;
    }

    if (Session.IsAuthority())
    {
        Session.SubmitChecksum(Session.GetLocalPlayer(), TickIndex, Checksum);
        ReportDesyncIfNew();
        return;
    }

    if (URA4NetworkChannel* Channel = ResolveChannel(Channels, Session.GetLocalPlayer()))
    {
        Channel->SendChecksumToServer(TickIndex, Checksum);
    }
}

void URA4NetworkManager::PruneUpToTick(uint32 Tick)
{
    Session.PruneUpToTick(Tick);
}

// --- Called by URA4NetworkChannel on receipt -------------------------------

void URA4NetworkManager::HandlePlayerFrame(uint8 FromPlayer, const RA4::CommandFrame& Frame)
{
    Session.ReceivePlayerFrame(FromPlayer, Frame);
    TryBroadcastTick(Frame.Tick);
}

void URA4NetworkManager::HandleAuthoritativeFrame(const RA4::CommandFrame& Frame)
{
    Session.ReceiveAuthoritativeFrame(Frame);
}

void URA4NetworkManager::VerifyClientChecksum(int32 ClientId, uint32 TickIndex, uint64 Checksum)
{
    Session.SubmitChecksum(uint8(ClientId), TickIndex, Checksum);
    ReportDesyncIfNew();
}

void URA4NetworkManager::TryBroadcastTick(uint32 Tick)
{
    if (!Session.IsAuthority())
    {
        return;
    }

    RA4::CommandFrame Authoritative;
    if (!Session.AssembleAuthoritativeFrame(Tick, Authoritative))
    {
        // Still waiting on at least one player. Broadcasting a partial frame would
        // hand different command streams to different peers.
        return;
    }

    // The server executes the same frame it sends. Delivering to its own session is
    // idempotent -- the frame is stored keyed by tick -- so an accidental second
    // delivery through a locally-controlled channel cannot duplicate commands.
    Session.ReceiveAuthoritativeFrame(Authoritative);

    for (const TPair<uint8, TWeakObjectPtr<URA4NetworkChannel>>& Entry : Channels)
    {
        if (Entry.Key == Session.GetLocalPlayer())
        {
            continue;
        }
        if (URA4NetworkChannel* Channel = Entry.Value.Get())
        {
            Channel->SendAuthoritativeFrame(Authoritative);
        }
    }
}

void URA4NetworkManager::ReportDesyncIfNew()
{
    if (!Session.HasDesynced() || bDesyncReported)
    {
        return;
    }

    bDesyncReported = true;
    const RA4::Net::DesyncReport& Report = Session.GetDesync();

    UE_LOG(LogRA4Net, Error,
           TEXT("DESYNC at tick %u: player %d reported %llu, authority had %llu."),
           Report.Tick, int32(Report.Player),
           static_cast<unsigned long long>(Report.Actual),
           static_cast<unsigned long long>(Report.Expected));

    OnDesyncDetected.Broadcast(int32(Report.Tick), int32(Report.Player));
}
