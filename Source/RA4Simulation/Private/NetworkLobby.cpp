// Copyright (c) Red Alert 4 project. Network lobby state machine and UDP wire transport adapter implementation.
#include "RA4Simulation/NetworkLobby.h"

#include <algorithm>
#include <cstring>
#include <set>

namespace RA4
{

NetworkLobby::NetworkLobby()
{
    Slots.resize(kMaxPlayers);
    for (uint8_t I = 0; I < kMaxPlayers; ++I)
    {
        Slots[I].SlotIndex = I;
        Slots[I].Status = (I == 0 ? LobbySlotStatus::Human : (I == 1 ? LobbySlotStatus::Open : LobbySlotStatus::Closed));
        Slots[I].Team = static_cast<uint8_t>((I % 2) + 1);
    }
}

void NetworkLobby::InitializeRoom(const LobbyRoomSettings& InSettings, const std::string& HostName, FactionId HostFaction)
{
    Settings = InSettings;
    Settings.MaxPlayers = std::clamp<uint8_t>(Settings.MaxPlayers, 2, kMaxPlayers);
    bMatchStarted = false;
    HostSlot = 0;

    Slots.clear();
    Slots.resize(Settings.MaxPlayers);

    for (uint8_t I = 0; I < Settings.MaxPlayers; ++I)
    {
        Slots[I].SlotIndex = I;
        Slots[I].Team = static_cast<uint8_t>((I % 2) + 1);

        if (I == 0)
        {
            Slots[I].Status = LobbySlotStatus::Human;
            Slots[I].PlayerName = HostName;
            Slots[I].Faction = HostFaction;
            Slots[I].bReady = true;
        }
        else if (I == 1)
        {
            Slots[I].Status = LobbySlotStatus::Open;
            Slots[I].PlayerName = "";
            Slots[I].Faction = FactionId::Alliance;
            Slots[I].bReady = false;
        }
        else
        {
            Slots[I].Status = LobbySlotStatus::Closed;
            Slots[I].PlayerName = "";
            Slots[I].Faction = FactionId::Soviet;
            Slots[I].bReady = false;
        }
    }
}

bool NetworkLobby::SetSlotStatus(uint8_t SlotIndex, LobbySlotStatus Status)
{
    if (SlotIndex >= Slots.size()) return false;
    if (SlotIndex == HostSlot && Status != LobbySlotStatus::Human) return false;

    Slots[SlotIndex].Status = Status;
    if (Status == LobbySlotStatus::Open || Status == LobbySlotStatus::Closed)
    {
        Slots[SlotIndex].PlayerName.clear();
        Slots[SlotIndex].bReady = false;
    }
    else if (Status == LobbySlotStatus::AI)
    {
        Slots[SlotIndex].PlayerName = "AI Commander " + std::to_string(SlotIndex);
        Slots[SlotIndex].bReady = true;
    }

    return true;
}

bool NetworkLobby::ConfigureSlot(uint8_t SlotIndex, const std::string& PlayerName, FactionId Faction, uint8_t Team, uint32_t ColorRGBA)
{
    if (SlotIndex >= Slots.size()) return false;
    if (Slots[SlotIndex].Status == LobbySlotStatus::Closed || Slots[SlotIndex].Status == LobbySlotStatus::Open) return false;

    Slots[SlotIndex].PlayerName = PlayerName;
    Slots[SlotIndex].Faction = Faction;
    Slots[SlotIndex].Team = Team;
    Slots[SlotIndex].ColorRGBA = ColorRGBA;
    return true;
}

bool NetworkLobby::SetSlotReady(uint8_t SlotIndex, bool bReady)
{
    if (SlotIndex >= Slots.size()) return false;
    if (Slots[SlotIndex].Status != LobbySlotStatus::Human) return false;

    Slots[SlotIndex].bReady = bReady;
    return true;
}

bool NetworkLobby::ValidateMatchStart(std::string* OutReason) const
{
    if (bMatchStarted)
    {
        if (OutReason) *OutReason = "Match already started";
        return false;
    }

    uint32_t ActiveCount = 0;
    std::set<uint8_t> ActiveTeams;

    for (size_t I = 0; I < Slots.size(); ++I)
    {
        const auto& Slot = Slots[I];
        if (Slot.Status == LobbySlotStatus::Human || Slot.Status == LobbySlotStatus::AI)
        {
            ++ActiveCount;
            ActiveTeams.insert(Slot.Team);

            if (Slot.Status == LobbySlotStatus::Human && !Slot.bReady)
            {
                if (OutReason) *OutReason = "Player in slot " + std::to_string(I) + " is not ready";
                return false;
            }
        }
    }

    if (ActiveCount < 2)
    {
        if (OutReason) *OutReason = "Need at least 2 active players to start";
        return false;
    }

    if (ActiveTeams.size() < 2)
    {
        if (OutReason) *OutReason = "All players are on the same team";
        return false;
    }

    return true;
}

bool NetworkLobby::StartMatch()
{
    if (!ValidateMatchStart(nullptr)) return false;
    bMatchStarted = true;
    return true;
}

MatchSetup NetworkLobby::GenerateMatchSetup() const
{
    MatchSetup Setup;
    Setup.Seed = Settings.RandomSeed;
    Setup.Map.Width = 64;
    Setup.Map.Height = 64;
    Setup.Map.Name = Settings.MapName;
    Setup.Map.Resize(64, 64, Tile_GroundPassable);

    for (uint8_t I = 0; I < kMaxPlayers; ++I)
    {
        if (I < Slots.size() && (Slots[I].Status == LobbySlotStatus::Human || Slots[I].Status == LobbySlotStatus::AI))
        {
            Setup.Players[I].bActive = true;
            Setup.Players[I].Team = Slots[I].Team;
            Setup.Players[I].Faction = Slots[I].Faction;
            Setup.Players[I].StartingCredits = static_cast<int32_t>(Settings.StartingCredits);
            Setup.Players[I].StartPositionIndex = I;
        }
        else
        {
            Setup.Players[I].bActive = false;
        }
    }

    return Setup;
}

// --- Wire Serialization ---

std::vector<uint8_t> NetWirePacketSerializer::SerializePacket(
    const NetWirePacketHeader& Header,
    const uint8_t* PayloadData,
    size_t PayloadSize)
{
    constexpr size_t kHeaderSize = sizeof(NetWirePacketHeader);
    std::vector<uint8_t> Buffer(kHeaderSize + PayloadSize);

    NetWirePacketHeader PackedHeader = Header;
    PackedHeader.PayloadSize = static_cast<uint32_t>(PayloadSize);

    std::memcpy(Buffer.data(), &PackedHeader, kHeaderSize);
    if (PayloadData != nullptr && PayloadSize > 0)
    {
        std::memcpy(Buffer.data() + kHeaderSize, PayloadData, PayloadSize);
    }

    return Buffer;
}

bool NetWirePacketSerializer::DeserializePacket(
    const uint8_t* Buffer,
    size_t BufferSize,
    NetWirePacketHeader& OutHeader,
    std::vector<uint8_t>& OutPayload)
{
    constexpr size_t kHeaderSize = sizeof(NetWirePacketHeader);
    if (Buffer == nullptr || BufferSize < kHeaderSize)
    {
        return false;
    }

    std::memcpy(&OutHeader, Buffer, kHeaderSize);

    if (OutHeader.Magic != 0x5241344E)
    {
        return false;
    }

    if (BufferSize < kHeaderSize + OutHeader.PayloadSize)
    {
        return false;
    }

    OutPayload.resize(OutHeader.PayloadSize);
    if (OutHeader.PayloadSize > 0)
    {
        std::memcpy(OutPayload.data(), Buffer + kHeaderSize, OutHeader.PayloadSize);
    }

    return true;
}

} // namespace RA4
