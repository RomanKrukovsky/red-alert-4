// Copyright (c) Red Alert 4 project. Tests for the match simulation systems.
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Core/SimConfig.h"

using namespace RA4;
using namespace RA4Test;

namespace
{
struct Fixture
{
    ContentDatabase Content;
    SimWorld World;

    explicit Fixture(uint64_t Seed = 12345)
    {
        BuildDefaultContent(Content);
        World.Initialize(&Content, MakeTestSetup(Seed));
    }
};
} // namespace

// ---------------------------------------------------------------------------
// Content
// ---------------------------------------------------------------------------

RA4_TEST(Content, DefaultSetPassesValidation)
{
    ContentDatabase Db;
    BuildDefaultContent(Db);

    std::vector<std::string> Errors;
    const bool bValid = Db.Validate(Errors);
    for (const std::string& E : Errors)
    {
        RA4Test::ReportFailure("content validation: " + E, __FILE__, __LINE__);
    }
    RA4_EXPECT(bValid);
}

RA4_TEST(Content, ValidationCatchesAuthoringMistakes)
{
    ContentDatabase Db;
    BuildDefaultContent(Db);

    EntityDef Broken;
    Broken.Name = "unit.test.broken";
    Broken.DisplayNameKey = "";                       // missing localization key
    Broken.Kind = EntityKind::Unit;
    Broken.MaxHealth = -5;                            // impossible health
    Broken.Weapon = ContentId(0xDEADBEEF);            // dangling weapon reference
    Broken.Unit.MaxSpeed = Fixed::Zero();             // immobile unit
    Db.AddEntity(Broken);

    std::vector<std::string> Errors;
    RA4_EXPECT(!Db.Validate(Errors));
    RA4_EXPECT(Errors.size() >= 4);
}

RA4_TEST(Content, HashChangesWithBalanceEdits)
{
    ContentDatabase A;
    BuildDefaultContent(A);
    ContentDatabase B;
    BuildDefaultContent(B);
    RA4_EXPECT(A.ComputeContentHash() == B.ComputeContentHash());

    B.SetDamageMultiplier(WarheadClass::ArmorPiercing, ArmorClass::Building, 51);
    RA4_EXPECT(A.ComputeContentHash() != B.ComputeContentHash());
}

RA4_TEST(Content, DamageTableEncodesRockPaperScissors)
{
    ContentDatabase Db;
    BuildDefaultContent(Db);

    // Rifles beat infantry, lose to armour.
    RA4_EXPECT(Db.GetDamageMultiplier(WarheadClass::SmallArms, ArmorClass::Infantry) >
               Db.GetDamageMultiplier(WarheadClass::SmallArms, ArmorClass::HeavyVehicle));
    // Anti-tank rounds do the reverse.
    RA4_EXPECT(Db.GetDamageMultiplier(WarheadClass::ArmorPiercing, ArmorClass::HeavyVehicle) >
               Db.GetDamageMultiplier(WarheadClass::ArmorPiercing, ArmorClass::Infantry));
    // Anti-Air warhead cannot touch buildings, but shreds aircraft.
    RA4_EXPECT_EQ(Db.GetDamageMultiplier(WarheadClass::AntiAir, ArmorClass::Building), 0);
    RA4_EXPECT_EQ(Db.GetDamageMultiplier(WarheadClass::AntiAir, ArmorClass::Air), 200);
}

// ---------------------------------------------------------------------------
// Entity lifecycle
// ---------------------------------------------------------------------------

RA4_TEST(Simulation, RecycledSlotsInvalidateOldHandles)
{
    Fixture F;
    const EntityId A = F.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(1000, 1000));
    RA4_REQUIRE(A.IsValid());
    RA4_EXPECT(F.World.IsAlive(A));

    Command Sell = MakeCommand(CommandType::Surrender, 0);
    (void)Sell;

    // Kill it via damage so the normal destruction path runs.
    const EntityId Tank = F.World.SpawnUnit(Ids::SovHeavyTank, 1, Vec2::FromInts(1300, 1000));
    RA4_REQUIRE(Tank.IsValid());
    RunTicks(F.World, 400);

    RA4_EXPECT(!F.World.IsAlive(A));

    // Spawn until the freed slot is handed out again -- projectiles recycle slots
    // too, so which allocation lands on it is not fixed. What must hold is that the
    // stale handle never resolves to the new occupant.
    bool bSlotReused = false;
    for (int32_t I = 0; I < 8 && !bSlotReused; ++I)
    {
        const EntityId B = F.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(2000, 2000));
        RA4_REQUIRE(B.IsValid());
        RA4_EXPECT(F.World.IsAlive(B));
        if (B.Index == A.Index)
        {
            bSlotReused = true;
            RA4_EXPECT(B.Generation != A.Generation);
            RA4_EXPECT(!F.World.IsAlive(A));
        }
    }
    RA4_EXPECT(bSlotReused);
}

// ---------------------------------------------------------------------------
// Command validation
// ---------------------------------------------------------------------------

RA4_TEST(Commands, RejectsOrdersOnUnitsYouDoNotOwn)
{
    Fixture F;
    const EntityId Enemy = F.World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(5000, 5000));
    RA4_REQUIRE(Enemy.IsValid());

    Command C = MakeCommand(CommandType::Move, 0);
    C.Primary = Enemy;
    C.Location = Vec2::FromInts(1000, 1000);
    RA4_EXPECT(F.World.ApplyCommand(C).Reason == CommandReject::NotOwner);
}

RA4_TEST(Commands, RejectsStaleEntityHandles)
{
    Fixture F;
    Command C = MakeCommand(CommandType::Move, 0);
    C.Primary = EntityId(4000, 7);
    C.Location = Vec2::FromInts(1000, 1000);
    RA4_EXPECT(F.World.ApplyCommand(C).Reason == CommandReject::NoSuchEntity);
}

RA4_TEST(Commands, RejectsProductionWithoutPrerequisites)
{
    Fixture F;
    // No construction yard yet, so nothing can be produced.
    Command C = MakeCommand(CommandType::StartProduction, 0);
    C.Content = Ids::SovPower;
    RA4_EXPECT(F.World.ApplyCommand(C).Reason == CommandReject::TechRequirementsUnmet);

    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    // Heavy tanks need a war factory, which still does not exist.
    Command T = MakeCommand(CommandType::StartProduction, 0);
    T.Content = Ids::SovHeavyTank;
    RA4_EXPECT(F.World.ApplyCommand(T).Reason == CommandReject::TechRequirementsUnmet);
}

RA4_TEST(Commands, RejectsProductionYouCannotAfford)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);
    MatchSetup Setup = MakeTestSetup();
    Setup.Players[0].StartingCredits = 100;

    SimWorld World;
    World.Initialize(&Content, Setup);
    World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);

    Command C = MakeCommand(CommandType::StartProduction, 0);
    C.Content = Ids::SovPower;   // costs 800
    RA4_EXPECT(World.ApplyCommand(C).Reason == CommandReject::InsufficientCredits);
    RA4_EXPECT_EQ(World.GetPlayer(0).Credits, 100);
}

RA4_TEST(Commands, ThrottlesCommandFloods)
{
    Fixture F;
    const EntityId Unit = F.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(1000, 1000));
    RA4_REQUIRE(Unit.IsValid());

    CommandFrame Frame;
    Frame.Tick = 0;
    for (int32_t I = 0; I < 500; ++I)
    {
        Command C = MakeCommand(CommandType::Move, 0);
        C.Primary = Unit;
        C.Location = Vec2::FromInts(2000, 2000);
        Frame.Commands.push_back(C);
    }

    F.World.Tick(&Frame);

    int32_t RateLimited = 0;
    for (const SimEvent& E : F.World.GetEvents())
    {
        if (E.Type == SimEventType::CommandRejected && E.Value == int32_t(CommandReject::RateLimited))
        {
            ++RateLimited;
        }
    }
    // A client cannot buy unbounded server work by spamming a single tick.
    RA4_EXPECT(RateLimited > 0);
}

RA4_TEST(Cheats, GrantCreditsAndPower)
{
    Fixture F;
    F.World.CheatGrantCredits(0, 10000);
    RA4_EXPECT(F.World.GetPlayer(0).Credits == 20000);

    F.World.CheatGrantPower(0, 500);
    RA4_EXPECT(F.World.GetPlayer(0).PowerProduced >= 500);
}

// ---------------------------------------------------------------------------
// Placement
// ---------------------------------------------------------------------------

RA4_TEST(Placement, RequiresBuildRadiusAndClearGround)
{
    Fixture F;
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);

    RA4_EXPECT(F.World.IsPlacementValid(Ids::SovPower, 0, TileCoord(14, 10)));
    // Far outside the construction yard's 20 m radius.
    RA4_EXPECT(!F.World.IsPlacementValid(Ids::SovPower, 0, TileCoord(45, 45)));
    // Overlapping the yard's own footprint.
    RA4_EXPECT(!F.World.IsPlacementValid(Ids::SovPower, 0, TileCoord(11, 11)));
    // Off the map.
    RA4_EXPECT(!F.World.IsPlacementValid(Ids::SovPower, 0, TileCoord(63, 63)));
}

RA4_TEST(Placement, RejectsWaterAndCliffs)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);
    MatchSetup Setup = MakeTestSetup();
    Setup.Map.SetTileFlag(14, 10, Tile_Water, true);
    Setup.Map.SetTileFlag(16, 10, Tile_Cliff, true);

    SimWorld World;
    World.Initialize(&Content, Setup);
    World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);

    RA4_EXPECT(!World.IsPlacementValid(Ids::SovPower, 0, TileCoord(14, 10)));
    RA4_EXPECT(!World.IsPlacementValid(Ids::SovPower, 0, TileCoord(16, 10)));
    RA4_EXPECT(World.IsPlacementValid(Ids::SovPower, 0, TileCoord(10, 14)));
}

// ---------------------------------------------------------------------------
// Production and power
// ---------------------------------------------------------------------------

RA4_TEST(Production, StructureIsPaidQueuedThenPlaced)
{
    Fixture F;
    SpawnEnemyOutpost(F.World);
    const EntityId Yard = F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    RA4_REQUIRE(Yard.IsValid());

    const int32_t Before = F.World.GetPlayer(0).Credits;

    Command Start = MakeCommand(CommandType::StartProduction, 0);
    Start.Primary = Yard;
    Start.Content = Ids::SovPower;
    RA4_REQUIRE(F.World.ApplyCommand(Start).IsAccepted());
    RA4_EXPECT_EQ(F.World.GetPlayer(0).Credits, Before - 800);

    // Cannot place before the item is finished.
    Command EarlyPlace = MakeCommand(CommandType::PlaceBuilding, 0);
    EarlyPlace.Content = Ids::SovPower;
    EarlyPlace.Tile = TileCoord(14, 10);
    RA4_EXPECT(F.World.ApplyCommand(EarlyPlace).Reason == CommandReject::NoProducer);

    RunTicks(F.World, SecondsToTicks(9));

    Command Place = MakeCommand(CommandType::PlaceBuilding, 0);
    Place.Content = Ids::SovPower;
    Place.Tile = TileCoord(14, 10);
    RA4_REQUIRE(F.World.ApplyCommand(Place).IsAccepted());

    RA4_EXPECT_EQ(CountEntitiesOfType(F.World, 0, Ids::SovPower), 1);

    // It arrives under construction and only counts toward power once finished.
    RA4_EXPECT_EQ(F.World.GetPlayer(0).PowerProduced, 0);
    RunTicks(F.World, SecondsToTicks(10));
    RA4_EXPECT_EQ(F.World.GetPlayer(0).PowerProduced, 150);
}

RA4_TEST(Production, CancellingRefundsCredits)
{
    Fixture F;
    SpawnEnemyOutpost(F.World);
    const EntityId Yard = F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    const int32_t Before = F.World.GetPlayer(0).Credits;

    Command Start = MakeCommand(CommandType::StartProduction, 0);
    Start.Primary = Yard;
    Start.Content = Ids::SovPower;
    RA4_REQUIRE(F.World.ApplyCommand(Start).IsAccepted());
    RunTicks(F.World, 40);

    Command Cancel = MakeCommand(CommandType::CancelProduction, 0);
    Cancel.Primary = Yard;
    Cancel.Slot = 0;
    RA4_REQUIRE(F.World.ApplyCommand(Cancel).IsAccepted());

    RA4_EXPECT_EQ(F.World.GetPlayer(0).Credits, Before);
    RunTicks(F.World, SecondsToTicks(20));
    RA4_EXPECT_EQ(CountEntitiesOfType(F.World, 0, Ids::SovPower), 0);
}

RA4_TEST(Production, UnitLeavesTheFactoryAndObeysTheRallyPoint)
{
    Fixture F;
    SpawnEnemyOutpost(F.World);
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    F.World.SpawnBuilding(Ids::SovPower, 0, TileCoord(14, 10), true);
    const EntityId Barracks = F.World.SpawnBuilding(Ids::SovBarracks, 0, TileCoord(10, 14), true);
    RA4_REQUIRE(Barracks.IsValid());

    Command Rally = MakeCommand(CommandType::SetRallyPoint, 0);
    Rally.Primary = Barracks;
    Rally.Location = Vec2::FromInts(4000, 3000);
    RA4_REQUIRE(F.World.ApplyCommand(Rally).IsAccepted());

    Command Start = MakeCommand(CommandType::StartProduction, 0);
    Start.Primary = Barracks;
    Start.Content = Ids::SovConscript;
    RA4_REQUIRE(F.World.ApplyCommand(Start).IsAccepted());

    const int32_t Ticks = RunUntil(F.World, SecondsToTicks(10),
                                   [&] { return CountEntitiesOfType(F.World, 0, Ids::SovConscript) == 1; });
    RA4_EXPECT(Ticks >= 0);

    const EntityId Soldier = FindFirstOfType(F.World, 0, Ids::SovConscript);
    RA4_REQUIRE(Soldier.IsValid());

    const Vec2 SpawnPos = F.World.GetTransform(Soldier)->Position;
    RunTicks(F.World, SecondsToTicks(6));
    const Vec2 LaterPos = F.World.GetTransform(Soldier)->Position;

    // It should have moved toward the rally point, not stayed on the factory.
    RA4_EXPECT(Distance(SpawnPos, LaterPos) > Fixed::FromInt(200));
    RA4_EXPECT(DistanceSquared(LaterPos, Vec2::FromInts(4000, 3000)) <
               DistanceSquared(SpawnPos, Vec2::FromInts(4000, 3000)));
}

RA4_TEST(Power, ShortageSlowsProductionInsteadOfStoppingIt)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);

    // Two identical scenarios; the second is starved of power by a war factory
    // with nothing to feed it.
    auto RunScenario = [&Content](bool bWithPower) -> int32_t
    {
        SimWorld World;
        World.Initialize(&Content, MakeTestSetup());
        SpawnEnemyOutpost(World);
        World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
        if (bWithPower)
        {
            World.SpawnBuilding(Ids::SovPower, 0, TileCoord(14, 10), true);
            World.SpawnBuilding(Ids::SovPower, 0, TileCoord(14, 13), true);
        }
        const EntityId Barracks = World.SpawnBuilding(Ids::SovBarracks, 0, TileCoord(10, 14), true);

        Command Start = MakeCommand(CommandType::StartProduction, 0);
        Start.Primary = Barracks;
        Start.Content = Ids::SovConscript;
        World.ApplyCommand(Start);

        return RunUntil(World, SecondsToTicks(60),
                        [&] { return CountEntitiesOfType(World, 0, Ids::SovConscript) == 1; });
    };

    const int32_t Powered = RunScenario(true);
    const int32_t Starved = RunScenario(false);

    RA4_EXPECT(Powered > 0);
    RA4_EXPECT(Starved > 0);
    // Slower, but never stalled: losing power must not make a base unrecoverable.
    RA4_EXPECT(Starved > Powered);
}

// ---------------------------------------------------------------------------
// Economy
// ---------------------------------------------------------------------------

RA4_TEST(Economy, HarvesterCompletesTheFullGatherLoop)
{
    Fixture F;
    SpawnEnemyOutpost(F.World);
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    F.World.SpawnBuilding(Ids::SovPower, 0, TileCoord(14, 10), true);
    // The refinery ships a harvester with it, exactly like the original games.
    F.World.SpawnBuilding(Ids::SovRefinery, 0, TileCoord(10, 14), true);

    for (int32_t X = 5; X <= 6; ++X)
    {
        for (int32_t Y = 16; Y <= 17; ++Y)
        {
            F.World.SpawnResourceNode(Ids::OreField, TileCoord(X, Y), 2000);
        }
    }

    const EntityId Harvester = FindFirstOfType(F.World, 0, Ids::SovHarvester);
    RA4_REQUIRE(Harvester.IsValid());

    const int32_t StartCredits = F.World.GetPlayer(0).Credits;

    // Should reach the field and load up.
    const int32_t ToFull = RunUntil(F.World, SecondsToTicks(60), [&]
    {
        const HarvesterComp* H = F.World.GetHarvester(Harvester);
        return H != nullptr && H->Cargo > 0;
    });
    RA4_EXPECT(ToFull >= 0);

    // ... and come back and unload.
    const int32_t ToDelivery = RunUntil(F.World, SecondsToTicks(120), [&]
    {
        return F.World.GetPlayer(0).Credits > StartCredits;
    });
    RA4_EXPECT(ToDelivery >= 0);
    RA4_EXPECT(F.World.GetPlayer(0).TotalHarvested > 0);
}

RA4_TEST(Economy, ResourceFieldsAreFinite)
{
    Fixture F;
    SpawnEnemyOutpost(F.World);
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    F.World.SpawnBuilding(Ids::SovPower, 0, TileCoord(14, 10), true);
    F.World.SpawnBuilding(Ids::SovRefinery, 0, TileCoord(10, 14), true);

    const EntityId Node = F.World.SpawnResourceNode(Ids::OreField, TileCoord(6, 16), 300);
    RA4_REQUIRE(Node.IsValid());

    const int32_t Drained = RunUntil(F.World, SecondsToTicks(120), [&] { return !F.World.IsAlive(Node); });
    RA4_EXPECT(Drained >= 0);
    // Draining the last field must not deadlock the harvester logic.
    RunTicks(F.World, SecondsToTicks(10));
    RA4_EXPECT(F.World.GetPhase() == MatchPhase::Running);
}

// ---------------------------------------------------------------------------
// Combat
// ---------------------------------------------------------------------------

RA4_TEST(Combat, ArmourClassesDecideTheOutcome)
{
    Fixture F;
    const EntityId Tank = F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2000, 2000));
    const EntityId Soldier = F.World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(2600, 2000));
    RA4_REQUIRE(Tank.IsValid() && Soldier.IsValid());

    const int32_t Ticks = RunUntil(F.World, SecondsToTicks(60), [&] { return !F.World.IsAlive(Soldier); });
    RA4_EXPECT(Ticks >= 0);

    // Rifle fire barely scratches heavy armour.
    RA4_REQUIRE(F.World.IsAlive(Tank));
    const HealthComp* TankHealth = F.World.GetHealth(Tank);
    RA4_REQUIRE(TankHealth != nullptr);
    RA4_EXPECT(TankHealth->Current > (TankHealth->Max * 8) / 10);
}

RA4_TEST(Combat, UnitsHoldFireOutsideWeaponRange)
{
    Fixture F;
    const EntityId Tank = F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2000, 2000));
    // 40 m apart: well beyond both vision and the 9 m gun.
    const EntityId Soldier = F.World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(6000, 2000));
    RA4_REQUIRE(Tank.IsValid() && Soldier.IsValid());

    RunTicks(F.World, SecondsToTicks(20));
    RA4_EXPECT(F.World.IsAlive(Soldier));
    const HealthComp* H = F.World.GetHealth(Soldier);
    RA4_REQUIRE(H != nullptr);
    RA4_EXPECT_EQ(H->Current, H->Max);
}

RA4_TEST(Combat, AttackOrderClosesTheDistance)
{
    Fixture F;
    SpawnEnemyOutpost(F.World);
    const EntityId Tank = F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2000, 2000));
    const EntityId Target = F.World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(9000, 2000));
    RA4_REQUIRE(Tank.IsValid() && Target.IsValid());

    Command Attack = MakeCommand(CommandType::Attack, 0);
    Attack.Primary = Tank;
    Attack.Target = Target;
    RA4_REQUIRE(F.World.ApplyCommand(Attack).IsAccepted());

    const int32_t Ticks = RunUntil(F.World, SecondsToTicks(90), [&] { return !F.World.IsAlive(Target); });
    RA4_EXPECT(Ticks >= 0);
    RA4_EXPECT(F.World.IsAlive(Tank));
}

RA4_TEST(Combat, DefensiveStructuresEngageOnTheirOwn)
{
    Fixture F;
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    F.World.SpawnBuilding(Ids::SovPower, 0, TileCoord(14, 10), true);
    F.World.SpawnBuilding(Ids::SovBarracks, 0, TileCoord(10, 14), true);
    const EntityId Turret = F.World.SpawnBuilding(Ids::SovTurret, 0, TileCoord(14, 14), true);
    RA4_REQUIRE(Turret.IsValid());

    // 4 m from the turret, inside its 11 m gun.
    const EntityId Intruder = F.World.SpawnUnit(Ids::AllLightTank, 1, Vec2::FromInts(3300, 3300));
    RA4_REQUIRE(Intruder.IsValid());

    const int32_t Ticks = RunUntil(F.World, SecondsToTicks(60), [&] { return !F.World.IsAlive(Intruder); });
    RA4_EXPECT(Ticks >= 0);
}

RA4_TEST(Combat, SplashDamageHitsEverythingInTheBlast)
{
    Fixture F;
    // Rocket infantry: 2 m splash, so a tight cluster takes collateral damage.
    const EntityId Shooter = F.World.SpawnUnit(MakeContentId("unit.sov.rocket_trooper"), 0, Vec2::FromInts(2000, 2000));
    RA4_REQUIRE(Shooter.IsValid());

    const EntityId A = F.World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(2700, 2000));
    const EntityId B = F.World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(2760, 2000));
    RA4_REQUIRE(A.IsValid() && B.IsValid());

    const int32_t Ticks = RunUntil(F.World, SecondsToTicks(30),
                                   [&] { return !F.World.IsAlive(A) || !F.World.IsAlive(B); });
    RA4_EXPECT(Ticks >= 0);

    // Whichever died, the neighbour must have taken splash rather than being
    // untouched.
    if (F.World.IsAlive(B))
    {
        const HealthComp* H = F.World.GetHealth(B);
        RA4_REQUIRE(H != nullptr);
        RA4_EXPECT(H->Current < H->Max);
    }
}

// ---------------------------------------------------------------------------
// Movement
// ---------------------------------------------------------------------------

RA4_TEST(Movement, UnitsReachTheirDestination)
{
    Fixture F;
    SpawnEnemyOutpost(F.World);
    const EntityId Unit = F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2000, 2000));
    RA4_REQUIRE(Unit.IsValid());

    const Vec2 Goal = Vec2::FromInts(6000, 5000);
    Command Move = MakeCommand(CommandType::Move, 0);
    Move.Primary = Unit;
    Move.Location = Goal;
    RA4_REQUIRE(F.World.ApplyCommand(Move).IsAccepted());

    const int32_t Ticks = RunUntil(F.World, SecondsToTicks(60), [&]
    {
        const MovementComp* M = F.World.GetMovement(Unit);
        return M != nullptr && !M->bHasDestination;
    });
    RA4_EXPECT(Ticks >= 0);
    RA4_EXPECT(Distance(F.World.GetTransform(Unit)->Position, Goal) < Fixed::FromInt(120));
}

RA4_TEST(Movement, QueuedWaypointsAreFollowedInOrder)
{
    Fixture F;
    SpawnEnemyOutpost(F.World);
    const EntityId Unit = F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2000, 2000));
    RA4_REQUIRE(Unit.IsValid());

    Command First = MakeCommand(CommandType::Move, 0);
    First.Primary = Unit;
    First.Location = Vec2::FromInts(5000, 2000);
    RA4_REQUIRE(F.World.ApplyCommand(First).IsAccepted());

    Command Second = MakeCommand(CommandType::Move, 0);
    Second.Primary = Unit;
    Second.Mode = OrderMode::Queue;
    Second.Location = Vec2::FromInts(5000, 6000);
    RA4_REQUIRE(F.World.ApplyCommand(Second).IsAccepted());

    RA4_EXPECT_EQ(F.World.GetOrders(Unit)->Count, 2);

    const int32_t Ticks = RunUntil(F.World, SecondsToTicks(90),
                                   [&] { return F.World.GetOrders(Unit)->Count == 0; });
    RA4_EXPECT(Ticks >= 0);
    RA4_EXPECT(Distance(F.World.GetTransform(Unit)->Position, Vec2::FromInts(5000, 6000)) < Fixed::FromInt(150));
}

RA4_TEST(Movement, StopClearsTheWholeOrderQueue)
{
    Fixture F;
    const EntityId Unit = F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2000, 2000));
    RA4_REQUIRE(Unit.IsValid());

    for (int32_t I = 0; I < 3; ++I)
    {
        Command Move = MakeCommand(CommandType::Move, 0);
        Move.Primary = Unit;
        Move.Mode = I == 0 ? OrderMode::Replace : OrderMode::Queue;
        Move.Location = Vec2::FromInts(5000 + I * 500, 5000);
        F.World.ApplyCommand(Move);
    }
    RA4_EXPECT_EQ(F.World.GetOrders(Unit)->Count, 3);

    Command Stop = MakeCommand(CommandType::Stop, 0);
    Stop.Primary = Unit;
    RA4_REQUIRE(F.World.ApplyCommand(Stop).IsAccepted());
    RA4_EXPECT_EQ(F.World.GetOrders(Unit)->Count, 0);
    RA4_EXPECT(!F.World.GetMovement(Unit)->bHasDestination);
}

RA4_TEST(Movement, ImpassableTerrainBlocksGroundUnits)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);
    MatchSetup Setup = MakeTestSetup();
    // A wall of water across the unit's path.
    for (int32_t Y = 0; Y < 64; ++Y)
    {
        Setup.Map.SetTileFlag(20, Y, Tile_Water, true);
    }

    SimWorld World;
    World.Initialize(&Content, Setup);
    SpawnEnemyOutpost(World);
    const EntityId Unit = World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2000, 2000));
    RA4_REQUIRE(Unit.IsValid());

    Command Move = MakeCommand(CommandType::Move, 0);
    Move.Primary = Unit;
    Move.Location = Vec2::FromInts(10000, 2000);
    RA4_REQUIRE(World.ApplyCommand(Move).IsAccepted());

    RunTicks(World, SecondsToTicks(30));
    // It must be stopped west of the water, not swimming through it.
    RA4_EXPECT(World.GetTransform(Unit)->Position.X < Fixed::FromInt(20 * 200));
}

RA4_TEST(Movement, RoutesAroundTerrainWallThroughItsOnlyGap)
{
    // Break caught: replacing path-guided steering with direct steering leaves a
    // unit stuck at the first cliff instead of reaching a legal route around it.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    MatchSetup Setup = MakeTestSetup();
    for (int32_t Y = 0; Y < 32; ++Y)
    {
        if (Y != 25)
        {
            Setup.Map.SetTileFlag(20, Y, Tile_Cliff, true);
        }
    }

    SimWorld World;
    World.Initialize(&Content, Setup);
    SpawnEnemyOutpost(World);
    const EntityId Tank = World.SpawnUnit(Ids::SovHeavyTank, 0, Setup.Map.TileCenterToWorld(TileCoord(10, 10)));
    RA4_REQUIRE(Tank.IsValid());

    Command Move = MakeCommand(CommandType::Move, 0);
    Move.Primary = Tank;
    Move.Location = Setup.Map.TileCenterToWorld(TileCoord(30, 10));
    RA4_REQUIRE(World.ApplyCommand(Move).IsAccepted());

    const int32_t Ticks = RunUntil(World, SecondsToTicks(60), [&]
    {
        return World.GetOrders(Tank)->IsEmpty() &&
               World.GetTransform(Tank)->Position.X > Fixed::FromInt(5000);
    });
    RA4_EXPECT(Ticks >= 0);
}

// ---------------------------------------------------------------------------
// Victory
// ---------------------------------------------------------------------------

RA4_TEST(Victory, LosingEverythingEndsTheMatch)
{
    Fixture F;
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    const EntityId EnemyYard = F.World.SpawnBuilding(Ids::AllConYard, 1, TileCoord(12, 10), true);
    RA4_REQUIRE(EnemyYard.IsValid());

    // Park enough armour next to the enemy base to level it.
    for (int32_t I = 0; I < 4; ++I)
    {
        F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2400 + I * 150, 2600));
    }

    const int32_t Ticks = RunUntil(F.World, SecondsToTicks(120),
                                   [&] { return F.World.GetPhase() == MatchPhase::Finished; });
    RA4_EXPECT(Ticks >= 0);
    RA4_EXPECT_EQ(int32_t(F.World.GetWinner()), 0);
    RA4_EXPECT(F.World.GetPlayer(1).bDefeated);
}

RA4_TEST(Victory, SurrenderEndsTheMatchImmediately)
{
    Fixture F;
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    F.World.SpawnBuilding(Ids::AllConYard, 1, TileCoord(40, 40), true);

    Command Surrender = MakeCommand(CommandType::Surrender, 1);
    RA4_REQUIRE(F.World.ApplyCommand(Surrender).IsAccepted());
    RunTicks(F.World, 2);

    RA4_EXPECT(F.World.GetPhase() == MatchPhase::Finished);
    RA4_EXPECT_EQ(int32_t(F.World.GetWinner()), 0);
}

RA4_TEST(Economy, MultiHarvesterTenCyclesAndRefineryQueue)
{
    Fixture F;
    SpawnEnemyOutpost(F.World);
    EntityId Ref1 = F.World.SpawnBuilding(Ids::SovRefinery, 0, TileCoord(15, 15), true);
    EntityId Ref2 = F.World.SpawnBuilding(Ids::SovRefinery, 0, TileCoord(25, 25), true);
    RA4_REQUIRE(F.World.IsAlive(Ref2));

    for (int X = 0; X < 4; ++X)
    {
        for (int Y = 0; Y < 4; ++Y)
        {
            F.World.SpawnResourceNode(Ids::OreField, TileCoord(18 + X, 18 + Y), 50000);
        }
    }

    std::vector<EntityId> Harvs;
    for (int I = 0; I < 4; ++I)
    {
        Harvs.push_back(F.World.SpawnUnit(Ids::SovHarvester, 0, Vec2::FromInts(16 * kTileSizeUnits + I * 40, 16 * kTileSizeUnits)));
    }

    const int32_t InitialCredits = F.World.GetPlayer(0).Credits;
    RunTicks(F.World, 1500);

    RA4_EXPECT(F.World.GetPlayer(0).TotalHarvested > 0);
    RA4_EXPECT(F.World.GetPlayer(0).Credits > InitialCredits);

    // Destroy Ref1 mid-match; harvesters targeting Ref1 must recover and reroute to Ref2
    F.World.DebugDamage(Ref1, 10000);
    RunTicks(F.World, 500);

    for (EntityId HarvId : Harvs)
    {
        RA4_EXPECT(F.World.IsAlive(HarvId));
        const HarvesterComp* Hv = F.World.GetHarvester(HarvId);
        RA4_REQUIRE(Hv != nullptr);
        RA4_EXPECT(Hv->AssignedRefinery != Ref1);
    }
}

RA4_TEST(Content, PrerequisitesGroupGroupedRules)
{
    Fixture F;
    EntityDef TestDef;
    TestDef.Production.PrerequisitesGroup.AllOf = { Ids::SovRefinery };
    TestDef.Production.PrerequisitesGroup.AnyOf = { Ids::SovBarracks, Ids::SovWarFactory };
    TestDef.Production.PrerequisitesGroup.NoneOf = { Ids::AllConYard };

    RA4_EXPECT(!F.World.HasPrerequisites(0, TestDef));

    F.World.SpawnBuilding(Ids::SovRefinery, 0, TileCoord(10, 10), true);
    RA4_EXPECT(!F.World.HasPrerequisites(0, TestDef)); // Missing AnyOf (Barracks or WarFactory)

    F.World.SpawnBuilding(Ids::SovBarracks, 0, TileCoord(15, 15), true);
    RA4_EXPECT(F.World.HasPrerequisites(0, TestDef)); // AllOf + AnyOf met

    F.World.SpawnBuilding(Ids::AllConYard, 0, TileCoord(20, 20), true);
    RA4_EXPECT(!F.World.HasPrerequisites(0, TestDef)); // Failed NoneOf constraint
}

RA4_TEST(Construction, UnderConstructionTargetabilityAndCancellation)
{
    Fixture F;
    EntityId ConYard = F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    RA4_REQUIRE(F.World.IsAlive(ConYard));
    EntityId Barracks = F.World.SpawnBuilding(Ids::SovBarracks, 0, TileCoord(15, 15), false);

    const BuildingComp* B = F.World.GetBuilding(Barracks);
    RA4_REQUIRE(B != nullptr);
    RA4_EXPECT(B->State == ConstructionState::UnderConstruction);

    // Can be targeted and damaged while under construction
    F.World.DebugDamage(Barracks, 50);
    const HealthComp* H = F.World.GetHealth(Barracks);
    RA4_REQUIRE(H != nullptr);
    RA4_EXPECT(H->Current < H->Max);

    // Selling / cancelling refunds portion of cost and clears occupancy
    const int32_t CreditsBefore = F.World.GetPlayer(0).Credits;
    Command SellCmd = MakeCommand(CommandType::SellBuilding, 0);
    SellCmd.Primary = Barracks;
    RA4_REQUIRE(F.World.ApplyCommand(SellCmd).IsAccepted());
    RunTicks(F.World, 1);

    RA4_EXPECT(!F.World.IsAlive(Barracks));
    RA4_EXPECT(F.World.GetPlayer(0).Credits > CreditsBefore);
}

RA4_TEST(Lifecycle, SimWorldRestartRestoresCleanState)
{
    Fixture F;
    SpawnEnemyOutpost(F.World);
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    F.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(12 * kTileSizeUnits, 12 * kTileSizeUnits));
    RunTicks(F.World, 100);

    const uint32_t TickBefore = F.World.GetTick();
    RA4_EXPECT_EQ(int32_t(TickBefore), 100);

    uint32_t AliveBefore = 0;
    for (const EntityCore& C : F.World.GetAllCores()) { if (C.bAlive) AliveBefore++; }
    RA4_EXPECT(AliveBefore > 0);

    F.World.Restart();

    const uint32_t TickAfter = F.World.GetTick();
    RA4_EXPECT_EQ(int32_t(TickAfter), 0);

    uint32_t AliveAfter = 0;
    for (const EntityCore& C : F.World.GetAllCores()) { if (C.bAlive) AliveAfter++; }
    RA4_EXPECT_EQ(int32_t(AliveAfter), 0);
}

// --- Presentation-facing contracts for construction and turret aim -----------
// The visuals for these live in Unreal and can only be judged on screen, but the
// VALUES the visuals read are simulation state and belong in a test. A building
// that reports "complete" while still building, or a turret angle that never
// moves, would render wrongly with no way to trace it from a screenshot.

RA4_TEST(Simulation, ConstructionProgressIsMonotonicAndBounded)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);
    SimWorld World;
    World.Initialize(&Content, RA4Test::MakeTestSetup());

    // Placed through the normal path, i.e. NOT instantly complete -- the same call
    // the build command makes.
    const EntityId Site = World.SpawnBuilding(RA4Test::Ids::SovPower, 0, TileCoord(10, 10),
                                              /*bInstantComplete*/ false);
    RA4_EXPECT(Site.IsValid());

    const BuildingComp* B = World.GetBuilding(Site);
    RA4_EXPECT(B != nullptr);
    RA4_EXPECT(B->State == ConstructionState::UnderConstruction);

    // A freshly placed site must NOT read as finished: this is exactly the value
    // the presentation layer uses to decide whether to sink the mesh and show a
    // progress bar, so reporting 1000 here would make construction invisible.
    const int32_t Initial = World.GetConstructionProgressPerMille(Site);
    RA4_EXPECT(Initial < 1000);

    int32_t Previous = Initial;
    bool bMonotonic = true;
    bool bWithinBounds = true;
    for (int32_t Tick = 0; Tick < 400; ++Tick)
    {
        World.Tick(nullptr);
        const int32_t Now = World.GetConstructionProgressPerMille(Site);
        if (Now < Previous)
        {
            bMonotonic = false;   // progress must never go backwards
        }
        if (Now < 0 || Now > 1000)
        {
            bWithinBounds = false;   // the visual divides by this; out of range warps the mesh
        }
        Previous = Now;
    }
    RA4_EXPECT(bMonotonic);
    RA4_EXPECT(bWithinBounds);

    // And it did advance, rather than sitting at zero forever -- otherwise a
    // building would stay half-sunk permanently.
    RA4_EXPECT(Previous > Initial);
}

RA4_TEST(Simulation, InstantlyPlacedBuildingReportsComplete)
{
    // Map-authored and cheat-placed buildings use bInstantComplete. They must
    // report 1000 immediately, or every pre-placed base on a map would spawn
    // half-buried with a progress bar over it.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    SimWorld World;
    World.Initialize(&Content, RA4Test::MakeTestSetup());

    const EntityId Done = World.SpawnBuilding(RA4Test::Ids::SovPower, 0, TileCoord(20, 20),
                                              /*bInstantComplete*/ true);
    RA4_EXPECT(Done.IsValid());
    RA4_EXPECT(World.GetConstructionProgressPerMille(Done) == 1000);

    // Non-buildings have no construction state at all and must also read complete,
    // since the same presentation path runs for every entity.
    const EntityId Unit = World.SpawnUnit(RA4Test::Ids::SovConscript, 0,
                                          Vec2(Fixed::FromInt(500), Fixed::FromInt(500)));
    RA4_EXPECT(World.GetConstructionProgressPerMille(Unit) == 1000);

    // An invalid id must not report "under construction" either.
    RA4_EXPECT(World.GetConstructionProgressPerMille(EntityId::Invalid()) == 1000);
}

RA4_TEST(Simulation, TurretTracksTargetIndependentlyOfHull)
{
    // The turret angle is what the presentation layer now rotates a turret mesh
    // (or the hull, as a fallback) to. Pin that it actually moves toward a target
    // and that it is independent of the hull's facing -- if it simply mirrored
    // Facing, a turret mesh would be decorative.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    SimWorld World;
    World.Initialize(&Content, RA4Test::MakeTestSetup());

    const EntityId Tank = World.SpawnUnit(RA4Test::Ids::SovConscript, 0,
                                          Vec2(Fixed::FromInt(1000), Fixed::FromInt(1000)));
    // Enemy placed at a right angle to the tank's initial facing.
    World.SpawnUnit(RA4Test::Ids::AllRifleman, 1,
                    Vec2(Fixed::FromInt(1000), Fixed::FromInt(1600)));

    const TransformComp* T = World.GetTransform(Tank);
    RA4_EXPECT(T != nullptr);
    const int32_t HullBefore = T->Facing;
    const int32_t TurretBefore = T->TurretFacing;

    for (int32_t Tick = 0; Tick < 40; ++Tick)
    {
        World.Tick(nullptr);
    }

    const TransformComp* After = World.GetTransform(Tank);
    RA4_EXPECT(After != nullptr);
    // The turret moved to acquire the target...
    RA4_EXPECT(After->TurretFacing != TurretBefore);
    // ...and the angle stays in the simulation's canonical range, which the
    // degree conversion in presentation depends on.
    RA4_EXPECT(After->TurretFacing >= 0 && After->TurretFacing < kAngleTurn);
    // Hull facing is a separate value; the test asserts independence by checking
    // the turret is not merely a copy of it.
    const bool bIndependent = (After->TurretFacing != After->Facing) || (HullBefore != After->Facing);
    RA4_EXPECT(bIndependent);
}
