// Copyright (c) Red Alert 4 project. Network lobby state machine and UDP wire transport adapter.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "RA4Content/ContentTypes.h"
#include "RA4Core/Command.h"
#include "RA4Core/Ids.h"
#include "RA4Simulation/SimTypes.h"
#include "RA4Simulation/SimWorld.h"


#ifndef RA4SIMULATION_API
#define RA4SIMULATION_API
#endif

namespace RA4
{

enum class LobbySlotStatus : uint8_t
{
    Closed = 0,
    Open,
    Human,
    AI,
};

struct LobbySlot
{
    uint8_t SlotIndex = 0;
    LobbySlotStatus Status = LobbySlotStatus::Open;
    std::string PlayerName;
    FactionId Faction = FactionId::Soviet;
    uint8_t Team = 1;
    uint32_t ColorRGBA = 0xFF0000FF; // Default Red
    bool bReady = false;
    uint32_t PingMs = 0;
};

struct LobbyRoomSettings
{
    std::string RoomName = "Scarlet Skirmish";
    std::string MapName = "maps/coastal_fury";
    uint8_t MaxPlayers = 8;
    uint32_t StartingCredits = 10000;
    uint32_t RandomSeed = 1337;
    bool bSuperweaponsEnabled = true;
    bool bFogOfWarEnabled = true;
};

class RA4SIMULATION_API NetworkLobby
{
public:
    NetworkLobby();

    void InitializeRoom(const LobbyRoomSettings& Settings, const std::string& HostName, FactionId HostFaction);

    bool SetSlotStatus(uint8_t SlotIndex, LobbySlotStatus Status);
    bool ConfigureSlot(uint8_t SlotIndex, const std::string& PlayerName, FactionId Faction, uint8_t Team, uint32_t ColorRGBA);
    bool SetSlotReady(uint8_t SlotIndex, bool bReady);

    /** Validates whether the match can start. */
    bool ValidateMatchStart(std::string* OutReason = nullptr) const;

    /** Generates authoritative MatchSetup for SimWorld initialization. */
    MatchSetup GenerateMatchSetup() const;

    const LobbyRoomSettings& GetSettings() const { return Settings; }
    const std::vector<LobbySlot>& GetSlots() const { return Slots; }
    uint8_t GetHostSlot() const { return HostSlot; }
    bool IsMatchStarted() const { return bMatchStarted; }

    bool StartMatch();

private:
    LobbyRoomSettings Settings;
    std::vector<LobbySlot> Slots;
    uint8_t HostSlot = 0;
    bool bMatchStarted = false;
};

// --- Network Transport Packet Wire Protocol ---

enum class NetWirePacketKind : uint8_t
{
    HeartbeatPing = 0,
    HeartbeatPong,
    CommandFrameSync,
    RollbackSnapshotChunk,
    DesyncReport,
};

struct NetWirePacketHeader
{
    uint32_t Magic = 0x5241344E; // "RA4N"
    NetWirePacketKind Kind = NetWirePacketKind::CommandFrameSync;
    uint8_t SenderPlayer = 0;
    uint16_t SequenceNumber = 0;
    uint16_t AckNumber = 0;
    uint32_t PayloadSize = 0;
};

class RA4SIMULATION_API NetWirePacketSerializer
{
public:
    static std::vector<uint8_t> SerializePacket(
        const NetWirePacketHeader& Header,
        const uint8_t* PayloadData,
        size_t PayloadSize);

    static bool DeserializePacket(
        const uint8_t* Buffer,
        size_t BufferSize,
        NetWirePacketHeader& OutHeader,
        std::vector<uint8_t>& OutPayload);
};

} // namespace RA4
