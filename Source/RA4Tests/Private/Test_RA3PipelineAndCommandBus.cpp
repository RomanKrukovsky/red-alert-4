// Copyright (c) Red Alert 4 project. Unit tests for ArmorMatrix and CommandBus.
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Combat/ArmorMatrix.h"
#include "RA4Simulation/CommandBus.h"
#include "RA4Simulation/SimWorld.h"

using namespace RA4;
using namespace RA4Test;

namespace
{
struct CommandBusFixture
{
    ContentDatabase Content;
    SimWorld World;

    CommandBusFixture()
    {
        BuildDefaultContent(Content);
        World.Initialize(&Content, MakeTestSetup());
        SpawnEnemyOutpost(World);
    }
};
} // namespace

RA4_TEST(ArmorMatrix, DamageCalculation)
{
    const RA4::ArmorMatrix& Matrix = RA4::GetDefaultArmorMatrix();

    // Ballistic vs LightInfantry (100% -> 100 dmg)
    int32_t Dmg1 = Matrix.CalculateDamage(100, RA4::WarheadClass::Ballistic, RA4::ArmorClass::LightInfantry);
    RA4_EXPECT_EQ(Dmg1, 100);

    // Ballistic vs HeavyVehicle (25% -> 25 dmg)
    int32_t Dmg2 = Matrix.CalculateDamage(100, RA4::WarheadClass::Ballistic, RA4::ArmorClass::HeavyVehicle);
    RA4_EXPECT_EQ(Dmg2, 25);

    // ArmorPiercing vs HeavyVehicle (100% -> 100 dmg)
    int32_t Dmg3 = Matrix.CalculateDamage(100, RA4::WarheadClass::ArmorPiercing, RA4::ArmorClass::HeavyVehicle);
    RA4_EXPECT_EQ(Dmg3, 100);

    // Siege vs LightInfantry (200% -> 200 dmg)
    int32_t Dmg4 = Matrix.CalculateDamage(100, RA4::WarheadClass::Siege, RA4::ArmorClass::LightInfantry);
    RA4_EXPECT_EQ(Dmg4, 200);
}

RA4_TEST(CommandBus, QueueAndDispatch)
{
    RA4::CommandBus Bus;

    RA4::Command Cmd1;
    Cmd1.Type = RA4::CommandType::Move;
    Cmd1.Issuer = 0;
    Cmd1.Location = RA4::Vec2(RA4::Fixed::FromInt(500), RA4::Fixed::FromInt(500));

    Bus.EnqueueCommand(1, Cmd1);
    RA4_EXPECT_EQ(Bus.GetPendingCommandCount(), 1);

    RA4::CommandFrame Frame1 = Bus.FetchFrameForTick(1);
    RA4_EXPECT_EQ(Frame1.Commands.size(), 1);
    RA4_EXPECT_EQ(Frame1.Tick, 1);

    Bus.ClearUpToTick(1);
    RA4_EXPECT_EQ(Bus.GetPendingCommandCount(), 0);
}

RA4_TEST(CommandBus, DispatchTickAppliesAProductionFrameExactlyOnce)
{
    CommandBusFixture F;
    CommandBus Bus;

    const EntityId Yard = F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    RA4_REQUIRE(Yard.IsValid());

    const int32_t BeforeCredits = F.World.GetPlayer(0).Credits;

    Command Start = MakeCommand(CommandType::StartProduction, 0);
    Start.Primary = Yard;
    Start.Content = Ids::SovPower;
    Bus.EnqueueCommand(F.World.GetTick(), Start);

    const int32_t Accepted = Bus.DispatchTick(F.World.GetTick(), F.World);
    const BuildingComp* YardState = F.World.GetBuilding(Yard);
    RA4_REQUIRE(YardState != nullptr);

    RA4_EXPECT_EQ(Accepted, 1);
    // The frame was applied once, so exactly one item is queued at the full price.
    // DispatchTick runs a whole simulation tick, so ADR-0012 draws exactly one
    // funding slice -- ceil(800/160) = 5 -- and no more. A second application would
    // show up as a doubled slice.
    RA4_EXPECT_EQ(int32_t(YardState->Queue.size()), 1);
    RA4_EXPECT_EQ(YardState->Queue.front().TotalCost, 800);
    RA4_EXPECT_EQ(YardState->Queue.front().PaidCredits, 5);
    RA4_EXPECT_EQ(F.World.GetPlayer(0).Credits, BeforeCredits - 5);
}

RA4_TEST(CommandBus, DispatchTickReturnsZeroForRejectedCommands)
{
    CommandBusFixture F;
    CommandBus Bus;

    Command Start = MakeCommand(CommandType::StartProduction, 0);
    Start.Content = Ids::SovPower;
    Bus.EnqueueCommand(F.World.GetTick(), Start);

    const int32_t Accepted = Bus.DispatchTick(F.World.GetTick(), F.World);

    RA4_EXPECT_EQ(Accepted, 0);
    RA4_EXPECT_EQ(F.World.GetPlayer(0).Credits, 10000);
}

RA4_TEST(CommandBus, DispatchTickReturnsZeroWhenMatchIsAlreadyOver)
{
    CommandBusFixture F;
    CommandBus Bus;

    const EntityId Yard = F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    RA4_REQUIRE(Yard.IsValid());

    Command Surrender = MakeCommand(CommandType::Surrender, 0);
    RA4_REQUIRE(F.World.ApplyCommand(Surrender).IsAccepted());
    RunTicks(F.World, 1);
    RA4_REQUIRE(F.World.GetPhase() != MatchPhase::Running);

    Command Start = MakeCommand(CommandType::StartProduction, 0);
    Start.Primary = Yard;
    Start.Content = Ids::SovPower;
    Bus.EnqueueCommand(F.World.GetTick(), Start);

    Command Move = MakeCommand(CommandType::Move, 0);
    Move.Primary = EntityId(999, 1);
    Move.Location = Vec2::FromInts(1000, 1000);
    Bus.EnqueueCommand(F.World.GetTick(), Move);

    const int32_t Accepted = Bus.DispatchTick(F.World.GetTick(), F.World);
    const BuildingComp* YardState = F.World.GetBuilding(Yard);
    RA4_REQUIRE(YardState != nullptr);

    RA4_EXPECT_EQ(Accepted, 0);
    RA4_EXPECT_EQ(F.World.GetPlayer(0).Credits, 10000);
    RA4_EXPECT_EQ(int32_t(YardState->Queue.size()), 0);

    int32_t MatchOverRejects = 0;
    for (const SimEvent& Event : F.World.GetEvents())
    {
        if (Event.Type == SimEventType::CommandRejected &&
            Event.Value == int32_t(CommandReject::MatchOver))
        {
            ++MatchOverRejects;
        }
    }
    RA4_EXPECT_EQ(MatchOverRejects, 2);
}

RA4_TEST(CommandBus, DispatchTickConsumesItsBufferedFrame)
{
    CommandBusFixture F;
    CommandBus Bus;

    const EntityId Yard = F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    RA4_REQUIRE(Yard.IsValid());

    const int32_t BeforeCredits = F.World.GetPlayer(0).Credits;
    const TickIndex DispatchTick = F.World.GetTick();

    Command Start = MakeCommand(CommandType::StartProduction, 0);
    Start.Primary = Yard;
    Start.Content = Ids::SovPower;
    Bus.EnqueueCommand(DispatchTick, Start);

    const int32_t FirstAccepted = Bus.DispatchTick(DispatchTick, F.World);
    const int32_t CreditsAfterFirst = F.World.GetPlayer(0).Credits;
    const BuildingComp* YardAfterFirst = F.World.GetBuilding(Yard);
    RA4_REQUIRE(YardAfterFirst != nullptr);

    const int32_t SecondAccepted = Bus.DispatchTick(DispatchTick, F.World);
    const BuildingComp* YardAfterSecond = F.World.GetBuilding(Yard);
    RA4_REQUIRE(YardAfterSecond != nullptr);

    RA4_EXPECT_EQ(FirstAccepted, 1);
    RA4_EXPECT_EQ(SecondAccepted, 0);
    // The first dispatch runs one tick, so exactly one ADR-0012 funding slice is
    // drawn: ceil(800/160) = 5.
    RA4_EXPECT_EQ(CreditsAfterFirst, BeforeCredits - 5);
    // The second dispatch finds the frame already consumed and returns without
    // ticking, so credits must not move again. This is the real double-application
    // check: a re-applied frame would draw another slice or queue a second item.
    RA4_EXPECT_EQ(F.World.GetPlayer(0).Credits, CreditsAfterFirst);
    RA4_EXPECT_EQ(int32_t(YardAfterFirst->Queue.size()), 1);
    RA4_EXPECT_EQ(int32_t(YardAfterSecond->Queue.size()), 1);
}
