// Copyright (c) Red Alert 4 project. Official .ra4rep Binary Replay Format & Desync Verifier.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "RA4Core/ByteStream.h"
#include "RA4Core/Fixed.h"
#include "RA4Core/Ids.h"
#include "RA4Simulation/SimTypes.h"
#include "RA4Simulation/SimWorld.h"

#ifndef RA4REPLAY_API
#define RA4REPLAY_API
#endif

namespace RA4
{

constexpr uint32_t kReplayMagic = 0x52413452; // 'RA4R'
constexpr uint32_t kReplayFormatVersion = 1;

struct ReplayCheckpoint
{
    TickIndex Tick = 0;
    uint64_t StateChecksum = 0;
};

struct ReplayTimedCommand
{
    TickIndex Tick = 0;
    Command Cmd;
};

struct RA4REPLAY_API ReplayContainer
{
    // Header
    uint32_t Magic = kReplayMagic;
    uint32_t Version = kReplayFormatVersion;
    std::string MapName = "DefaultMap";
    uint64_t InitialSeed = 42;
    uint32_t MatchDurationTicks = 0;
    uint32_t PlayerCount = 2;

    // Stream
    std::vector<ReplayCheckpoint> Checkpoints;
    std::vector<ReplayTimedCommand> Commands;

    void Serialize(ByteWriter& W) const;
    bool Deserialize(ByteReader& R);
};

class RA4REPLAY_API ReplayIntegrityVerifier
{
public:
    /** Plays back a replay against a SimWorld instance and verifies every checkpoint for zero desync. */
    static bool VerifyBitExactIntegrity(const ReplayContainer& Replay, SimWorld& World);
};

} // namespace RA4
