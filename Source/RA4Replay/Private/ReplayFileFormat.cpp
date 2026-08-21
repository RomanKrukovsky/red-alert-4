// Copyright (c) Red Alert 4 project. Official .ra4rep Binary Replay Format & Desync Verifier.
#include "RA4Replay/ReplayFileFormat.h"

namespace RA4
{

void ReplayContainer::Serialize(ByteWriter& W) const
{
    W.WriteUInt32(Magic);
    W.WriteUInt32(Version);
    W.WriteString(MapName);
    W.WriteUInt64(InitialSeed);
    W.WriteUInt32(MatchDurationTicks);
    W.WriteUInt32(PlayerCount);

    W.WriteUInt32(uint32_t(Checkpoints.size()));
    for (const auto& C : Checkpoints)
    {
        W.WriteUInt32(C.Tick);
        W.WriteUInt64(C.StateChecksum);
    }

    W.WriteUInt32(uint32_t(Commands.size()));
    for (const auto& Cmd : Commands)
    {
        W.WriteUInt32(Cmd.Tick);
        Cmd.Cmd.Serialize(W);
    }
}

bool ReplayContainer::Deserialize(ByteReader& R)
{
    Magic = R.ReadUInt32();
    if (Magic != kReplayMagic) return false;

    Version = R.ReadUInt32();
    if (Version != kReplayFormatVersion) return false;

    MapName = R.ReadString();
    InitialSeed = R.ReadUInt64();
    MatchDurationTicks = R.ReadUInt32();
    PlayerCount = R.ReadUInt32();

    const uint32_t NumCheckpoints = R.ReadUInt32();
    Checkpoints.resize(NumCheckpoints);
    for (uint32_t I = 0; I < NumCheckpoints; ++I)
    {
        Checkpoints[I].Tick = R.ReadUInt32();
        Checkpoints[I].StateChecksum = R.ReadUInt64();
    }

    const uint32_t NumCommands = R.ReadUInt32();
    Commands.resize(NumCommands);
    for (uint32_t I = 0; I < NumCommands; ++I)
    {
        Commands[I].Tick = R.ReadUInt32();
        Commands[I].Cmd = Command::Deserialize(R);
    }

    return !R.HasError();
}

bool ReplayIntegrityVerifier::VerifyBitExactIntegrity(const ReplayContainer& Replay, SimWorld& World)
{
    size_t CmdIdx = 0;
    size_t ChkIdx = 0;

    for (TickIndex T = 0; T < Replay.MatchDurationTicks; ++T)
    {
        while (CmdIdx < Replay.Commands.size() && Replay.Commands[CmdIdx].Tick == T)
        {
            World.ApplyCommand(Replay.Commands[CmdIdx].Cmd);
            ++CmdIdx;
        }

        World.Tick(nullptr);

        while (ChkIdx < Replay.Checkpoints.size() && Replay.Checkpoints[ChkIdx].Tick == (T + 1))
        {
            const uint64_t CurrentChecksum = World.ComputeStateChecksum();
            if (CurrentChecksum != Replay.Checkpoints[ChkIdx].StateChecksum)
            {
                return false; // Desync detected!
            }
            ++ChkIdx;
        }
    }

    return true;
}

} // namespace RA4
