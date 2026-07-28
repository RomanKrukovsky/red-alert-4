// Copyright (c) Red Alert 4 project. Replay capture and playback.
//
// A replay is the match setup plus the command stream -- nothing else. Because the
// simulation is deterministic, replaying the commands reproduces the match exactly.
// That makes a replay file roughly a hundred kilobytes for a long game instead of
// hundreds of megabytes of state, and it makes every replay a regression test.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "RA4Core/ByteStream.h"
#include "RA4Core/Command.h"
#include "RA4Simulation/SimWorld.h"

namespace RA4
{

// Bump on any change to the tick order, the command layout or the checksum
// contents. Old replays are then rejected up front instead of desyncing.
constexpr uint32_t kReplayFormatVersion = 1;
constexpr uint32_t kReplayMagic = 0x34414952;   // 'RA4R'

struct ReplayHeader
{
    uint32_t Magic = kReplayMagic;
    uint32_t FormatVersion = kReplayFormatVersion;
    std::string GameVersion;
    uint64_t ContentHash = 0;
    uint64_t Seed = 0;
    std::string MapName;
    int32_t MapWidth = 0;
    int32_t MapHeight = 0;
    std::vector<uint8_t> MapTiles;

    struct PlayerEntry
    {
        bool bActive = false;
        uint8_t Faction = 0;
        int32_t StartingCredits = 0;
        std::string Name;
    };
    PlayerEntry Players[kMaxPlayers];
};

// A checkpoint lets playback detect where a divergence started instead of only
// that one happened, and lets a spectator seek without replaying from tick zero
// once state snapshots land.
struct ReplayCheckpoint
{
    TickIndex Tick = 0;
    uint64_t Checksum = 0;
};

class ReplayRecorder
{
public:
    void Begin(const ReplayHeader& InHeader);
    // Only non-empty frames are stored; a 30 minute match is mostly idle ticks.
    void RecordFrame(const CommandFrame& Frame);
    void RecordCheckpoint(TickIndex Tick, uint64_t Checksum);
    void End(TickIndex FinalTick, PlayerId Winner);

    std::vector<uint8_t> Serialize() const;
    bool SaveToFile(const std::string& Path) const;

    const ReplayHeader& GetHeader() const { return Header; }
    size_t GetFrameCount() const { return Frames.size(); }

private:
    ReplayHeader Header;
    std::vector<CommandFrame> Frames;
    std::vector<ReplayCheckpoint> Checkpoints;
    TickIndex FinalTick = 0;
    PlayerId Winner = kInvalidPlayer;
};

struct ReplayData
{
    ReplayHeader Header;
    std::vector<CommandFrame> Frames;
    std::vector<ReplayCheckpoint> Checkpoints;
    TickIndex FinalTick = 0;
    PlayerId Winner = kInvalidPlayer;
};

// Returns false and fills OutError on a truncated file, wrong magic or a format
// version this build cannot interpret.
bool DeserializeReplay(const std::vector<uint8_t>& Bytes, ReplayData& Out, std::string& OutError);
bool LoadReplayFromFile(const std::string& Path, ReplayData& Out, std::string& OutError);

struct ReplayVerifyResult
{
    bool bSucceeded = false;
    TickIndex DivergedAtTick = 0;
    uint64_t ExpectedChecksum = 0;
    uint64_t ActualChecksum = 0;
    TickIndex TicksPlayed = 0;
    PlayerId Winner = kInvalidPlayer;
    std::string Error;
};

// Replays the command stream into a fresh SimWorld and compares every recorded
// checkpoint. This is the determinism regression test the CI runs on every commit.
ReplayVerifyResult VerifyReplay(const ReplayData& Replay, const ContentDatabase& Content);

// Builds the header from a match setup so recording sites cannot forget a field.
ReplayHeader MakeHeaderFromSetup(const MatchSetup& Setup, const ContentDatabase& Content,
                                 const std::string& GameVersion);

} // namespace RA4
