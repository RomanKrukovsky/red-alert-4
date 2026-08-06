// Copyright (c) Red Alert 4 project. Unit tests for the damage table and CommandBus.
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Content/ContentDatabase.h"
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

// The damage table the game actually consults, pinned against the balance document.
//
// This replaces a test that asserted the values of RA4Combat::ArmorMatrix -- a second,
// unreachable copy of the table that no module depended on. That test passed while
// disagreeing with the shipped numbers on most entries (Ballistic vs HeavyVehicle: live
// 50, the copy 25), so a green "ArmorMatrix.DamageCalculation" implied the balance was
// covered when nothing covered it. The copy has been deleted; this guards the real path.
//
// Spot values are quoted from RA4Content/DamageMatrix.h, itself taken from Section 2 of
// the units bible. They are asserted through ContentDatabase because that is what
// SimWorld::ApplyDamage calls -- testing DamageMatrix directly would skip the copy step
// that actually feeds combat.
RA4_TEST(DamageTable, LiveMultipliersMatchTheBalanceDocument)
{
    RA4::ContentDatabase Content;
    Content.ResetDamageTableToDefaults();

    auto Mult = [&Content](RA4::WarheadClass W, RA4::ArmorClass A)
    {
        return Content.GetDamageMultiplier(W, A);
    };

    // Small arms: full effect on unarmoured infantry, poor against armour and structures.
    RA4_EXPECT_EQ(Mult(RA4::WarheadClass::Ballistic, RA4::ArmorClass::LightInfantry), 100);
    RA4_EXPECT_EQ(Mult(RA4::WarheadClass::Ballistic, RA4::ArmorClass::HeavyVehicle), 50);
    RA4_EXPECT_EQ(Mult(RA4::WarheadClass::Ballistic, RA4::ArmorClass::Building), 30);

    // Fragmentation is the anti-infantry answer and is deliberately poor against armour.
    RA4_EXPECT_EQ(Mult(RA4::WarheadClass::Fragmentation, RA4::ArmorClass::LightInfantry), 150);
    RA4_EXPECT_EQ(Mult(RA4::WarheadClass::Fragmentation, RA4::ArmorClass::HeavyVehicle), 50);

    // Armour-piercing inverts that: strong into vehicles, wasteful on infantry.
    RA4_EXPECT_EQ(Mult(RA4::WarheadClass::ArmorPiercing, RA4::ArmorClass::LightInfantry), 50);
    RA4_EXPECT_EQ(Mult(RA4::WarheadClass::ArmorPiercing, RA4::ArmorClass::HeavyVehicle), 150);

    // Siege exists to level bases, and must stay unable to hit aircraft.
    RA4_EXPECT_EQ(Mult(RA4::WarheadClass::Siege, RA4::ArmorClass::Building), 200);
    RA4_EXPECT_EQ(Mult(RA4::WarheadClass::Siege, RA4::ArmorClass::Air), 10);

    // AntiAir is the sharpest asymmetry in the table: near-useless on the ground, and
    // literally zero against buildings, so a flak battery cannot chip a structure down.
    RA4_EXPECT_EQ(Mult(RA4::WarheadClass::AntiAir, RA4::ArmorClass::Air), 200);
    RA4_EXPECT_EQ(Mult(RA4::WarheadClass::AntiAir, RA4::ArmorClass::Building), 0);

    // Out-of-range indices must degrade to neutral rather than read past the array: a
    // content bug should not become an out-of-bounds read in the damage path.
    RA4_EXPECT_EQ(Mult(static_cast<RA4::WarheadClass>(RA4::WarheadClass::Count),
                       RA4::ArmorClass::LightInfantry), 100);
    RA4_EXPECT_EQ(Mult(RA4::WarheadClass::Ballistic,
                       static_cast<RA4::ArmorClass>(RA4::ArmorClass::Count)), 100);
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
    RA4_EXPECT_EQ(F.World.GetPlayer(0).Credits, BeforeCredits - 800);
    RA4_EXPECT_EQ(int32_t(YardState->Queue.size()), 1);
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
    RA4_EXPECT_EQ(CreditsAfterFirst, BeforeCredits - 800);
    RA4_EXPECT_EQ(F.World.GetPlayer(0).Credits, CreditsAfterFirst);
    RA4_EXPECT_EQ(int32_t(YardAfterFirst->Queue.size()), 1);
    RA4_EXPECT_EQ(int32_t(YardAfterSecond->Queue.size()), 1);
}
