// Copyright (c) Red Alert 4 project.
//
// Transport adapter over RA4::Net::LockstepSession. The protocol rules -- input
// delay, deterministic frame assembly, stalling, checksum adjudication -- all live
// in the engine-free session so the headless suite can play a whole match between
// two peers and assert they stay bit-identical (see TestNetwork.cpp). Nothing in
// this file decides anything about the protocol; it moves bytes and forwards calls.
//
// RPCs live on URA4NetworkChannel, not here: a UWorldSubsystem is not an AActor and
// cannot own replicated functions. The channel is a component on the PlayerController,
// which gives one owned, client-addressable endpoint per player.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "RA4Core/Command.h"
#include "RA4Simulation/LockstepSession.h"
#include "RA4NetworkManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCommandRejected, int32, ReasonCode);

// Raised on the server when two peers disagree about state. Carries the culprit tick
// and player, because "something desynced" is not a diagnosable report.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDesyncDetected, int32, Tick, int32, PlayerIndex);

class URA4NetworkChannel;

UCLASS()
class RA4NETWORK_API URA4NetworkManager : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /** Starts a lockstep match. Every peer must pass the same NumPlayers and
        InputDelay; disagreement about either desyncs the command stream itself,
        before the simulation has run a single step. */
    void BeginMatch(uint8 LocalPlayerIndex, uint8 NumPlayers, bool bIsServer,
                    uint32 InputDelayTicks = RA4::Net::kDefaultInputDelay);
    void EndMatch();

    UFUNCTION(BlueprintPure, Category = "Network")
    bool IsMatchActive() const { return Session.IsInitialized(); }

    bool IsServer() const { return Session.IsAuthority(); }
    uint32 GetInputDelay() const { return Session.GetInputDelay(); }
    uint8 GetLocalPlayerIndex() const { return Session.GetLocalPlayer(); }

    /** Registers a player's RPC endpoint. The server needs one per client so frames
        can be addressed; a client registers only its own. */
    void RegisterChannel(uint8 PlayerIndex, URA4NetworkChannel* Channel);
    void UnregisterChannel(uint8 PlayerIndex);

    // --- Called by the simulation subsystem --------------------------------

    /** Schedules a locally issued command and returns the tick it will execute on.
        The caller must not apply it locally -- it comes back in the authoritative
        frame with everyone else's, which is what keeps the peers identical.
        CurrentTick is the tick the player was looking at when they issued it. */
    uint32 SendCommandToServer(const RA4::Command& Cmd, uint32 CurrentTick);

    /** Sends this peer's slice for TargetTick, empty or not. An unsent empty frame is
        indistinguishable from packet loss and would stall every other player. */
    void FlushLocalFrame(uint32 TargetTick);

    /** Whether the simulation may run Tick. False means wait -- do not tick locally,
        or this peer leaves the others behind and the match is over. */
    bool CanAdvanceToTick(uint32 Tick) const { return Session.CanAdvance(Tick); }

    /** Consumes the authoritative frame for Tick. False if it has not arrived. */
    bool ConsumeFrameForTick(uint32 Tick, RA4::CommandFrame& OutFrame);

    /** Reports the local post-tick checksum. Adjudicated directly on the server; sent
        over the wire on a client. */
    void SubmitStateChecksum(uint32 TickIndex, uint64 Checksum);

    /** Retires bookkeeping for ticks at or below Tick, which otherwise grows for the
        whole match. */
    void PruneUpToTick(uint32 Tick);

    // --- Called by URA4NetworkChannel on receipt ---------------------------

    /** Server: a player's command slice arrived. Once every player has reported for
        that tick the frame is assembled and broadcast. */
    void HandlePlayerFrame(uint8 FromPlayer, const RA4::CommandFrame& Frame);

    /** Client: the authoritative frame arrived. */
    void HandleAuthoritativeFrame(const RA4::CommandFrame& Frame);

    /** Server: a client reported its checksum for a tick. */
    void VerifyClientChecksum(int32 ClientId, uint32 TickIndex, uint64 Checksum);

    UPROPERTY(BlueprintAssignable, Category = "Network")
    FOnCommandRejected OnCommandRejected;

    UPROPERTY(BlueprintAssignable, Category = "Network")
    FOnDesyncDetected OnDesyncDetected;

    UFUNCTION(BlueprintPure, Category = "Network")
    bool HasDesynced() const { return Session.HasDesynced(); }

private:
    /** Assembles Tick if it is complete and sends it to every registered channel. */
    void TryBroadcastTick(uint32 Tick);

    /** Raises OnDesyncDetected once, the first time the session reports one. */
    void ReportDesyncIfNew();

    RA4::Net::LockstepSession Session;

    // Keyed by player slot. The server holds every client's channel; a client holds
    // only its own. Weak because the PlayerController owning a channel can leave
    // mid-match and must not be kept alive by this map.
    UPROPERTY()
    TMap<uint8, TWeakObjectPtr<URA4NetworkChannel>> Channels;

    bool bDesyncReported = false;
};
