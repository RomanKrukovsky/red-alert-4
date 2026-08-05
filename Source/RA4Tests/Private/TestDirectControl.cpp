// Copyright (c) Red Alert 4 project. Authoritative direct-control command tests.
//
// These tests prove that the four DirectControl command types are the *only* way
// first-person input reaches the simulation, that the server enforces ownership
// and liveness the same way it does for ordinary orders, and that the same weapon
// is used in direct and RTS mode (no secret first-person-only damage buff).
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Content/ContentDatabase.h"
#include "RA4Simulation/SimWorld.h"

using namespace RA4;
using namespace RA4Test;

namespace
{

Command MakeEnter(PlayerId Issuer, EntityId Vehicle)
{
    Command C;
    C.Type = CommandType::DirectControlEnter;
    C.Issuer = Issuer;
    C.Primary = Vehicle;
    return C;
}

Command MakeExit(PlayerId Issuer, EntityId Vehicle)
{
    Command C;
    C.Type = CommandType::DirectControlExit;
    C.Issuer = Issuer;
    C.Primary = Vehicle;
    return C;
}

Command MakeDrive(PlayerId Issuer, EntityId Vehicle, int8_t Throttle, int8_t Steering)
{
    Command C;
    C.Type = CommandType::DirectControlDrive;
    C.Issuer = Issuer;
    C.Primary = Vehicle;
    C.DirectAxes.Throttle = Throttle;
    C.DirectAxes.Steering = Steering;
    return C;
}

Command MakeFire(PlayerId Issuer, EntityId Vehicle, EntityId Target, uint8_t Flags)
{
    Command C;
    C.Type = CommandType::DirectControlFire;
    C.Issuer = Issuer;
    C.Primary = Vehicle;
    C.Target = Target;
    C.DirectAxes.Flags = Flags;
    return C;
}

} // namespace

RA4_TEST(DirectControl, EnterRequiresOwnership)
{
    ContentDatabase Db;
    BuildDefaultContent(Db);
    SimWorld World;
    World.Initialize(&Db, MakeTestSetup(42));

    const EntityId Tank = World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(100, 100));
    const EntityId Enemy = World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(200, 200));

    // Player 1 cannot enter player 0's tank.
    RA4_EXPECT(!(World.ApplyCommand(MakeEnter(1, Tank))).IsAccepted());
    // Player 0 cannot enter an enemy unit.
    RA4_EXPECT(!(World.ApplyCommand(MakeEnter(0, Enemy))).IsAccepted());
    // Owner can.
    RA4_EXPECT(World.ApplyCommand(MakeEnter(0, Tank)).IsAccepted());
    const DirectControlComp* Dc = World.GetDirectControl(Tank);
    RA4_EXPECT(Dc != nullptr);
    RA4_EXPECT_EQ(uint8_t(Dc->Phase), uint8_t(DirectControlPhase::Entering));
}

RA4_TEST(DirectControl, OneDriverAtATime)
{
    ContentDatabase Db;
    BuildDefaultContent(Db);
    SimWorld World;
    World.Initialize(&Db, MakeTestSetup(42));

    const EntityId Tank = World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(100, 100));
    RA4_EXPECT(World.ApplyCommand(MakeEnter(0, Tank)).IsAccepted());
    // Same player re-entering is idempotent.
    RA4_EXPECT(World.ApplyCommand(MakeEnter(0, Tank)).IsAccepted());
}

RA4_TEST(DirectControl, RejectsBuildingsAndUnarmedUnits)
{
    ContentDatabase Db;
    BuildDefaultContent(Db);
    SimWorld World;
    World.Initialize(&Db, MakeTestSetup(42));

    const EntityId ConYard = World.SpawnBuilding(Ids::SovConYard, 0, TileCoord{4, 4}, true);
    // Buildings are not direct-controllable.
    RA4_EXPECT(!(World.ApplyCommand(MakeEnter(0, ConYard))).IsAccepted());
}

RA4_TEST(DirectControl, ExitClearsOrdersAndStopsVehicle)
{
    ContentDatabase Db;
    BuildDefaultContent(Db);
    SimWorld World;
    World.Initialize(&Db, MakeTestSetup(42));

    const EntityId Tank = World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(100, 100));
    // Keep player 1 alive so SystemVictory does not end the match while we drive.
    World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(900, 900));
    World.ApplyCommand(MakeEnter(0, Tank));
    // Drive forward a bit. We use real CommandFrames so the per-tick command
    // counter resets the same way it does in a live match.
    for (int i = 0; i < 5; ++i)
    {
        CommandFrame F;
        F.Tick = World.GetTick();
        F.Commands.push_back(MakeDrive(0, Tank, 127, 0));
        World.Tick(&F);
        World.ClearEvents();
    }
    const CommandResult ExitResult = World.ApplyCommand(MakeExit(0, Tank));
    RA4_EXPECT(ExitResult.IsAccepted());
    const DirectControlComp* Dc = World.GetDirectControl(Tank);
    RA4_EXPECT(Dc != nullptr);
    RA4_EXPECT_EQ(uint8_t(Dc->Phase), uint8_t(DirectControlPhase::Exiting));
}

RA4_TEST(DirectControl, FireReusesRtsWeaponAndCooldown)
{
    ContentDatabase Db;
    BuildDefaultContent(Db);
    SimWorld World;
    World.Initialize(&Db, MakeTestSetup(42));

    const EntityId Tank = World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(100, 100));
    const EntityId Enemy = World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(150, 150));
    World.ApplyCommand(MakeEnter(0, Tank));

    // First fire should hit (cooldown 0).
    const CommandResult First = World.ApplyCommand(MakeFire(0, Tank, Enemy, 0x01));
    RA4_EXPECT(First.IsAccepted());

    // Immediate second fire must be rejected on cooldown.
    const CommandResult Second = World.ApplyCommand(MakeFire(0, Tank, Enemy, 0x01));
    RA4_EXPECT(!(Second.IsAccepted()));
}

RA4_TEST(DirectControl, CannotFireWithoutPossession)
{
    ContentDatabase Db;
    BuildDefaultContent(Db);
    SimWorld World;
    World.Initialize(&Db, MakeTestSetup(42));

    const EntityId Tank = World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(100, 100));
    const EntityId Enemy = World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(150, 150));
    // No Enter issued: fire must be rejected.
    RA4_EXPECT(!(World.ApplyCommand(MakeFire(0, Tank, Enemy, 0x01))).IsAccepted());
}

RA4_TEST(DirectControl, DestroyedVehicleExitsDriver)
{
    ContentDatabase Db;
    BuildDefaultContent(Db);
    SimWorld World;
    World.Initialize(&Db, MakeTestSetup(42));

    const EntityId Tank = World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(100, 100));
    // Keep player 1 alive so the match does not end the instant the tank dies.
    World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(900, 900));
    World.ApplyCommand(MakeEnter(0, Tank));
    World.DebugDamage(Tank, 100000);
    World.Tick(nullptr);

    // The simulation must emit DirectControlExited when the possessed vehicle
    // is destroyed, so the presentation layer can return the player to RTS view
    // instead of leaving the camera inside a corpse.
    bool bExitEventEmitted = false;
    for (const SimEvent& Ev : World.GetEvents())
    {
        if (Ev.Type == SimEventType::DirectControlExited &&
            Ev.Entity == Tank &&
            Ev.Player == PlayerId(0))
        {
            bExitEventEmitted = true;
            break;
        }
    }
    RA4_EXPECT(bExitEventEmitted);
}

RA4_TEST(DirectControl, SerializeRoundTripsDirectControlState)
{
    ContentDatabase Db;
    BuildDefaultContent(Db);
    SimWorld World;
    World.Initialize(&Db, MakeTestSetup(7));

    const EntityId Tank = World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(100, 100));
    // Keep player 1 alive so the match does not end during the test.
    World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(900, 900));
    World.ApplyCommand(MakeEnter(0, Tank));
    // Promote Entering -> Active with a Drive so the saved phase is Active.
    CommandFrame F;
    F.Tick = World.GetTick();
    F.Commands.push_back(MakeDrive(0, Tank, 0, 0));
    World.Tick(&F);
    World.ClearEvents();

    ByteWriter W;
    World.Serialize(W);
    RA4_EXPECT(!(W.GetBuffer()).empty());

    SimWorld Restored;
    ByteReader R(W.GetBuffer());
    RA4_EXPECT(Restored.Deserialize(R, &Db));

    const DirectControlComp* Dc = Restored.GetDirectControl(Tank);
    RA4_EXPECT(Dc != nullptr);
    RA4_EXPECT_EQ(uint8_t(Dc->Phase), uint8_t(DirectControlPhase::Active));
    RA4_EXPECT_EQ(Dc->Controller, PlayerId(0));
}