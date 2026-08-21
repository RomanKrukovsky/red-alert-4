// Copyright (c) Red Alert 4 project. Tests for Stage 3 (Rollback & GGPO-like Netcode).
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Core/Command.h"
#include "RA4Core/Fixed.h"
#include "RA4Core/Vector.h"
#include "RA4Simulation/RollbackSession.h"
#include "RA4Simulation/SimWorld.h"

#include <memory>
#include <vector>

using namespace RA4;
using namespace RA4Test;
using RA4::Net::RollbackSession;
using RA4::Net::RollbackMode;
using RA4::Net::RollbackEvent;

namespace
{

std::unique_ptr<ContentDatabase> MakeRollbackTestContent()
{
    auto Db = std::make_unique<ContentDatabase>();

    // ConYard to prevent match defeat
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

// --- 1. Immediate Local Input Responsiveness ---

RA4_TEST(Rollback, ImmediateLocalInputExecution)
{
    RollbackSession Session;
    Session.Initialize(/*LocalPlayer*/ 0, /*NumPlayers*/ 2, /*bIsAuthority*/ true,
                       /*LocalInputDelay*/ 0, /*MaxPredictionTicks*/ 10);

    RA4_EXPECT_EQ(Session.GetLocalInputDelay(), 0u);

    Command MoveCmd = MakeCommand(CommandType::Move, 0);
    MoveCmd.Location = Vec2(Fixed::FromInt(1500), Fixed::FromInt(1500));

    const TickIndex ScheduledTick = Session.SubmitLocalCommand(5, MoveCmd);
    RA4_EXPECT_EQ(ScheduledTick, 5u);

    const CommandFrame Outgoing = Session.TakeOutgoingFrame(5);
    RA4_EXPECT_EQ(Outgoing.Tick, 5u);
    RA4_REQUIRE(Outgoing.Commands.size() == 1);
    RA4_EXPECT(Outgoing.Commands[0].Location.X == Fixed::FromInt(1500));
}

// --- 2. Speculative Execution Under Network Lag ---

RA4_TEST(Rollback, SpeculativeTickingUnderNetworkLag)
{
    auto Content = MakeRollbackTestContent();
    MatchSetup Setup = MakeTestSetup();

    SimWorld World;
    World.Initialize(Content.get(), Setup);

    const ContentId ConYard = MakeContentId("building.test_conyard");
    World.SpawnBuilding(ConYard, 0, TileCoord(2, 2), true);
    World.SpawnBuilding(ConYard, 1, TileCoord(30, 30), true);

    RollbackSession Session;
    Session.Initialize(0, 2, false, /*LocalDelay*/ 0, /*MaxPredictionTicks*/ 4,
                       RollbackMode::SpeculativeRollback);

    // Initial state: tick 0 is ready
    RA4_EXPECT(Session.CanAdvance(0));

    // Advance 4 speculative ticks (ticks 0, 1, 2, 3) without receiving player 1's packets
    for (int I = 0; I < 4; ++I)
    {
        RA4_EXPECT(Session.AdvanceSimulation(World));
    }
    RA4_EXPECT_EQ(World.GetTick(), 4u);

    // The 5th speculative tick is at limit (4 prediction ticks ahead of confirmed tick 0)
    RA4_EXPECT(Session.AdvanceSimulation(World));
    RA4_EXPECT_EQ(World.GetTick(), 5u);

    // Further advancement beyond MaxPredictionTicks should be prevented to stop unbounded divergence
    RA4_EXPECT(!Session.CanAdvance(World.GetTick()));
    RA4_EXPECT(!Session.AdvanceSimulation(World));
    RA4_EXPECT_EQ(World.GetTick(), 5u);
}

// --- 3. Late Packet Arrival Triggers Rollback and Resimulation ---

RA4_TEST(Rollback, LatePacketArrivalTriggersRollbackAndResim)
{
    auto Content = MakeRollbackTestContent();
    MatchSetup Setup = MakeTestSetup();

    SimWorld World;
    World.Initialize(Content.get(), Setup);

    const ContentId ConYard = MakeContentId("building.test_conyard");
    World.SpawnBuilding(ConYard, 0, TileCoord(2, 2), true);
    World.SpawnBuilding(ConYard, 1, TileCoord(30, 30), true);

    const ContentId Scout = MakeContentId("unit.test_scout");
    const EntityId TankP1 = World.SpawnUnit(Scout, 1, Vec2(Fixed::FromInt(2000), Fixed::FromInt(2000)));

    RollbackSession Session;
    Session.Initialize(0, 2, false, /*LocalDelay*/ 0, /*MaxPredictionTicks*/ 10,
                       RollbackMode::SpeculativeRollback);

    // Simulate forward 5 ticks predicting that Player 1 is idle
    for (int I = 0; I < 5; ++I)
    {
        Session.AdvanceSimulation(World);
    }
    RA4_EXPECT_EQ(World.GetTick(), 5u);

    const Vec2 PositionBeforeRollback = World.GetTransform(TankP1)->Position;
    RA4_EXPECT(PositionBeforeRollback.X == Fixed::FromInt(2000));
    RA4_EXPECT(PositionBeforeRollback.Y == Fixed::FromInt(2000));

    // Now a delayed packet arrives from Player 1 containing a Move order issued at Tick = 1
    CommandFrame LateFrame;
    LateFrame.Tick = 1;
    Command MoveCmd = MakeCommand(CommandType::Move, 1);
    MoveCmd.Primary = TankP1;
    MoveCmd.Location = Vec2(Fixed::FromInt(3000), Fixed::FromInt(2000));
    LateFrame.Commands.push_back(MoveCmd);

    const RollbackEvent Event = Session.ReceiveRemoteFrame(1, LateFrame, &World);

    // Assert that rollback occurred cleanly
    RA4_EXPECT(Event.bOccurred);
    RA4_EXPECT_EQ(Event.RolledBackToTick, 1u);
    RA4_EXPECT_EQ(Event.ResimulatedToTick, 5u);
    RA4_EXPECT_EQ(Event.ResimulatedTickCount, 4u);
    RA4_EXPECT_EQ(Event.CausingPlayer, 1);

    // Assert that the simulation current tick is still 5
    RA4_EXPECT_EQ(World.GetTick(), 5u);

    // Assert that the resimulation executed the late Move order from Tick 1 to Tick 5,
    // so TankP1 has moved forward along X!
    const Vec2 PositionAfterRollback = World.GetTransform(TankP1)->Position;
    RA4_EXPECT(PositionAfterRollback.X > Fixed::FromInt(2000));
}

// --- 4. Bit-Exact Equivalence Between Lockstep & Rollback Resimulation ---

RA4_TEST(Rollback, BitExactEquivalenceWithPerfectLockstep)
{
    auto Content = MakeRollbackTestContent();
    MatchSetup Setup = MakeTestSetup();

    // World A: Reference simulation running with zero jitter / perfect lockstep
    SimWorld WorldA;
    WorldA.Initialize(Content.get(), Setup);
    const ContentId ConYard = MakeContentId("building.test_conyard");
    WorldA.SpawnBuilding(ConYard, 0, TileCoord(2, 2), true);
    WorldA.SpawnBuilding(ConYard, 1, TileCoord(30, 30), true);

    const ContentId Scout = MakeContentId("unit.test_scout");
    const EntityId TankA0 = WorldA.SpawnUnit(Scout, 0, Vec2(Fixed::FromInt(1000), Fixed::FromInt(1000)));
    const EntityId TankA1 = WorldA.SpawnUnit(Scout, 1, Vec2(Fixed::FromInt(2000), Fixed::FromInt(2000)));

    // World B: Rollback simulation running with speculative ticking & delayed packets
    SimWorld WorldB;
    WorldB.Initialize(Content.get(), Setup);
    WorldB.SpawnBuilding(ConYard, 0, TileCoord(2, 2), true);
    WorldB.SpawnBuilding(ConYard, 1, TileCoord(30, 30), true);
    const EntityId TankB0 = WorldB.SpawnUnit(Scout, 0, Vec2(Fixed::FromInt(1000), Fixed::FromInt(1000)));
    const EntityId TankB1 = WorldB.SpawnUnit(Scout, 1, Vec2(Fixed::FromInt(2000), Fixed::FromInt(2000)));

    // 1. Run World A for 10 ticks with orders given at Tick 1 and Tick 3
    Command Cmd0 = MakeCommand(CommandType::Move, 0);
    Cmd0.Primary = TankA0;
    Cmd0.Location = Vec2(Fixed::FromInt(2500), Fixed::FromInt(1000));

    Command Cmd1 = MakeCommand(CommandType::Move, 1);
    Cmd1.Primary = TankA1;
    Cmd1.Location = Vec2(Fixed::FromInt(2000), Fixed::FromInt(3500));

    for (TickIndex T = 0; T < 10; ++T)
    {
        CommandFrame Frame;
        Frame.Tick = T;
        if (T == 1) { Frame.Commands.push_back(Cmd0); }
        if (T == 3) { Frame.Commands.push_back(Cmd1); }
        WorldA.Tick(&Frame);
    }
    const uint64_t ReferenceChecksum = WorldA.ComputeStateChecksum();

    // 2. Run World B with RollbackSession:
    // - Player 0 submits Cmd0 at Tick 1
    // - Runs speculatively ahead to Tick 6 (predicting Player 1 is empty)
    // - Delayed packet with Cmd1 at Tick 3 arrives while at Tick 6 -> Rollback occurs!
    // - Advances to Tick 10
    RollbackSession SessionB;
    SessionB.Initialize(0, 2, false, /*LocalDelay*/ 0, /*MaxPredictionTicks*/ 10,
                        RollbackMode::SpeculativeRollback);

    Command CmdB0 = MakeCommand(CommandType::Move, 0);
    CmdB0.Primary = TankB0;
    CmdB0.Location = Vec2(Fixed::FromInt(2500), Fixed::FromInt(1000));

    Command CmdB1 = MakeCommand(CommandType::Move, 1);
    CmdB1.Primary = TankB1;
    CmdB1.Location = Vec2(Fixed::FromInt(2000), Fixed::FromInt(3500));

    // Ticks 0..5
    for (TickIndex T = 0; T < 6; ++T)
    {
        if (T == 1)
        {
            SessionB.SubmitLocalCommand(1, CmdB0);
        }
        SessionB.AdvanceSimulation(WorldB);
    }

    // At Tick 6, delayed packet for Tick 3 arrives
    CommandFrame DelayedFrame;
    DelayedFrame.Tick = 3;
    DelayedFrame.Commands.push_back(CmdB1);
    const RollbackEvent RollbackEv = SessionB.ReceiveRemoteFrame(1, DelayedFrame, &WorldB);
    RA4_EXPECT(RollbackEv.bOccurred);

    // Continue ticking to Tick 10
    for (TickIndex T = 6; T < 10; ++T)
    {
        SessionB.AdvanceSimulation(WorldB);
    }

    const uint64_t RollbackChecksum = WorldB.ComputeStateChecksum();

    // Assert that the rolled-back simulation reached the EXACT 100% bit-identical state!
    RA4_EXPECT_EQ(RollbackChecksum, ReferenceChecksum);
}

// --- 5. Adaptive Latency and Jitter Buffer Tuning ---

RA4_TEST(Rollback, AdaptiveJitterDelayTuning)
{
    RollbackSession Session;
    Session.Initialize(0, 2, true);

    // Low latency LAN: 20ms RTT, 2ms Jitter
    Session.UpdateNetworkLatency(1, 20, 2);
    RA4_EXPECT_EQ(Session.GetStats().RecommendedInputDelay, 1u);

    // High latency WAN with jitter: 150ms RTT, 25ms Jitter
    // One-way latency: 75ms + 50ms = 125ms -> ~3 ticks delay
    Session.UpdateNetworkLatency(1, 150, 25);
    RA4_EXPECT_EQ(Session.GetStats().RecommendedInputDelay, 3u);

    // Severe spike: 300ms RTT, 50ms Jitter -> clamped to max 5 ticks
    Session.UpdateNetworkLatency(1, 300, 50);
    RA4_EXPECT_EQ(Session.GetStats().RecommendedInputDelay, 5u);
}
