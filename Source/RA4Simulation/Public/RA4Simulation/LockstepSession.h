// Copyright (c) Red Alert 4 project. Deterministic lockstep session.
//
// Lockstep works because every peer runs the identical simulation over the identical
// command stream. This class owns the part of that which is not the simulation: who
// has sent what for which tick, when a peer is allowed to advance, and whether the
// peers still agree.
//
// Deliberately free of any engine dependency. The Unreal layer
// (URA4NetworkManager) is a transport adapter over this; the rules live here so the
// headless suite can play a whole match between two peers in-process and assert they
// stay bit-identical. A desync that only reproduces inside a running editor is a
// desync you cannot debug.
//
// Two properties are load-bearing and everything else follows from them:
//
//   1. Input delay. A command issued while tick T is current does not execute on T.
//      It is scheduled for T + InputDelay, which gives the packet time to reach every
//      peer before the tick that consumes it. Without this a peer would have to stall
//      on every single order.
//
//   2. Deterministic assembly order. The authoritative frame is built by walking
//      players in slot order, never in packet-arrival order. Two servers given the
//      same packets in a different order must still produce byte-identical frames,
//      otherwise the command stream itself desyncs before the simulation ever runs.
#pragma once

#include <cstdint>
#include <map>
#include <vector>

#include "RA4Core/Command.h"
#include "RA4Core/Ids.h"

namespace RA4
{

#ifndef RA4SIMULATION_API
#define RA4SIMULATION_API
#endif

namespace Net
{

/** Default number of ticks between issuing a command and executing it. At the 20 Hz
    simulation rate this is 100 ms of cover for latency, which is the usual starting
    point for a lockstep RTS and is renegotiable per match. */
constexpr TickIndex kDefaultInputDelay = 2;

/** Why a peer cannot advance to the tick it wants to run. */
enum class StallReason : uint8_t
{
    None = 0,        // the authoritative frame is present; run the tick
    AwaitingFrame,   // the server has not delivered this tick's frame yet
    NotStarted,      // Initialize has not been called
};

/** A checksum disagreement, recorded the first time it happens.

    Only the first is kept. After one desync every subsequent tick is meaningless, so
    a running tally would say nothing except how long it took to notice. */
struct DesyncReport
{
    bool bDetected = false;
    TickIndex Tick = 0;
    PlayerId Player = kInvalidPlayer;
    uint64_t Expected = 0;   // the authority's checksum
    uint64_t Actual = 0;     // what the disagreeing peer reported
};

/** One participant in a lockstep match: a client, or the server, or both at once for
    a listen server. Server-only calls are inert on a peer that is not the authority,
    so the same object is used on both sides without a branch at every call site. */
class RA4SIMULATION_API LockstepSession
{
public:
    LockstepSession() = default;

    /** NumPlayers counts command-issuing slots, so the server knows when a tick's
        frame is complete. bIsAuthority makes this peer the one that assembles frames
        and adjudicates checksums. */
    void Initialize(PlayerId InLocalPlayer, uint8_t InNumPlayers, bool bIsAuthority,
                    TickIndex InInputDelay = kDefaultInputDelay);
    void Reset();

    bool IsInitialized() const { return bInitialized; }
    bool IsAuthority() const { return bAuthority; }
    PlayerId GetLocalPlayer() const { return LocalPlayer; }
    uint8_t GetNumPlayers() const { return NumPlayers; }
    TickIndex GetInputDelay() const { return InputDelay; }

    // --- Local input -------------------------------------------------------

    /** Schedules a locally issued command and returns the tick it will execute on.
        The caller does not apply it to its own world; it arrives back with everyone
        else's in the authoritative frame. */
    TickIndex SubmitLocalCommand(TickIndex CurrentTick, const Command& Cmd);

    /** Hands over this peer's own commands for TargetTick, ready to send. Always
        succeeds, because "I issued nothing" still has to be transmitted: the server
        cannot distinguish an idle player from a lagging one otherwise, and would
        stall the whole match waiting for a frame that was never coming. */
    CommandFrame TakeOutgoingFrame(TickIndex TargetTick);

    // --- Server side -------------------------------------------------------

    /** Records one player's contribution to TargetTick. Ignored on a non-authority
        peer, and ignored if that player already reported for that tick — a duplicate
        packet must not double-apply its commands. */
    void ReceivePlayerFrame(PlayerId From, const CommandFrame& Frame);

    /** True once every player has reported for Tick. */
    bool IsFrameComplete(TickIndex Tick) const;

    /** Builds the authoritative frame for Tick by walking players in slot order.
        Returns false while the frame is still incomplete. */
    bool AssembleAuthoritativeFrame(TickIndex Tick, CommandFrame& Out);

    // --- Client side -------------------------------------------------------

    /** Accepts the authoritative frame from the server. */
    void ReceiveAuthoritativeFrame(const CommandFrame& Frame);

    /** Whether the simulation may run Tick, and if not, why. */
    StallReason GetStallReason(TickIndex Tick) const;
    bool CanAdvance(TickIndex Tick) const { return GetStallReason(Tick) == StallReason::None; }

    /** Consumes the authoritative frame for Tick. Returns false if it has not
        arrived; the caller must not tick the simulation in that case. */
    bool TakeAuthoritativeFrame(TickIndex Tick, CommandFrame& Out);

    // --- Determinism verification ------------------------------------------

    /** Reports a peer's state checksum after it finished Tick. On the authority the
        first checksum seen for a tick becomes the reference and every later one is
        compared against it. */
    void SubmitChecksum(PlayerId From, TickIndex Tick, uint64_t Checksum);

    const DesyncReport& GetDesync() const { return Desync; }
    bool HasDesynced() const { return Desync.bDetected; }

    /** Drops bookkeeping for ticks at or below Tick. Called once a tick is retired
        on every peer; without it a long match grows these maps without bound. */
    void PruneUpToTick(TickIndex Tick);

    size_t GetPendingOutgoingCount() const;
    size_t GetBufferedAuthoritativeCount() const { return AuthoritativeFrames.size(); }

private:
    struct PendingTick
    {
        // Indexed by player slot; a slot is only counted once it has reported, so an
        // empty command list and a missing packet stay distinguishable.
        std::vector<CommandFrame> PerPlayer;
        std::vector<bool> bReported;
        uint8_t ReportedCount = 0;
    };

    PendingTick& TickSlot(TickIndex Tick);

    bool bInitialized = false;
    bool bAuthority = false;
    PlayerId LocalPlayer = kInvalidPlayer;
    uint8_t NumPlayers = 0;
    TickIndex InputDelay = kDefaultInputDelay;

    // Local commands not yet sent, keyed by the tick they will execute on.
    std::map<TickIndex, std::vector<Command>> OutgoingCommands;

    // Server: partial frames still collecting player reports.
    std::map<TickIndex, PendingTick> IncomingFrames;

    // Client: assembled frames delivered by the server, awaiting execution.
    std::map<TickIndex, CommandFrame> AuthoritativeFrames;

    // Authority: the reference checksum per tick, and who set it.
    std::map<TickIndex, uint64_t> ReferenceChecksums;

    DesyncReport Desync;
};

const char* ToString(StallReason Reason);

} // namespace Net
} // namespace RA4
