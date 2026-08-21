// Copyright (c) Red Alert 4 project. Tests for Stage 12 (Multiplayer Network Lobby & Wire Transport Protocol).
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Content/ContentTypes.h"
#include "RA4Core/Ids.h"
#include "RA4Simulation/NetworkLobby.h"
#include "RA4Simulation/SimWorld.h"

#include <string>
#include <vector>

using namespace RA4;
using namespace RA4Test;

// --- 1. Lobby Slot Configuration & Ready State Tracking ---

RA4_TEST(NetworkLobby, RoomInitializationAndSlotConfiguration)
{
    NetworkLobby Lobby;

    LobbyRoomSettings Settings;
    Settings.RoomName = "Tournament Final";
    Settings.MapName = "maps/red_square";
    Settings.MaxPlayers = 4;
    Settings.StartingCredits = 15000;
    Settings.RandomSeed = 777;

    Lobby.InitializeRoom(Settings, "CommanderRed", FactionId::Soviet);

    RA4_EXPECT_EQ(Lobby.GetSlots().size(), 4u);
    RA4_EXPECT_EQ(static_cast<uint8_t>(Lobby.GetSlots()[0].Status), static_cast<uint8_t>(LobbySlotStatus::Human));
    RA4_EXPECT(Lobby.GetSlots()[0].PlayerName == "CommanderRed");
    RA4_EXPECT_EQ(static_cast<uint8_t>(Lobby.GetSlots()[0].Faction), static_cast<uint8_t>(FactionId::Soviet));
    RA4_EXPECT_EQ(Lobby.GetSlots()[0].bReady, true);

    // Add Human in Slot 1
    Lobby.SetSlotStatus(1, LobbySlotStatus::Human);
    Lobby.ConfigureSlot(1, "CommanderBlue", FactionId::Alliance, 2, 0x0000FFFF);

    // Add AI in Slot 2
    Lobby.SetSlotStatus(2, LobbySlotStatus::AI);
    Lobby.ConfigureSlot(2, "AI Bot", FactionId::EasternCoalition, 1, 0x00FF00FF);

    // Close Slot 3
    Lobby.SetSlotStatus(3, LobbySlotStatus::Closed);

    RA4_EXPECT_EQ(static_cast<uint8_t>(Lobby.GetSlots()[1].Status), static_cast<uint8_t>(LobbySlotStatus::Human));
    RA4_EXPECT_EQ(static_cast<uint8_t>(Lobby.GetSlots()[2].Status), static_cast<uint8_t>(LobbySlotStatus::AI));
    RA4_EXPECT_EQ(static_cast<uint8_t>(Lobby.GetSlots()[3].Status), static_cast<uint8_t>(LobbySlotStatus::Closed));
}

// --- 2. Match Start Validation & MatchSetup Export ---

RA4_TEST(NetworkLobby, ValidationAndMatchStartGeneration)
{
    NetworkLobby Lobby;

    LobbyRoomSettings Settings;
    Settings.MaxPlayers = 4;
    Lobby.InitializeRoom(Settings, "Player1", FactionId::Soviet);
    Lobby.SetSlotStatus(1, LobbySlotStatus::Human);
    Lobby.ConfigureSlot(1, "Player2", FactionId::Alliance, 2, 0x0000FFFF);

    // Slot 1 is not ready yet -> validation must fail
    std::string Reason;
    RA4_EXPECT_EQ(Lobby.ValidateMatchStart(&Reason), false);
    RA4_EXPECT(!Reason.empty());

    // Make Slot 1 ready
    Lobby.SetSlotReady(1, true);
    RA4_EXPECT_EQ(Lobby.ValidateMatchStart(&Reason), true);

    // Put both players on Team 1 -> validation must fail (no opposing team)
    Lobby.ConfigureSlot(1, "Player2", FactionId::Alliance, 1, 0x0000FFFF);
    RA4_EXPECT_EQ(Lobby.ValidateMatchStart(&Reason), false);

    // Restore Team 2 and start match
    Lobby.ConfigureSlot(1, "Player2", FactionId::Alliance, 2, 0x0000FFFF);
    RA4_EXPECT_EQ(Lobby.StartMatch(), true);
    RA4_EXPECT_EQ(Lobby.IsMatchStarted(), true);

    // Generate MatchSetup
    const MatchSetup Setup = Lobby.GenerateMatchSetup();
    RA4_EXPECT_EQ(Setup.Seed, Settings.RandomSeed);
    RA4_EXPECT(Setup.Map.Name == Settings.MapName);
    RA4_EXPECT_EQ(Setup.Players[0].bActive, true);
    RA4_EXPECT_EQ(Setup.Players[0].Team, 1u);
    RA4_EXPECT_EQ(static_cast<uint8_t>(Setup.Players[0].Faction), static_cast<uint8_t>(FactionId::Soviet));
    RA4_EXPECT_EQ(Setup.Players[1].bActive, true);
    RA4_EXPECT_EQ(Setup.Players[1].Team, 2u);
    RA4_EXPECT_EQ(static_cast<uint8_t>(Setup.Players[1].Faction), static_cast<uint8_t>(FactionId::Alliance));
    RA4_EXPECT_EQ(Setup.Players[2].bActive, false);
}

// --- 3. Wire Packet Serialization & Resilience ---

RA4_TEST(NetworkTransport, WirePacketSerializationAndDeserialization)
{
    NetWirePacketHeader Header;
    Header.Kind = NetWirePacketKind::CommandFrameSync;
    Header.SenderPlayer = 1;
    Header.SequenceNumber = 1042;
    Header.AckNumber = 1040;

    const std::vector<uint8_t> SamplePayload = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03, 0x04};

    // Serialize
    const std::vector<uint8_t> WireBuffer = NetWirePacketSerializer::SerializePacket(
        Header, SamplePayload.data(), SamplePayload.size());

    RA4_EXPECT(WireBuffer.size() == sizeof(NetWirePacketHeader) + SamplePayload.size());

    // Deserialize
    NetWirePacketHeader OutHeader;
    std::vector<uint8_t> OutPayload;

    const bool bSuccess = NetWirePacketSerializer::DeserializePacket(
        WireBuffer.data(), WireBuffer.size(), OutHeader, OutPayload);

    RA4_EXPECT_EQ(bSuccess, true);
    RA4_EXPECT_EQ(OutHeader.Magic, 0x5241344Eu);
    RA4_EXPECT_EQ(static_cast<uint8_t>(OutHeader.Kind), static_cast<uint8_t>(NetWirePacketKind::CommandFrameSync));
    RA4_EXPECT_EQ(OutHeader.SenderPlayer, 1u);
    RA4_EXPECT_EQ(OutHeader.SequenceNumber, 1042u);
    RA4_EXPECT_EQ(OutHeader.AckNumber, 1040u);
    RA4_EXPECT_EQ(OutHeader.PayloadSize, static_cast<uint32_t>(SamplePayload.size()));
    RA4_EXPECT(OutPayload == SamplePayload);

    // Truncated buffer test

    std::vector<uint8_t> TruncatedPayload;
    NetWirePacketHeader TruncatedHeader;
    const bool bTruncatedResult = NetWirePacketSerializer::DeserializePacket(
        WireBuffer.data(), sizeof(NetWirePacketHeader) - 1, TruncatedHeader, TruncatedPayload);
    RA4_EXPECT_EQ(bTruncatedResult, false);

    // Corrupted magic test
    std::vector<uint8_t> CorruptedBuffer = WireBuffer;
    CorruptedBuffer[0] = 0x00;
    const bool bCorruptedResult = NetWirePacketSerializer::DeserializePacket(
        CorruptedBuffer.data(), CorruptedBuffer.size(), OutHeader, OutPayload);
    RA4_EXPECT_EQ(bCorruptedResult, false);
}
