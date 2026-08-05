// Copyright (c) Red Alert 4 project. Tests for the match simulation systems.
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Core/ByteStream.h"
#include "RA4Core/SimConfig.h"

#include <utility>
#include <vector>

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

// ADR-0012 replaced the upfront charge with per-tick payment, so queuing something
// you cannot yet afford is a legitimate plan rather than an error. What must still
// hold is that a poor player never receives what they did not pay for: the item
// funds as far as the treasury allows, then stalls in Starved without completing.
RA4_TEST(Commands, ProductionYouCannotAffordQueuesButStarvesInsteadOfCompleting)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);
    MatchSetup Setup = MakeTestSetup();
    Setup.Players[0].StartingCredits = 100;

    SimWorld World;
    World.Initialize(&Content, Setup);
    // Without a live opponent the victory check ends the match on tick 0 and no
    // system runs again, which would make this test pass for the wrong reason.
    SpawnEnemyOutpost(World);
    const EntityId Yard = World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);

    Command C = MakeCommand(CommandType::StartProduction, 0);
    C.Primary = Yard;
    C.Content = Ids::SovPower;   // costs 800; the player has 100
    RA4_EXPECT(World.ApplyCommand(C).IsAccepted());
    RA4_EXPECT_EQ(World.GetPlayer(0).Credits, 100);   // nothing charged at queue time

    // Long enough that an unpaid item would have finished had progress been free.
    for (int32_t T = 0; T < 400; ++T)
    {
        World.Tick(nullptr);
    }

    const BuildingComp* B = World.GetBuilding(Yard);
    RA4_REQUIRE(B != nullptr);
    RA4_REQUIRE(B->Queue.size() == 1u);
    const ProductionItem& Item = B->Queue.front();

    // Every credit the player had went into it, and not one more.
    RA4_EXPECT_EQ(World.GetPlayer(0).Credits, 0);
    RA4_EXPECT_EQ(Item.PaidCredits, 100);
    RA4_EXPECT(Item.State == FlowPaymentState::Starved);
    // Crucially: not delivered. An unfunded item must never reach Completed.
    RA4_EXPECT(Item.ProgressTicks < Item.TotalTicks * 100);
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
    // ADR-0012: nothing is charged when the order is given. The price is drawn a
    // slice per tick while the item builds.
    RA4_EXPECT_EQ(F.World.GetPlayer(0).Credits, Before);

    // Cannot place before the item is finished.
    Command EarlyPlace = MakeCommand(CommandType::PlaceBuilding, 0);
    EarlyPlace.Content = Ids::SovPower;
    EarlyPlace.Tile = TileCoord(14, 10);
    RA4_EXPECT(F.World.ApplyCommand(EarlyPlace).Reason == CommandReject::NoProducer);

    RunTicks(F.World, SecondsToTicks(9));

    // By completion the full 800 has been paid -- no more, no less.
    RA4_EXPECT_EQ(F.World.GetPlayer(0).Credits, Before - 800);

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

RA4_TEST(Production, CancellingRefundsMostOfWhatWasActuallyPaid)
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

    // Only part of the price has been drawn so far, and that is what the refund is
    // computed from -- ADR-0012 refunds a fraction of PaidCredits, never of the
    // full cost, so cancelling can never be profitable.
    const BuildingComp* B = F.World.GetBuilding(Yard);
    RA4_REQUIRE(B != nullptr && B->Queue.size() == 1u);
    const int32_t Paid = B->Queue.front().PaidCredits;
    const int32_t TotalCost = B->Queue.front().TotalCost;
    RA4_EXPECT(Paid > 0 && Paid < TotalCost);
    RA4_EXPECT_EQ(F.World.GetPlayer(0).Credits, Before - Paid);

    // 40 of ~160 ticks is a quarter or less of the price, so this is the early
    // building tier: 90% back.
    RA4_EXPECT(Paid * 100 <= TotalCost * 25);
    const int32_t ExpectedRefund = (Paid * 90) / 100;

    Command Cancel = MakeCommand(CommandType::CancelProduction, 0);
    Cancel.Primary = Yard;
    Cancel.Slot = 0;
    RA4_REQUIRE(F.World.ApplyCommand(Cancel).IsAccepted());

    RA4_EXPECT_EQ(F.World.GetPlayer(0).Credits, Before - Paid + ExpectedRefund);
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

// ---------------------------------------------------------------------------
// Flow payment (ADR-0012)
// ---------------------------------------------------------------------------

namespace
{
// Returns the head of the given producer's queue, or nullptr when it is empty.
const ProductionItem* QueueHead(const SimWorld& World, EntityId Producer)
{
    const BuildingComp* B = World.GetBuilding(Producer);
    return (B != nullptr && !B->Queue.empty()) ? &B->Queue.front() : nullptr;
}

// A match with a live opponent, so SystemVictory does not end it on tick 0 and
// silently stop every other system.
struct FlowFixture
{
    ContentDatabase Content;
    SimWorld World;

    explicit FlowFixture(int32_t StartingCredits, uint64_t Seed = 4242)
    {
        BuildDefaultContent(Content);
        MatchSetup Setup = MakeTestSetup(Seed);
        Setup.Players[0].StartingCredits = StartingCredits;
        World.Initialize(&Content, Setup);
        SpawnEnemyOutpost(World);
    }
};
} // namespace

RA4_TEST(FlowPayment, PriceIsDrawnGraduallyAndTotalsExactlyTheCost)
{
    FlowFixture F(10000);
    const EntityId Yard = F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);

    Command Start = MakeCommand(CommandType::StartProduction, 0);
    Start.Primary = Yard;
    Start.Content = Ids::SovPower;   // 800 credits
    RA4_REQUIRE(F.World.ApplyCommand(Start).IsAccepted());
    RA4_EXPECT_EQ(F.World.GetPlayer(0).Credits, 10000);

    // Mid-build: partly paid, partly built, and the two track each other.
    RunTicks(F.World, 80);
    const ProductionItem* Mid = QueueHead(F.World, Yard);
    RA4_REQUIRE(Mid != nullptr);
    RA4_EXPECT(Mid->PaidCredits > 0 && Mid->PaidCredits < Mid->TotalCost);
    RA4_EXPECT(Mid->ProgressTicks > 0);
    RA4_EXPECT(F.World.GetPlayer(0).Credits < 10000);

    RunTicks(F.World, 200);
    const ProductionItem* Done = QueueHead(F.World, Yard);
    RA4_REQUIRE(Done != nullptr);   // a structure waits in the queue for placement
    RA4_EXPECT(Done->State == FlowPaymentState::Completed);
    // Never overcharged and never undercharged, whatever the rounding.
    RA4_EXPECT_EQ(Done->PaidCredits, Done->TotalCost);
    RA4_EXPECT_EQ(F.World.GetPlayer(0).Credits, 10000 - 800);
}

RA4_TEST(FlowPayment, StarvedItemKeepsItsProgressAndResumesWhenIncomeReturns)
{
    // Enough to start but not to finish, so funding stalls part-way.
    FlowFixture F(200);
    const EntityId Yard = F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);

    Command Start = MakeCommand(CommandType::StartProduction, 0);
    Start.Primary = Yard;
    Start.Content = Ids::SovPower;
    RA4_REQUIRE(F.World.ApplyCommand(Start).IsAccepted());

    RunTicks(F.World, 200);
    const ProductionItem* Stalled = QueueHead(F.World, Yard);
    RA4_REQUIRE(Stalled != nullptr);
    RA4_EXPECT(Stalled->State == FlowPaymentState::Starved);
    RA4_EXPECT_EQ(Stalled->PaidCredits, 200);
    RA4_EXPECT_EQ(F.World.GetPlayer(0).Credits, 0);
    const int32_t FrozenProgress = Stalled->ProgressTicks;
    RA4_EXPECT(FrozenProgress > 0);

    // Starvation must freeze progress, not rewind it: ADR-0012 forbids regression
    // because it would make the result depend on the path through funding states.
    RunTicks(F.World, 100);
    const ProductionItem* StillStalled = QueueHead(F.World, Yard);
    RA4_REQUIRE(StillStalled != nullptr);
    RA4_EXPECT_EQ(StillStalled->ProgressTicks, FrozenProgress);
    RA4_EXPECT_EQ(StillStalled->PaidCredits, 200);

    // Income arrives; the item must pick up from where it stopped and finish.
    F.World.CheatGrantCredits(0, 5000);
    RunTicks(F.World, 300);
    const ProductionItem* Finished = QueueHead(F.World, Yard);
    RA4_REQUIRE(Finished != nullptr);
    RA4_EXPECT(Finished->State == FlowPaymentState::Completed);
    RA4_EXPECT_EQ(Finished->PaidCredits, Finished->TotalCost);
    RA4_EXPECT(Finished->ProgressTicks >= FrozenProgress);
}

RA4_TEST(FlowPayment, DestroyedProducerRefundsHalfOfWhatTheQueueHadPaid)
{
    FlowFixture F(10000);
    const EntityId Yard = F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    F.World.SpawnBuilding(Ids::SovPower, 0, TileCoord(14, 10), true);
    const EntityId Barracks = F.World.SpawnBuilding(Ids::SovBarracks, 0, TileCoord(10, 14), true);
    RA4_REQUIRE(Yard.IsValid() && Barracks.IsValid());

    Command Start = MakeCommand(CommandType::StartProduction, 0);
    Start.Primary = Barracks;
    Start.Content = Ids::SovConscript;
    RA4_REQUIRE(F.World.ApplyCommand(Start).IsAccepted());

    RunTicks(F.World, 10);
    const ProductionItem* Item = QueueHead(F.World, Barracks);
    RA4_REQUIRE(Item != nullptr);
    RA4_EXPECT(Item->PaidCredits > 0);

    // Pausing stops funding, so the amount paid stops moving and the refund becomes
    // exactly predictable. Without this the final tick draws one more slice between
    // sampling and the death sweep, and the assertion chases a moving target.
    Command Pause = MakeCommand(CommandType::PauseProduction, 0);
    Pause.Primary = Barracks;
    Pause.Slot = 0;
    RA4_REQUIRE(F.World.ApplyCommand(Pause).IsAccepted());
    RunTicks(F.World, 3);

    const ProductionItem* Frozen = QueueHead(F.World, Barracks);
    RA4_REQUIRE(Frozen != nullptr);
    RA4_EXPECT(Frozen->State == FlowPaymentState::ManuallyPaused);
    const int32_t Paid = Frozen->PaidCredits;
    const int32_t CreditsBefore = F.World.GetPlayer(0).Credits;
    const int32_t ExpectedRefund = (Paid * 50) / 100;
    RA4_REQUIRE(ExpectedRefund > 0);

    // Flatten the factory. Losing the building already costs the player; ADR-0012
    // hands back half of what the dead queue had paid rather than confiscating it
    // silently on top of that.
    const HealthComp* H = F.World.GetHealth(Barracks);
    RA4_REQUIRE(H != nullptr);
    F.World.DebugDamage(Barracks, H->Max * 4);
    RunTicks(F.World, 1);

    RA4_EXPECT(!F.World.IsAlive(Barracks));
    RA4_EXPECT_EQ(F.World.GetPlayer(0).Credits, CreditsBefore + ExpectedRefund);
    // The refund is a fraction of money actually spent, so this can never be a way
    // to end up richer than before the order was given.
    RA4_EXPECT(F.World.GetPlayer(0).Credits < 10000);
}

RA4_TEST(FlowPayment, CancellingAfterHeavyInvestmentRefundsLessThanCancellingEarly)
{
    // The two tiers must actually differ, or the 25% threshold is decoration.
    const auto PaidAndRefundFor = [](int32_t TicksBeforeCancel) {
        FlowFixture F(10000);
        const EntityId Yard = F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
        Command Start = MakeCommand(CommandType::StartProduction, 0);
        Start.Primary = Yard;
        Start.Content = Ids::SovPower;
        Start.Slot = 0;
        F.World.ApplyCommand(Start);
        RunTicks(F.World, TicksBeforeCancel);

        const ProductionItem* Item = QueueHead(F.World, Yard);
        const int32_t Paid = (Item != nullptr) ? Item->PaidCredits : 0;
        const int32_t Before = F.World.GetPlayer(0).Credits;

        Command Cancel = MakeCommand(CommandType::CancelProduction, 0);
        Cancel.Primary = Yard;
        Cancel.Slot = 0;
        F.World.ApplyCommand(Cancel);
        return std::pair<int32_t, int32_t>{Paid, F.World.GetPlayer(0).Credits - Before};
    };

    const auto Early = PaidAndRefundFor(20);    // well under a quarter paid
    const auto Late = PaidAndRefundFor(140);    // well over a quarter paid
    RA4_REQUIRE(Early.first > 0 && Late.first > Early.first);

    RA4_EXPECT_EQ(Early.second, (Early.first * 90) / 100);
    RA4_EXPECT_EQ(Late.second, (Late.first * 60) / 100);
}

RA4_TEST(FlowPayment, MidFundingStateSurvivesSaveAndReload)
{
    FlowFixture F(10000);
    const EntityId Yard = F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);

    Command Start = MakeCommand(CommandType::StartProduction, 0);
    Start.Primary = Yard;
    Start.Content = Ids::SovPower;
    RA4_REQUIRE(F.World.ApplyCommand(Start).IsAccepted());
    RunTicks(F.World, 60);

    const ProductionItem* Before = QueueHead(F.World, Yard);
    RA4_REQUIRE(Before != nullptr);
    const FlowPaymentState SavedState = Before->State;
    const int32_t SavedPaid = Before->PaidCredits;
    const int32_t SavedProgress = Before->ProgressTicks;
    const int32_t SavedCost = Before->TotalCost;
    const uint64_t SavedChecksum = F.World.ComputeStateChecksum();

    ByteWriter W;
    F.World.Serialize(W);

    SimWorld Restored;
    ByteReader R(W.GetBuffer());
    RA4_REQUIRE(Restored.Deserialize(R, &F.Content));
    RA4_REQUIRE(!R.HasError());

    const ProductionItem* After = QueueHead(Restored, Yard);
    RA4_REQUIRE(After != nullptr);
    RA4_EXPECT(After->State == SavedState);
    RA4_EXPECT_EQ(After->PaidCredits, SavedPaid);
    RA4_EXPECT_EQ(After->ProgressTicks, SavedProgress);
    RA4_EXPECT_EQ(After->TotalCost, SavedCost);
    // Payment state feeds the checksum, so a reload that lost it would desync.
    RA4_EXPECT(Restored.ComputeStateChecksum() == SavedChecksum);
}

RA4_TEST(FlowPayment, ScarceCreditsGoToTheLowerEntityIndexAndNothingIsLost)
{
    // Two producers competing for less money than one tick's combined demand. That
    // is the only situation in which the allocation order is observable, and the
    // documented order is priority (all equal today) then entity index -- never the
    // order the collection loop happened to visit buildings in.
    FlowFixture F(7);
    const EntityId First = F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    const EntityId Second = F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(20, 20), true);
    RA4_REQUIRE(First.IsValid() && Second.IsValid());
    RA4_REQUIRE(First.Index != Second.Index);

    for (const EntityId Producer : {First, Second})
    {
        Command Start = MakeCommand(CommandType::StartProduction, 0);
        Start.Primary = Producer;
        Start.Content = Ids::SovPower;
        RA4_REQUIRE(F.World.ApplyCommand(Start).IsAccepted());
    }

    RunTicks(F.World, 1);

    const ProductionItem* A = QueueHead(F.World, First);
    const ProductionItem* B = QueueHead(F.World, Second);
    RA4_REQUIRE(A != nullptr && B != nullptr);

    // Seven credits cannot cover two five-credit slices, so one queue is served in
    // full and the other gets the remainder -- an even split would mean the order was
    // not being applied at all.
    const ProductionItem* Winner = (First.Index < Second.Index) ? A : B;
    const ProductionItem* Loser = (First.Index < Second.Index) ? B : A;
    RA4_EXPECT_EQ(Winner->PaidCredits, 5);
    RA4_EXPECT_EQ(Loser->PaidCredits, 2);

    // Every credit is accounted for: none evaporated and none was conjured.
    RA4_EXPECT_EQ(A->PaidCredits + B->PaidCredits + F.World.GetPlayer(0).Credits, 7);
    // The loser could not buy a whole slice, so it must say so rather than sitting
    // in an unexplained Funding state.
    RA4_EXPECT(Loser->State == FlowPaymentState::Starved);

    // Replaying the same scenario must award the credits the same way, or two peers
    // handed the identical command stream would diverge on who got paid.
    FlowFixture G(7);
    const EntityId GFirst = G.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    const EntityId GSecond = G.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(20, 20), true);
    for (const EntityId Producer : {GFirst, GSecond})
    {
        Command Start = MakeCommand(CommandType::StartProduction, 0);
        Start.Primary = Producer;
        Start.Content = Ids::SovPower;
        RA4_REQUIRE(G.World.ApplyCommand(Start).IsAccepted());
    }
    RunTicks(G.World, 1);
    const ProductionItem* GA = QueueHead(G.World, GFirst);
    const ProductionItem* GB = QueueHead(G.World, GSecond);
    RA4_REQUIRE(GA != nullptr && GB != nullptr);
    RA4_EXPECT_EQ(GA->PaidCredits, A->PaidCredits);
    RA4_EXPECT_EQ(GB->PaidCredits, B->PaidCredits);
}

RA4_TEST(FlowPayment, IdenticalCommandStreamsProduceIdenticalChecksums)
{
    const auto RunOne = [](uint64_t Seed) {
        FlowFixture F(3000, Seed);
        const EntityId Yard = F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
        const EntityId Second = F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(20, 20), true);

        for (const EntityId Producer : {Yard, Second})
        {
            Command Start = MakeCommand(CommandType::StartProduction, 0);
            Start.Primary = Producer;
            Start.Content = Ids::SovPower;
            F.World.ApplyCommand(Start);
        }
        RunTicks(F.World, 250);
        return F.World.ComputeStateChecksum();
    };

    // Same seed and same commands must land on the same state, including which of
    // two competing queues won the scarce credits.
    RA4_EXPECT(RunOne(777) == RunOne(777));
}

// Regression: the funding pass once collected candidates into an array sized by
// kMaxProductionQueueLength (9), which is a per-building queue-depth cap, not a
// bound on how many buildings a player can own. Every producer past the ninth was
// silently dropped -- its item never paid a credit and so never advanced, stalling
// forever with nothing shown to the player.
RA4_TEST(FlowPayment, EveryProducerIsFundedEvenBeyondTheQueueLengthCap)
{
    FlowFixture F(200000);

    // Comfortably more producers than kMaxProductionQueueLength.
    constexpr int32_t kProducerCount = 14;
    std::vector<EntityId> Barracks;
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(4, 4), true);
    for (int32_t N = 0; N < kProducerCount; ++N)
    {
        const EntityId B = F.World.SpawnBuilding(Ids::SovPower, 0, TileCoord(4 + N * 3, 10), true);
        RA4_REQUIRE(B.IsValid());
        const EntityId Bar = F.World.SpawnBuilding(Ids::SovBarracks, 0, TileCoord(4 + N * 3, 16), true);
        RA4_REQUIRE(Bar.IsValid());
        Barracks.push_back(Bar);
    }

    for (const EntityId Producer : Barracks)
    {
        Command Start = MakeCommand(CommandType::StartProduction, 0);
        Start.Primary = Producer;
        Start.Content = Ids::SovConscript;
        RA4_REQUIRE(F.World.ApplyCommand(Start).IsAccepted());
    }

    RunTicks(F.World, 20);

    // With a large treasury every single queue must be drawing credits. Before the
    // fix the last five sat at zero paid, zero progress, forever.
    for (size_t N = 0; N < Barracks.size(); ++N)
    {
        const ProductionItem* Item = QueueHead(F.World, Barracks[N]);
        if (Item == nullptr)
        {
            continue;   // already finished and popped, which is also fine
        }
        RA4_EXPECT(Item->PaidCredits > 0);
        RA4_EXPECT(Item->State != FlowPaymentState::Queued);
    }
}

// Selling is OwnershipChanged, which carries no queue refund: the sale price is
// already the compensation. Paying the destroyed-producer refund on top made selling
// a loaded factory strictly better than keeping it.
RA4_TEST(FlowPayment, SellingAProducerDoesNotAlsoPayTheDestroyedQueueRefund)
{
    FlowFixture F(10000);
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    F.World.SpawnBuilding(Ids::SovPower, 0, TileCoord(14, 10), true);
    const EntityId Barracks = F.World.SpawnBuilding(Ids::SovBarracks, 0, TileCoord(10, 14), true);
    RA4_REQUIRE(Barracks.IsValid());

    Command Start = MakeCommand(CommandType::StartProduction, 0);
    Start.Primary = Barracks;
    Start.Content = Ids::SovConscript;
    RA4_REQUIRE(F.World.ApplyCommand(Start).IsAccepted());
    RunTicks(F.World, 10);

    // Freeze funding so the arithmetic is exact.
    Command Pause = MakeCommand(CommandType::PauseProduction, 0);
    Pause.Primary = Barracks;
    Pause.Slot = 0;
    RA4_REQUIRE(F.World.ApplyCommand(Pause).IsAccepted());
    RunTicks(F.World, 2);

    const ProductionItem* Frozen = QueueHead(F.World, Barracks);
    RA4_REQUIRE(Frozen != nullptr);
    RA4_EXPECT(Frozen->PaidCredits > 0);

    const EntityDef* BarracksDef = F.Content.FindEntity(Ids::SovBarracks);
    RA4_REQUIRE(BarracksDef != nullptr);
    const int32_t SalePrice =
        (BarracksDef->Production.Cost * BarracksDef->Building.SellRefundPercent) / 100;

    const int32_t Before = F.World.GetPlayer(0).Credits;

    Command Sell = MakeCommand(CommandType::SellBuilding, 0);
    Sell.Primary = Barracks;
    RA4_REQUIRE(F.World.ApplyCommand(Sell).IsAccepted());
    RunTicks(F.World, 2);

    RA4_EXPECT(!F.World.IsAlive(Barracks));
    // Exactly the sale price, with nothing added for the queue that went with it.
    RA4_EXPECT_EQ(F.World.GetPlayer(0).Credits, Before + SalePrice);
}
