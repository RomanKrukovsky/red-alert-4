// Copyright (c) Red Alert 4 project.
#include "RA4Replay/Replay.h"

#include "RA4Core/SimConfig.h"

#include <cstdio>

namespace RA4
{

namespace
{
void SerializeHeader(ByteWriter& W, const ReplayHeader& H)
{
    W.WriteUInt32(H.Magic);
    W.WriteUInt32(H.FormatVersion);
    W.WriteString(H.GameVersion);
    W.WriteUInt64(H.ContentHash);
    W.WriteUInt64(H.Seed);
    W.WriteString(H.MapName);
    W.WriteInt32(H.MapWidth);
    W.WriteInt32(H.MapHeight);
    W.WriteUInt32(uint32_t(H.MapTiles.size()));
    W.WriteBytes(H.MapTiles.data(), H.MapTiles.size());
    for (const ReplayHeader::PlayerEntry& P : H.Players)
    {
        W.WriteBool(P.bActive);
        W.WriteUInt8(P.Team);
        W.WriteUInt8(P.Faction);
        W.WriteInt32(P.StartingCredits);
        W.WriteInt32(P.StartPositionIndex);
        W.WriteString(P.Name);
    }
    W.WriteBool(H.bReconEnabled);
    W.WriteUInt64(H.ReconSettingsHash);
}

bool DeserializeHeader(ByteReader& R, ReplayHeader& H, std::string& OutError)
{
    H.Magic = R.ReadUInt32();
    if (H.Magic != kReplayMagic)
    {
        OutError = "not a Red Alert 4 replay (bad magic)";
        return false;
    }
    H.FormatVersion = R.ReadUInt32();
    if (H.FormatVersion != kReplayFormatVersion)
    {
        OutError = "replay format version " + std::to_string(H.FormatVersion) + " cannot be played by this build (expected " +
                   std::to_string(kReplayFormatVersion) + ")";
        return false;
    }
    H.GameVersion = R.ReadString();
    H.ContentHash = R.ReadUInt64();
    H.Seed = R.ReadUInt64();
    H.MapName = R.ReadString();
    H.MapWidth = R.ReadInt32();
    H.MapHeight = R.ReadInt32();

    const uint32_t TileCount = R.ReadUInt32();
    if (R.HasError() || TileCount > uint32_t(kMaxMapTiles) * uint32_t(kMaxMapTiles))
    {
        OutError = "replay map data is corrupt or oversized";
        return false;
    }
    H.MapTiles.resize(TileCount);
    for (uint32_t I = 0; I < TileCount; ++I)
    {
        H.MapTiles[I] = R.ReadUInt8();
    }

    for (ReplayHeader::PlayerEntry& P : H.Players)
    {
        P.bActive = R.ReadBool();
        P.Team = R.ReadUInt8();
        P.Faction = R.ReadUInt8();
        P.StartingCredits = R.ReadInt32();
        P.StartPositionIndex = R.ReadInt32();
        P.Name = R.ReadString();
    }

    H.bReconEnabled = R.ReadBool();
    H.ReconSettingsHash = R.ReadUInt64();
    if (!H.bReconEnabled && H.ReconSettingsHash != 0)
    {
        // A hash without the flag is either corruption or tampering; there is no
        // legitimate writer that produces it (MakeHeaderFromSetup sets both or
        // neither), so refuse rather than guess.
        OutError = "replay header is inconsistent: recon settings hash present but recon disabled";
        return false;
    }

    if (R.HasError())
    {
        OutError = "replay header is truncated";
        return false;
    }
    return true;
}
} // namespace

ReplayHeader MakeHeaderFromSetup(const MatchSetup& Setup, const ContentDatabase& Content,
                                 const std::string& GameVersion,
                                 const Recon::ReconSettings* ReconSettings)
{
    ReplayHeader H;
    H.GameVersion = GameVersion;
    H.ContentHash = Content.ComputeContentHash();
    H.Seed = Setup.Seed;
    H.MapName = Setup.Map.Name;
    H.MapWidth = Setup.Map.Width;
    H.MapHeight = Setup.Map.Height;
    H.MapTiles = Setup.Map.Tiles;
    for (int32_t I = 0; I < kMaxPlayers; ++I)
    {
        H.Players[I].bActive = Setup.Players[I].bActive;
        H.Players[I].Team = Setup.Players[I].Team;
        H.Players[I].Faction = uint8_t(Setup.Players[I].Faction);
        H.Players[I].StartingCredits = Setup.Players[I].StartingCredits;
        H.Players[I].StartPositionIndex = Setup.Players[I].StartPositionIndex;
    }
    if (ReconSettings != nullptr && ReconSettings->bEnabled)
    {
        H.bReconEnabled = true;
        H.ReconSettingsHash = ReconSettings->ComputeSettingsHash();
    }
    return H;
}

void ReplayRecorder::Begin(const ReplayHeader& InHeader)
{
    Header = InHeader;
    Frames.clear();
    Checkpoints.clear();
    FinalTick = 0;
    Winner = kInvalidPlayer;
}

void ReplayRecorder::RecordFrame(const CommandFrame& Frame)
{
    if (Frame.Commands.empty())
    {
        return;
    }
    Frames.push_back(Frame);
}

void ReplayRecorder::RecordCheckpoint(TickIndex Tick, uint64_t Checksum)
{
    Checkpoints.push_back(ReplayCheckpoint{Tick, Checksum});
}

void ReplayRecorder::End(TickIndex InFinalTick, PlayerId InWinner)
{
    FinalTick = InFinalTick;
    Winner = InWinner;
}

std::vector<uint8_t> ReplayRecorder::Serialize() const
{
    ByteWriter W;
    SerializeHeader(W, Header);

    W.WriteUInt32(uint32_t(Frames.size()));
    for (const CommandFrame& F : Frames)
    {
        F.Serialize(W);
    }

    W.WriteUInt32(uint32_t(Checkpoints.size()));
    for (const ReplayCheckpoint& C : Checkpoints)
    {
        W.WriteUInt32(C.Tick);
        W.WriteUInt64(C.Checksum);
    }

    W.WriteUInt32(FinalTick);
    W.WriteUInt8(Winner);
    return W.GetBuffer();
}

bool ReplayRecorder::SaveToFile(const std::string& Path) const
{
    const std::vector<uint8_t> Bytes = Serialize();
    std::FILE* File = std::fopen(Path.c_str(), "wb");
    if (File == nullptr)
    {
        return false;
    }
    const size_t Written = std::fwrite(Bytes.data(), 1, Bytes.size(), File);
    std::fclose(File);
    return Written == Bytes.size();
}

bool DeserializeReplay(const std::vector<uint8_t>& Bytes, ReplayData& Out, std::string& OutError)
{
    ByteReader R(Bytes);
    if (!DeserializeHeader(R, Out.Header, OutError))
    {
        return false;
    }

    const uint32_t FrameCount = R.ReadUInt32();
    if (R.HasError())
    {
        OutError = "replay is truncated before the command stream";
        return false;
    }
    Out.Frames.clear();
    Out.Frames.reserve(FrameCount);
    for (uint32_t I = 0; I < FrameCount; ++I)
    {
        Out.Frames.push_back(CommandFrame::Deserialize(R));
        if (R.HasError())
        {
            OutError = "replay command stream is truncated at frame " + std::to_string(I);
            return false;
        }
    }

    const uint32_t CheckpointCount = R.ReadUInt32();
    Out.Checkpoints.clear();
    Out.Checkpoints.reserve(CheckpointCount);
    for (uint32_t I = 0; I < CheckpointCount; ++I)
    {
        ReplayCheckpoint C;
        C.Tick = R.ReadUInt32();
        C.Checksum = R.ReadUInt64();
        Out.Checkpoints.push_back(C);
    }

    Out.FinalTick = R.ReadUInt32();
    Out.Winner = R.ReadUInt8();

    if (R.HasError())
    {
        OutError = "replay trailer is truncated";
        return false;
    }
    return true;
}

bool LoadReplayFromFile(const std::string& Path, ReplayData& Out, std::string& OutError)
{
    std::FILE* File = std::fopen(Path.c_str(), "rb");
    if (File == nullptr)
    {
        OutError = "cannot open " + Path;
        return false;
    }
    std::fseek(File, 0, SEEK_END);
    const long Size = std::ftell(File);
    std::fseek(File, 0, SEEK_SET);
    if (Size <= 0)
    {
        std::fclose(File);
        OutError = "replay file is empty";
        return false;
    }
    std::vector<uint8_t> Bytes(static_cast<size_t>(Size));
    const size_t Read = std::fread(Bytes.data(), 1, Bytes.size(), File);
    std::fclose(File);
    if (Read != Bytes.size())
    {
        OutError = "short read on replay file";
        return false;
    }
    return DeserializeReplay(Bytes, Out, OutError);
}

ReplayVerifyResult VerifyReplay(const ReplayData& Replay, const ContentDatabase& Content,
                                const Recon::ReconSettings* ReconSettings)
{
    ReplayVerifyResult Result;

    // A content mismatch guarantees divergence, so refuse before wasting the run.
    const uint64_t LocalContentHash = Content.ComputeContentHash();
    if (LocalContentHash != Replay.Header.ContentHash)
    {
        Result.Error = "content hash mismatch: replay was recorded with different game data";
        return Result;
    }

    // Recon ruleset identity (I-B5). Belief state is checksummed, so replaying
    // an intel-enabled recording without the identical settings is a guaranteed
    // checkpoint divergence -- refuse it as loudly as a content mismatch.
    const bool bLocalIntel = ReconSettings != nullptr && ReconSettings->bEnabled;
    if (Replay.Header.bReconEnabled != bLocalIntel)
    {
        Result.Error = Replay.Header.bReconEnabled
                           ? "replay was recorded with the recon layer enabled; supply the matching "
                             "ReconSettings with bEnabled=true"
                           : "replay was recorded without the recon layer; do not supply enabled ReconSettings";
        return Result;
    }
    if (bLocalIntel && ReconSettings->ComputeSettingsHash() != Replay.Header.ReconSettingsHash)
    {
        Result.Error = "recon settings hash mismatch: replay was recorded under a different recon ruleset";
        return Result;
    }

    MatchSetup Setup;
    Setup.Seed = Replay.Header.Seed;
    Setup.Map.Name = Replay.Header.MapName;
    Setup.Map.Width = Replay.Header.MapWidth;
    Setup.Map.Height = Replay.Header.MapHeight;
    Setup.Map.Tiles = Replay.Header.MapTiles;
    for (int32_t I = 0; I < kMaxPlayers; ++I)
    {
        Setup.Players[I].bActive = Replay.Header.Players[I].bActive;
        Setup.Players[I].Team = Replay.Header.Players[I].Team;
        Setup.Players[I].Faction = FactionId(Replay.Header.Players[I].Faction);
        Setup.Players[I].StartingCredits = Replay.Header.Players[I].StartingCredits;
        Setup.Players[I].StartPositionIndex = Replay.Header.Players[I].StartPositionIndex;
    }

    SimWorld World;
    World.Initialize(&Content, Setup, ReconSettings);

    // The recorder stores only non-empty frames, so playback walks the frame list
    // in parallel with the tick counter rather than indexing by tick.
    size_t NextFrame = 0;
    size_t NextCheckpoint = 0;

    for (TickIndex Tick = 0; Tick < Replay.FinalTick; ++Tick)
    {
        const CommandFrame* Frame = nullptr;
        if (NextFrame < Replay.Frames.size() && Replay.Frames[NextFrame].Tick == Tick)
        {
            Frame = &Replay.Frames[NextFrame];
            ++NextFrame;
        }

        World.Tick(Frame);
        World.ClearEvents();

        while (NextCheckpoint < Replay.Checkpoints.size() &&
               Replay.Checkpoints[NextCheckpoint].Tick == World.GetTick())
        {
            const uint64_t Actual = World.ComputeStateChecksum();
            if (Actual != Replay.Checkpoints[NextCheckpoint].Checksum)
            {
                Result.bSucceeded = false;
                Result.DivergedAtTick = World.GetTick();
                Result.ExpectedChecksum = Replay.Checkpoints[NextCheckpoint].Checksum;
                Result.ActualChecksum = Actual;
                Result.TicksPlayed = World.GetTick();
                Result.Error = "state diverged from the recorded checkpoint";
                return Result;
            }
            ++NextCheckpoint;
        }
    }

    Result.bSucceeded = true;
    Result.TicksPlayed = World.GetTick();
    Result.Winner = World.GetWinner();
    return Result;
}

} // namespace RA4
