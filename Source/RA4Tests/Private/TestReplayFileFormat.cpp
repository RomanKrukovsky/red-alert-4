// Copyright (c) Red Alert 4 project. Tests for Stage 20 (Official .ra4rep Replay Container).
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Content/ContentDatabase.h"
#include "RA4Core/ByteStream.h"
#include "RA4Core/Command.h"
#include "RA4Core/Fixed.h"
#include "RA4Replay/ReplayFileFormat.h"
#include "RA4Simulation/SimWorld.h"

#include <memory>

using namespace RA4;
using namespace RA4Test;

namespace
{

std::unique_ptr<ContentDatabase> MakeReplayContentDb()
{
    auto Db = std::make_unique<ContentDatabase>();
    Db->ResetDamageTableToDefaults();

    // ConYard
    {
        EntityDef E;
        E.Id = MakeContentId("building.sov.conyard");
        E.Name = "building.sov.conyard";
        E.Kind = EntityKind::Building;
        E.MaxHealth = 2000;
        E.Armor = ArmorClass::Building;
        E.Building.FootprintX = 2;
        E.Building.FootprintY = 2;
        E.Building.bIsConstructionYard = true;
        E.Building.PowerProduced = 100;
        Db->AddEntity(E);
    }

    // Heavy Tank
    {
        EntityDef E;
        E.Id = MakeContentId("unit.sov.heavy_tank");
        E.Name = "unit.sov.heavy_tank";
        E.Kind = EntityKind::Unit;
        E.MaxHealth = 1000;
        E.Armor = ArmorClass::HeavyVehicle;
        E.Production.Cost = 1200;
        E.Unit.Layer = MovementLayer::Tracked;
        E.Unit.MaxSpeed = Fixed::FromInt(150);
        E.Unit.Acceleration = Fixed::FromInt(300);
        E.Unit.TurnRatePerSecond = 1024;
        E.Unit.CollisionRadius = Fixed::FromInt(25);
        Db->AddEntity(E);
    }

    return Db;
}

} // namespace

// --- 1. Binary Container Roundtrip ---

RA4_TEST(ReplayFileFormat, BinarySerializationAndDeserializationRoundtrip)
{
    ReplayContainer OutReplay;
    OutReplay.Magic = kReplayMagic;
    OutReplay.Version = kReplayFormatVersion;
    OutReplay.MapName = "Map_Heidelberg_Crossing";
    OutReplay.InitialSeed = 133742;
    OutReplay.MatchDurationTicks = 1200;
    OutReplay.PlayerCount = 4;

    // Add Checkpoints
    OutReplay.Checkpoints.push_back({100, 0xABCDEF0123456789ULL});
    OutReplay.Checkpoints.push_back({200, 0x1122334455667788ULL});

    // Add Commands
    {
        ReplayTimedCommand TC;
        TC.Tick = 5;
        TC.Cmd.Type = CommandType::Move;
        TC.Cmd.Issuer = 0;
        TC.Cmd.Location = Vec2(Fixed::FromInt(500), Fixed::FromInt(700));
        OutReplay.Commands.push_back(TC);
    }
    {
        ReplayTimedCommand TC;
        TC.Tick = 12;
        TC.Cmd.Type = CommandType::Stop;
        TC.Cmd.Issuer = 1;
        OutReplay.Commands.push_back(TC);
    }

    ByteWriter Writer;
    OutReplay.Serialize(Writer);

    ByteReader Reader(Writer.GetBuffer());
    ReplayContainer InReplay;
    const bool bLoaded = InReplay.Deserialize(Reader);

    RA4_EXPECT_EQ(bLoaded, true);
    RA4_EXPECT_EQ(InReplay.Magic, kReplayMagic);
    RA4_EXPECT_EQ(InReplay.Version, kReplayFormatVersion);
    RA4_EXPECT(InReplay.MapName == "Map_Heidelberg_Crossing");
    RA4_EXPECT_EQ(InReplay.InitialSeed, uint64_t(133742));
    RA4_EXPECT_EQ(InReplay.MatchDurationTicks, uint32_t(1200));
    RA4_EXPECT_EQ(InReplay.PlayerCount, uint32_t(4));

    RA4_EXPECT_EQ(InReplay.Checkpoints.size(), size_t(2));
    RA4_EXPECT_EQ(InReplay.Checkpoints[0].Tick, uint32_t(100));
    RA4_EXPECT_EQ(InReplay.Checkpoints[0].StateChecksum, 0xABCDEF0123456789ULL);

    RA4_EXPECT_EQ(InReplay.Commands.size(), size_t(2));
    RA4_EXPECT_EQ(InReplay.Commands[0].Tick, uint32_t(5));
    RA4_EXPECT_EQ(uint8_t(InReplay.Commands[0].Cmd.Type), uint8_t(CommandType::Move));
    RA4_EXPECT(InReplay.Commands[0].Cmd.Location.X == Fixed::FromInt(500));
}

// --- 2. Bit-Exact Integrity & Desync Detection ---

RA4_TEST(ReplayFileFormat, PlaybackDeterminismAndIntegrityVerification)
{
    auto Db = MakeReplayContentDb();
    MatchSetup Setup = MakeTestSetup(901);

    SimWorld Sim;
    Sim.Initialize(Db.get(), Setup);

    const ContentId ConYard = MakeContentId("building.sov.conyard");
    Sim.SpawnBuilding(ConYard, 0, TileCoord(5, 5), true);
    Sim.SpawnBuilding(ConYard, 1, TileCoord(50, 50), true);

    const ContentId TankDef = MakeContentId("unit.sov.heavy_tank");
    const EntityId Tank = Sim.SpawnUnit(TankDef, 0, Vec2(Fixed::FromInt(500), Fixed::FromInt(500)));

    ReplayContainer Replay;
    Replay.MatchDurationTicks = 10;

    // Record order
    {
        ReplayTimedCommand TC;
        TC.Tick = 2;
        TC.Cmd.Type = CommandType::Move;
        TC.Cmd.Issuer = 0;
        TC.Cmd.Primary = Tank;
        TC.Cmd.Location = Vec2(Fixed::FromInt(1000), Fixed::FromInt(1000));
        Replay.Commands.push_back(TC);
    }

    // Run authoritative sim to capture ground-truth checkpoints
    for (TickIndex T = 0; T < 10; ++T)
    {
        if (T == 2)
        {
            Sim.ApplyCommand(Replay.Commands[0].Cmd);
        }
        Sim.Tick(nullptr);

        if ((T + 1) % 2 == 0)
        {
            Replay.Checkpoints.push_back({T + 1, Sim.ComputeStateChecksum()});
        }
    }

    // Playback verification against fresh SimWorld
    SimWorld PlaybackSim;
    PlaybackSim.Initialize(Db.get(), Setup);
    PlaybackSim.SpawnBuilding(ConYard, 0, TileCoord(5, 5), true);
    PlaybackSim.SpawnBuilding(ConYard, 1, TileCoord(50, 50), true);
    PlaybackSim.SpawnUnit(TankDef, 0, Vec2(Fixed::FromInt(500), Fixed::FromInt(500)));

    const bool bVerified = ReplayIntegrityVerifier::VerifyBitExactIntegrity(Replay, PlaybackSim);
    RA4_EXPECT_EQ(bVerified, true);

    // Corrupt one checkpoint hash -> verify desync is caught
    Replay.Checkpoints[1].StateChecksum ^= 0xFFFFFFFFULL;

    SimWorld DesyncSim;
    DesyncSim.Initialize(Db.get(), Setup);
    DesyncSim.SpawnBuilding(ConYard, 0, TileCoord(5, 5), true);
    DesyncSim.SpawnBuilding(ConYard, 1, TileCoord(50, 50), true);
    DesyncSim.SpawnUnit(TankDef, 0, Vec2(Fixed::FromInt(500), Fixed::FromInt(500)));

    const bool bDesyncCaught = !ReplayIntegrityVerifier::VerifyBitExactIntegrity(Replay, DesyncSim);
    RA4_EXPECT_EQ(bDesyncCaught, true);
}
