// Copyright (c) Red Alert 4 project. Rollback (GGPO-like) networking session.
#pragma once

#include <cstdint>
#include <map>
#include <vector>
#include <string>

#include "RA4Core/Command.h"
#include "RA4Core/Ids.h"
#include "RA4Simulation/LockstepSession.h"
#include "RA4Simulation/SimSnapshot.h"
#include "RA4Simulation/SimWorld.h"


namespace RA4
{

#ifndef RA4SIMULATION_API
#define RA4SIMULATION_API
#endif

namespace Net
{

/** Network operation mode */
enum class RollbackMode : uint8_t
{
    PureLockstep = 0,       // Strict lockstep: stall if frames are missing
    SpeculativeRollback = 1 // Speculative execution: predict inputs and rollback on mismatch
};

/** Summary of a rollback resimulation event */
struct RollbackEvent
{
    bool bOccurred = false;
    TickIndex RolledBackToTick = 0;
    TickIndex ResimulatedToTick = 0;
    uint32_t ResimulatedTickCount = 0;
    PlayerId CausingPlayer = kInvalidPlayer;
};

/** Jitter and network latency diagnostics */
struct NetworkStats
{
    uint32_t RoundTripTimeMs = 0;
    uint32_t JitterMs = 0;
    uint32_t RollbackCount = 0;
    uint32_t MaxRollbackDepth = 0;
    uint32_t StalledTickCount = 0;
    TickIndex RecommendedInputDelay = 0;
};

/** Rollback Session managing local input scheduling, remote frame ingestion,
    speculative ticking, snapshot rollback, and determinism verification. */
class RA4SIMULATION_API RollbackSession
{
public:
    RollbackSession() = default;

    /** Initializes the session.
        - InLocalPlayer: local player ID
        - InNumPlayers: total active players
        - InIsAuthority: true if authoritative server / host
        - InLocalInputDelay: local input execution delay (0 for immediate prediction)
        - InMaxPredictionTicks: maximum forward ticks before forcing stall (default 15 ~ 750ms)
        - InMode: pure lockstep vs speculative rollback */
    void Initialize(PlayerId InLocalPlayer, uint8_t InNumPlayers, bool bInIsAuthority,
                    TickIndex InLocalInputDelay = 0,
                    TickIndex InMaxPredictionTicks = 15,
                    RollbackMode InMode = RollbackMode::SpeculativeRollback);

    void Reset();

    bool IsInitialized() const { return bInitialized; }
    bool IsAuthority() const { return bAuthority; }
    PlayerId GetLocalPlayer() const { return LocalPlayer; }
    uint8_t GetNumPlayers() const { return NumPlayers; }
    TickIndex GetLocalInputDelay() const { return LocalInputDelay; }
    TickIndex GetMaxPredictionTicks() const { return MaxPredictionTicks; }
    RollbackMode GetMode() const { return Mode; }
    TickIndex GetConfirmedTick() const { return ConfirmedTick; }

    void SetMode(RollbackMode InMode) { Mode = InMode; }
    void SetLocalInputDelay(TickIndex Delay) { LocalInputDelay = Delay; }
    void SetMaxPredictionTicks(TickIndex MaxTicks) { MaxPredictionTicks = MaxTicks; }

    // --- Local Input Submission --------------------------------------------

    /** Submits a local command and queues it for execution at CurrentTick + LocalInputDelay.
        Returns the scheduled execution tick. */
    TickIndex SubmitLocalCommand(TickIndex CurrentTick, const Command& Cmd);

    /** Extracts outgoing command frame for TargetTick to transmit to peers. */
    CommandFrame TakeOutgoingFrame(TickIndex TargetTick);

    // --- Remote Frame Ingestion & Rollback ----------------------------------

    /** Ingests a remote player's command frame for a specific tick.
        If World is provided and the incoming frame diverges from past predictions,
        automatically performs a rollback and resimulation up to World's current tick.
        Returns RollbackEvent describing any resimulation performed. */
    RollbackEvent ReceiveRemoteFrame(PlayerId From, const CommandFrame& Frame, SimWorld* World = nullptr);

    /** Checks whether simulation can advance to TargetTick without violating prediction limits. */
    bool CanAdvance(TickIndex TargetTick) const;

    /** Builds the merged CommandFrame for TickIndex (using confirmed inputs or predicted defaults). */
    CommandFrame BuildFrameForTick(TickIndex Tick) const;

    /** Advances simulation by one tick:
        1. Validates prediction limits.
        2. Records pre/post snapshot into World history.
        3. Executes tick with predicted or confirmed CommandFrame.
        4. Caches executed frame for future rollback divergence detection. */
    bool AdvanceSimulation(SimWorld& World);

    // --- Adaptive Jitter Buffer & Latency -----------------------------------

    /** Updates network RTT and jitter metrics to adaptively tune input delay. */
    void UpdateNetworkLatency(PlayerId Peer, uint32_t RttMs, uint32_t JitterMs);

    /** Returns current network and rollback performance statistics. */
    const NetworkStats& GetStats() const { return Stats; }

    // --- Checksum & Desync Adjudication -------------------------------------

    /** Submits state checksum for verified confirmed ticks. */
    void SubmitChecksum(PlayerId From, TickIndex Tick, uint64_t Checksum);

    const DesyncReport& GetDesync() const { return Desync; }
    bool HasDesynced() const { return Desync.bDetected; }

    /** Prunes historical commands and executed frames older than PruneTick. */
    void PruneUpToTick(TickIndex PruneTick);

private:
    struct PlayerTickInput
    {
        bool bReceived = false;
        std::vector<Command> Commands;
    };

    struct TickInputs
    {
        std::vector<PlayerTickInput> Players;
        bool bAllConfirmed = false;
    };

    TickInputs& GetOrCreateTickInputs(TickIndex Tick);
    const TickInputs* FindTickInputs(TickIndex Tick) const;
    void UpdateConfirmedTick();
    bool DoesFrameDiffer(const CommandFrame& Executed, PlayerId From, const std::vector<Command>& Actual) const;

    bool bInitialized = false;
    bool bAuthority = false;
    PlayerId LocalPlayer = kInvalidPlayer;
    uint8_t NumPlayers = 0;
    TickIndex LocalInputDelay = 0;
    TickIndex MaxPredictionTicks = 15;
    RollbackMode Mode = RollbackMode::SpeculativeRollback;

    TickIndex ConfirmedTick = 0;

    // Outgoing local commands awaiting transmission: [Tick -> Commands]
    std::map<TickIndex, std::vector<Command>> OutgoingCommands;

    // Confirmed and received inputs from all players: [Tick -> TickInputs]
    std::map<TickIndex, TickInputs> InputsPerTick;

    // History of executed CommandFrames (for rollback comparison): [Tick -> Frame]
    std::map<TickIndex, CommandFrame> ExecutedFrames;

    // Checksums for confirmed ticks: [Tick -> Checksum]
    std::map<TickIndex, uint64_t> ReferenceChecksums;

    DesyncReport Desync;
    NetworkStats Stats;
};

} // namespace Net
} // namespace RA4
