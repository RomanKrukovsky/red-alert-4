// Copyright (c) Red Alert 4 project.
// Vertical Proving Ground: 500+ entity stress scenario, deterministic replay & desync testing.

#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Content/ContentDatabase.h"
#include "RA4Simulation/SimWorld.h"

using namespace RA4;
using namespace RA4Test;

namespace RA4
{

RA4_TEST(ProvingGround, HeadlessStressScenario500Entities)
{
    ContentDatabase Db;
    BuildDefaultContent(Db);

    SimWorld World;
    World.Initialize(&Db, MakeTestSetup(1337));

    // Spawn 250 units for Player 0 and 250 for Player 1 (Total 500 entities)
    std::vector<EntityId> HandlesP0;
    std::vector<EntityId> HandlesP1;

    for (int i = 0; i < 250; ++i)
    {
        Vec2 PosP0 = Vec2::FromInts(1000 + (i % 25) * 10, 1000 + (i / 25) * 10);
        EntityId H0 = World.SpawnUnit(Ids::SovConscript, 0, PosP0);
        HandlesP0.push_back(H0);

        Vec2 PosP1 = Vec2::FromInts(5000 - (i % 25) * 10, 5000 - (i / 25) * 10);
        EntityId H1 = World.SpawnUnit(Ids::AllRifleman, 1, PosP1);
        HandlesP1.push_back(H1);
    }

    RA4_EXPECT_EQ(HandlesP0.size() + HandlesP1.size(), 500);

    // Issue move commands for units
    Command MoveCmd0 = MakeCommand(CommandType::Move, 0);
    MoveCmd0.Primary = HandlesP0[0];
    MoveCmd0.Location = Vec2::FromInts(3000, 3000);

    Command MoveCmd1 = MakeCommand(CommandType::Move, 1);
    MoveCmd1.Primary = HandlesP1[0];
    MoveCmd1.Location = Vec2::FromInts(2000, 2000);

    CommandFrame Frame;
    Frame.Tick = 1;
    Frame.Commands.push_back(MoveCmd0);
    Frame.Commands.push_back(MoveCmd1);

    World.Tick(&Frame);

    // Run simulation for 200 ticks (10 seconds)
    RunTicks(World, 200);

    uint64_t HashRun1 = World.ComputeStateChecksum();
    RA4_EXPECT(HashRun1 != 0);

    // Run identical second simulation to verify deterministic hash
    SimWorld World2;
    World2.Initialize(&Db, MakeTestSetup(1337));

    std::vector<EntityId> Handles2P0;
    std::vector<EntityId> Handles2P1;

    for (int i = 0; i < 250; ++i)
    {
        Vec2 PosP0 = Vec2::FromInts(1000 + (i % 25) * 10, 1000 + (i / 25) * 10);
        Handles2P0.push_back(World2.SpawnUnit(Ids::SovConscript, 0, PosP0));

        Vec2 PosP1 = Vec2::FromInts(5000 - (i % 25) * 10, 5000 - (i / 25) * 10);
        Handles2P1.push_back(World2.SpawnUnit(Ids::AllRifleman, 1, PosP1));
    }

    MoveCmd0.Primary = Handles2P0[0];
    MoveCmd1.Primary = Handles2P1[0];

    CommandFrame Frame2;
    Frame2.Tick = 1;
    Frame2.Commands.push_back(MoveCmd0);
    Frame2.Commands.push_back(MoveCmd1);

    World2.Tick(&Frame2);

    RunTicks(World2, 200);

    uint64_t HashRun2 = World2.ComputeStateChecksum();
    RA4_EXPECT_EQ(HashRun1, HashRun2);
}

RA4_TEST(ProvingGround, ForcedDesyncDetection)
{
    ContentDatabase Db;
    BuildDefaultContent(Db);

    SimWorld MainWorld;
    MainWorld.Initialize(&Db, MakeTestSetup(42));

    SimWorld DesyncWorld;
    DesyncWorld.Initialize(&Db, MakeTestSetup(42));

    MainWorld.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(1000, 1000));
    DesyncWorld.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(1000, 1000));

    // Run 50 ticks in sync
    for (int t = 0; t < 50; ++t)
    {
        MainWorld.Tick(nullptr);
        DesyncWorld.Tick(nullptr);
        RA4_EXPECT_EQ(MainWorld.ComputeStateChecksum(), DesyncWorld.ComputeStateChecksum());
    }

    // Force desync mutation on frame 51
    DesyncWorld.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(2000, 2000));

    MainWorld.Tick(nullptr);
    DesyncWorld.Tick(nullptr);

    // Verify desync detection
    RA4_EXPECT(MainWorld.ComputeStateChecksum() != DesyncWorld.ComputeStateChecksum());
}

RA4_TEST(ProvingGround, HeadlessStressScenario1000Entities)
{
    ContentDatabase Db;
    BuildDefaultContent(Db);

    SimWorld World;
    World.Initialize(&Db, MakeTestSetup(4096));

    for (int i = 0; i < 500; ++i)
    {
        Vec2 PosP0 = Vec2::FromInts(1000 + (i % 50) * 8, 1000 + (i / 50) * 8);
        World.SpawnUnit(Ids::SovConscript, 0, PosP0);

        Vec2 PosP1 = Vec2::FromInts(5000 - (i % 50) * 8, 5000 - (i / 50) * 8);
        World.SpawnUnit(Ids::AllRifleman, 1, PosP1);
    }

    RA4_EXPECT_EQ(World.GetEntityCapacity(), 1000);

    // Run simulation for 100 ticks
    RunTicks(World, 100);

    uint64_t StateHash = World.ComputeStateChecksum();
    RA4_EXPECT(StateHash != 0);
}

RA4_TEST(ProvingGround, HeadlessStressScenario2000Entities)
{
    ContentDatabase Db;
    BuildDefaultContent(Db);

    SimWorld World;
    World.Initialize(&Db, MakeTestSetup(8192));

    for (int i = 0; i < 1000; ++i)
    {
        Vec2 PosP0 = Vec2::FromInts(1000 + (i % 50) * 8, 1000 + (i / 50) * 8);
        World.SpawnUnit(Ids::SovConscript, 0, PosP0);

        Vec2 PosP1 = Vec2::FromInts(5000 - (i % 50) * 8, 5000 - (i / 50) * 8);
        World.SpawnUnit(Ids::AllRifleman, 1, PosP1);
    }

    RA4_EXPECT_EQ(World.GetEntityCapacity(), 2000);

    // Run simulation for 100 ticks
    RunTicks(World, 100);

    uint64_t StateHash = World.ComputeStateChecksum();
    RA4_EXPECT(StateHash != 0);
}

} // namespace RA4
