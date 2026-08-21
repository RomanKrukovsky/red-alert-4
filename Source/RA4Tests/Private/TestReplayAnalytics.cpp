// Copyright (c) Red Alert 4 project. Tests for Stage 6 (Replay Scrubbing, Playback & Match Analytics).
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Core/Command.h"
#include "RA4Core/Fixed.h"
#include "RA4Core/Vector.h"
#include "RA4Replay/Replay.h"
#include "RA4Replay/ReplayPlaybackEngine.h"
#include "RA4Replay/MatchTelemetryTracker.h"
#include "RA4Simulation/SimWorld.h"

#include <memory>
#include <vector>

using namespace RA4;
using namespace RA4Test;

namespace
{

std::unique_ptr<ContentDatabase> MakeReplayTestContent()
{
    auto Db = std::make_unique<ContentDatabase>();

    // ConYard
    {
        EntityDef E;
        E.Id = MakeContentId("building.test_conyard");
        E.Name = "building.test_conyard";
        E.Kind = EntityKind::Building;
        E.MaxHealth = 2000;
        E.Armor = ArmorClass::Building;
        E.Building.FootprintX = 2;
        E.Building.FootprintY = 2;
        E.Building.bIsConstructionYard = true;
        E.Building.bProvidesBuildRadius = true;
        E.Building.BuildRadius = Fixed::FromInt(2000);
        Db->AddEntity(E);
    }

    // Fast Scout Tank
    {
        EntityDef E;
        E.Id = MakeContentId("unit.test_scout");
        E.Name = "unit.test_scout";
        E.Kind = EntityKind::Unit;
        E.MaxHealth = 300;
        E.Armor = ArmorClass::LightVehicle;
        E.Production.Cost = 500;
        E.Unit.Layer = MovementLayer::Tracked;
        E.Unit.MaxSpeed = Fixed::FromInt(200);
        E.Unit.Acceleration = Fixed::FromInt(400);
        E.Unit.TurnRatePerSecond = 2048;
        E.Unit.CollisionRadius = Fixed::FromInt(20);
        Db->AddEntity(E);
    }

    return Db;
}

} // namespace

// --- 1. Replay Scrubbing & Bidirectional Seeking ---

RA4_TEST(ReplayScrubbing, ForwardAndBackwardSeeking)
{
    auto Content = MakeReplayTestContent();
    MatchSetup Setup = MakeTestSetup(42);

    ReplayRecorder Recorder;
    ReplayHeader Header = MakeHeaderFromSetup(Setup, *Content, "1.0.0");
    Recorder.Begin(Header);

    // Reference simulation: run 100 ticks and record
    SimWorld WorldRef;
    WorldRef.Initialize(Content.get(), Setup);

    const ContentId ConYard = MakeContentId("building.test_conyard");
    WorldRef.SpawnBuilding(ConYard, 0, TileCoord(2, 2), true);
    WorldRef.SpawnBuilding(ConYard, 1, TileCoord(30, 30), true);

    const ContentId Scout = MakeContentId("unit.test_scout");
    const EntityId Tank0 = WorldRef.SpawnUnit(Scout, 0, Vec2(Fixed::FromInt(1000), Fixed::FromInt(1000)));

    Command MoveCmd = MakeCommand(CommandType::Move, 0);
    MoveCmd.Primary = Tank0;
    MoveCmd.Location = Vec2(Fixed::FromInt(3000), Fixed::FromInt(3000));

    std::map<TickIndex, uint64_t> CheckpointMap;
    const SimSnapshot InitialSnap = WorldRef.CaptureSnapshot();
    CheckpointMap[0] = WorldRef.ComputeStateChecksum();


    for (TickIndex T = 0; T < 100; ++T)
    {
        CommandFrame Frame;
        Frame.Tick = T;
        if (T == 10)
        {
            Frame.Commands.push_back(MoveCmd);
            Recorder.RecordFrame(Frame);
        }
        WorldRef.Tick(&Frame);
        CheckpointMap[WorldRef.GetTick()] = WorldRef.ComputeStateChecksum();
    }
    Recorder.End(100, kInvalidPlayer);

    ReplayData Data;
    std::string Err;
    RA4_REQUIRE(DeserializeReplay(Recorder.Serialize(), Data, Err));

    // Load into ReplayPlaybackEngine with keyframes every 25 ticks
    ReplayPlaybackEngine Engine;
    RA4_REQUIRE(Engine.LoadWithInitialSnapshot(Data, *Content, InitialSnap, /*KeyframeInterval*/ 25));
    RA4_EXPECT_EQ(Engine.GetTotalTicks(), 100u);

    // 1. Seek forward to tick 80
    RA4_REQUIRE(Engine.SeekToTick(80));
    RA4_EXPECT_EQ(Engine.GetCurrentTick(), 80u);
    const uint64_t ChecksumAt80_First = Engine.GetWorld()->ComputeStateChecksum();
    RA4_EXPECT_EQ(ChecksumAt80_First, CheckpointMap[80]);

    // 2. Seek backward to tick 30 (requires snapshot rollback + fast-forward from tick 25)
    RA4_REQUIRE(Engine.SeekToTick(30));
    RA4_EXPECT_EQ(Engine.GetCurrentTick(), 30u);
    const uint64_t ChecksumAt30 = Engine.GetWorld()->ComputeStateChecksum();
    RA4_EXPECT_EQ(ChecksumAt30, CheckpointMap[30]);

    // 3. Seek forward again to tick 80
    RA4_REQUIRE(Engine.SeekToTick(80));
    RA4_EXPECT_EQ(Engine.GetCurrentTick(), 80u);
    const uint64_t ChecksumAt80_Second = Engine.GetWorld()->ComputeStateChecksum();
    RA4_EXPECT_EQ(ChecksumAt80_Second, ChecksumAt80_First);
}

// --- 2. Playback Speed Multipliers ---

RA4_TEST(ReplayScrubbing, SpeedMultipliers)
{
    auto Content = MakeReplayTestContent();
    MatchSetup Setup = MakeTestSetup(42);

    SimWorld WorldRef;
    WorldRef.Initialize(Content.get(), Setup);
    const ContentId ConYard = MakeContentId("building.test_conyard");
    WorldRef.SpawnBuilding(ConYard, 0, TileCoord(2, 2), true);
    WorldRef.SpawnBuilding(ConYard, 1, TileCoord(30, 30), true);
    const SimSnapshot InitialSnap = WorldRef.CaptureSnapshot();

    ReplayRecorder Recorder;
    ReplayHeader Header = MakeHeaderFromSetup(Setup, *Content, "1.0.0");
    Recorder.Begin(Header);
    Recorder.End(200, kInvalidPlayer);

    ReplayData Data;
    std::string Err;
    DeserializeReplay(Recorder.Serialize(), Data, Err);

    ReplayPlaybackEngine Engine;
    Engine.LoadWithInitialSnapshot(Data, *Content, InitialSnap, 50);

    Engine.Play();
    RA4_EXPECT_EQ(static_cast<uint8_t>(Engine.GetState()), static_cast<uint8_t>(PlaybackState::Playing));

    // 1x speed: 0.1s real time = 2 ticks @ 20Hz
    Engine.SetPlaybackSpeed(1.0f);
    uint32_t Ticks = Engine.Step(0.10f);
    RA4_EXPECT_EQ(Ticks, 2u);

    // 4x speed: 0.1s real time = 8 ticks
    Engine.SetPlaybackSpeed(4.0f);
    Ticks = Engine.Step(0.10f);
    RA4_EXPECT_EQ(Ticks, 8u);
}


// --- 3. APM Tracking ---

RA4_TEST(Analytics, APMTracking)
{
    MatchTelemetryTracker Tracker;
    Tracker.Initialize(/*NumPlayers*/ 2, /*SampleInterval*/ 20);

    // Feed burst of 10 commands on tick 20
    CommandFrame Frame;
    Frame.Tick = 20;
    for (int I = 0; I < 10; ++I)
    {
        Command C = MakeCommand(CommandType::Move, 0);
        Frame.Commands.push_back(C);
    }

    Tracker.IngestTickCommands(20, &Frame);

    const auto* Stats0 = Tracker.GetPlayerStats(0);
    RA4_REQUIRE(Stats0 != nullptr);
    RA4_EXPECT_EQ(Stats0->TotalCommandsIssued, 10);
    RA4_EXPECT(Stats0->PeakAPM > 100.0f); // 10 commands in 1s = 600 APM rate
}

// --- 4. Timeline Sampling and JSON Export ---

RA4_TEST(Analytics, TimelineSamplingAndJsonExport)
{
    auto Content = MakeReplayTestContent();
    MatchSetup Setup = MakeTestSetup(42);

    SimWorld World;
    World.Initialize(Content.get(), Setup);

    const ContentId ConYard = MakeContentId("building.test_conyard");
    World.SpawnBuilding(ConYard, 0, TileCoord(2, 2), true);
    World.SpawnBuilding(ConYard, 1, TileCoord(30, 30), true);

    const ContentId Scout = MakeContentId("unit.test_scout");
    World.SpawnUnit(Scout, 0, Vec2(Fixed::FromInt(1000), Fixed::FromInt(1000)));

    MatchTelemetryTracker Tracker;
    Tracker.Initialize(2, /*SampleInterval*/ 20);

    for (TickIndex T = 0; T <= 40; ++T)
    {
        World.Tick(nullptr);
        Tracker.SampleWorldState(World);
    }

    const auto* Stats0 = Tracker.GetPlayerStats(0);
    RA4_REQUIRE(Stats0 != nullptr);
    RA4_EXPECT(Stats0->Timeline.size() >= 2u);
    RA4_EXPECT(Stats0->PeakArmyValue >= 500);

    const std::string Json = Tracker.ExportToJson(World);
    RA4_EXPECT(!Json.empty());
    RA4_EXPECT(Json.find("\"totalTicks\"") != std::string::npos);
    RA4_EXPECT(Json.find("\"players\"") != std::string::npos);
    RA4_EXPECT(Json.find("\"timeline\"") != std::string::npos);
}
