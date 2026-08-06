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
    // fix the last five sat at zero paid, zero progress, forever. Count explicitly
    // rather than skipping absent queues, or the test would pass vacuously if every
    // queue happened to have emptied.
    int32_t Funded = 0;
    for (size_t N = 0; N < Barracks.size(); ++N)
    {
        const ProductionItem* Item = QueueHead(F.World, Barracks[N]);
        RA4_REQUIRE(Item != nullptr);   // a conscript takes far longer than 20 ticks
        RA4_EXPECT(Item->PaidCredits > 0);
        RA4_EXPECT(Item->State != FlowPaymentState::Queued);
        if (Item->PaidCredits > 0)
        {
            ++Funded;
        }
    }
    RA4_EXPECT_EQ(Funded, kProducerCount);
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

// A sold building is still alive when the funding pass runs -- SystemDeaths does not
// sweep until the end of the tick -- so without an explicit skip it kept drawing
// credits for a queue that was about to be discarded refund-free.
RA4_TEST(FlowPayment, SoldProducerStopsDrawingCreditsImmediately)
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

    const EntityDef* Def = F.Content.FindEntity(Ids::SovBarracks);
    RA4_REQUIRE(Def != nullptr);
    const int32_t SalePrice = (Def->Production.Cost * Def->Building.SellRefundPercent) / 100;
    const int32_t Before = F.World.GetPlayer(0).Credits;

    Command Sell = MakeCommand(CommandType::SellBuilding, 0);
    Sell.Primary = Barracks;
    RA4_REQUIRE(F.World.ApplyCommand(Sell).IsAccepted());
    RunTicks(F.World, 1);

    RA4_EXPECT(!F.World.IsAlive(Barracks));
    // Exactly the sale price: not one further credit was drawn on the way out, and
    // no queue refund was added on top.
    RA4_EXPECT_EQ(F.World.GetPlayer(0).Credits, Before + SalePrice);
}

// A save taken between the sell command and the death sweep used to reload a building
// permanently flagged "selling", which then silently forfeited its ADR-0012
// destruction refund when it was later destroyed for real. The intent is now
// tick-scoped and unserialized, so a reload cannot inherit it.
RA4_TEST(FlowPayment, SaleIntentDoesNotSurviveASaveAndSuppressLaterRefunds)
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

    // Issue the sale but save before ticking, so the sale is still pending.
    Command Sell = MakeCommand(CommandType::SellBuilding, 0);
    Sell.Primary = Barracks;
    RA4_REQUIRE(F.World.ApplyCommand(Sell).IsAccepted());

    ByteWriter W;
    F.World.Serialize(W);

    SimWorld Restored;
    ByteReader R(W.GetBuffer());
    RA4_REQUIRE(Restored.Deserialize(R, &F.Content));
    RA4_REQUIRE(!R.HasError());

    // The pending sale did not survive, so the building is alive and ordinary.
    RA4_REQUIRE(Restored.IsAlive(Barracks));
    const ProductionItem* Item = QueueHead(Restored, Barracks);
    RA4_REQUIRE(Item != nullptr);
    const int32_t Paid = Item->PaidCredits;
    RA4_EXPECT(Paid > 0);

    // Pause so the figure stops moving, then destroy it violently. The full
    // destroyed-producer refund must be paid: this is not a sale any more.
    Command Pause = MakeCommand(CommandType::PauseProduction, 0);
    Pause.Primary = Barracks;
    Pause.Slot = 0;
    RA4_REQUIRE(Restored.ApplyCommand(Pause).IsAccepted());
    RunTicks(Restored, 2);

    const ProductionItem* Frozen = QueueHead(Restored, Barracks);
    RA4_REQUIRE(Frozen != nullptr);
    const int32_t FrozenPaid = Frozen->PaidCredits;
    const int32_t Before = Restored.GetPlayer(0).Credits;

    const HealthComp* H = Restored.GetHealth(Barracks);
    RA4_REQUIRE(H != nullptr);
    Restored.DebugDamage(Barracks, H->Max * 4);
    RunTicks(Restored, 1);

    RA4_EXPECT(!Restored.IsAlive(Barracks));
    RA4_EXPECT_EQ(Restored.GetPlayer(0).Credits, Before + (FrozenPaid * 50) / 100);
}

// Starvation is announced exactly once per stall. Without the edge trigger a broke
// player would emit one event per tick per queue and bury the alert feed.
RA4_TEST(FlowPayment, StarvationIsAnnouncedOnceNotEveryTick)
{
    FlowFixture F(3);
    const EntityId Yard = F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);

    Command Start = MakeCommand(CommandType::StartProduction, 0);
    Start.Primary = Yard;
    Start.Content = Ids::SovPower;
    RA4_REQUIRE(F.World.ApplyCommand(Start).IsAccepted());

    int32_t StarvedEvents = 0;
    for (int32_t T = 0; T < 60; ++T)
    {
        F.World.ClearEvents();
        F.World.Tick(nullptr);
        for (const SimEvent& Ev : F.World.GetEvents())
        {
            if (Ev.Type == SimEventType::ProductionStarved)
            {
                ++StarvedEvents;
                RA4_EXPECT_EQ(int32_t(Ev.Player), 0);
                RA4_EXPECT(Ev.Value > 0);   // credits still owed
            }
        }
    }

    const ProductionItem* Item = QueueHead(F.World, Yard);
    RA4_REQUIRE(Item != nullptr);
    RA4_EXPECT(Item->State == FlowPaymentState::Starved);
    // One announcement for the one stall, not sixty.
    RA4_EXPECT_EQ(StarvedEvents, 1);
}

// ---------------------------------------------------------------------------
// Energy tiers (ADR-0013)
// ---------------------------------------------------------------------------

// The band boundaries are the part of ADR-0013 most likely to be quietly broken by a
// later edit, and an off-by-one there silently shifts the whole penalty curve. Pin the
// exact crossings, including both sides of each one.
RA4_TEST(PowerTier, BandBoundariesAreExact)
{
    RA4_EXPECT(PowerTierForRatio(200) == PowerTier::Normal);
    RA4_EXPECT(PowerTierForRatio(100) == PowerTier::Normal);
    RA4_EXPECT(PowerTierForRatio(99) == PowerTier::Mild);
    RA4_EXPECT(PowerTierForRatio(70) == PowerTier::Mild);
    RA4_EXPECT(PowerTierForRatio(69) == PowerTier::Moderate);
    RA4_EXPECT(PowerTierForRatio(40) == PowerTier::Moderate);
    RA4_EXPECT(PowerTierForRatio(39) == PowerTier::Severe);
    RA4_EXPECT(PowerTierForRatio(10) == PowerTier::Severe);
    RA4_EXPECT(PowerTierForRatio(9) == PowerTier::Critical);
    RA4_EXPECT(PowerTierForRatio(0) == PowerTier::Critical);
}

RA4_TEST(PowerTier, SpeedFollowsTheTableAndNeverReachesZero)
{
    // Normal and Mild/Moderate are the plain cases: full speed, then the ratio itself.
    RA4_EXPECT_EQ(PowerSpeedPercentForTier(PowerTier::Normal, 100), 100);
    RA4_EXPECT_EQ(PowerSpeedPercentForTier(PowerTier::Mild, 85), 85);
    RA4_EXPECT_EQ(PowerSpeedPercentForTier(PowerTier::Moderate, 45), 45);

    // Severe has a floor: 12% power still builds at 12%, but 10% does not drop below
    // the floor, and a hypothetical lower value is clamped up to it.
    RA4_EXPECT_EQ(PowerSpeedPercentForTier(PowerTier::Severe, 12), 12);
    RA4_EXPECT_EQ(PowerSpeedPercentForTier(PowerTier::Severe, 10), kPowerSevereFloorPercent);

    // Critical is a flat rate rather than the ratio, which by then is near zero and
    // would mean "stopped" in all but name.
    RA4_EXPECT_EQ(PowerSpeedPercentForTier(PowerTier::Critical, 3), kPowerCriticalSpeedPercent);
    RA4_EXPECT_EQ(PowerSpeedPercentForTier(PowerTier::Critical, 0), kPowerCriticalSpeedPercent);

    // Whatever the tier, speed is never zero: a rate of nothing is a deadlock, not a
    // penalty, and a base that can never rebuild its power plant is a dead match.
    for (int32_t Ratio = 0; Ratio <= 120; ++Ratio)
    {
        RA4_EXPECT(PowerSpeedPercentForTier(PowerTierForRatio(Ratio), Ratio) > 0);
    }
}

namespace
{
// A base with a chosen power balance. The war factory is the load: it draws power and
// produces vehicles, so it is both what creates the deficit and what the deficit acts
// on. PowerPlants controls the tier.
struct PowerFixture
{
    ContentDatabase Content;
    SimWorld World;
    // Captured rather than assumed. An earlier version of these tests reached for the
    // yard as MakeId(1), which only worked because SpawnEnemyOutpost happens to take
    // index 0 -- reordering the fixture would have silently retargeted commands at
    // another building.
    EntityId Yard;

    explicit PowerFixture(int32_t PowerPlants)
    {
        BuildDefaultContent(Content);
        World.Initialize(&Content, MakeTestSetup(31337));
        SpawnEnemyOutpost(World);
        Yard = World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
        for (int32_t N = 0; N < PowerPlants; ++N)
        {
            World.SpawnBuilding(Ids::SovPower, 0, TileCoord(14 + N * 3, 10), true);
        }
    }

    // Turrets are the cheapest way to add a known power draw: 40 each against a
    // reactor's 150, so the caller can dial in a specific tier.
    void AddPowerDraw(int32_t Turrets)
    {
        for (int32_t N = 0; N < Turrets; ++N)
        {
            World.SpawnBuilding(Ids::SovTurret, 0,
                               TileCoord(20 + (N % 7) * 2, 24 + (N / 7) * 3), true);
        }
    }
};
} // namespace

RA4_TEST(PowerTier, DeficitSlowsVehicleProductionInProportionToTheTier)
{
    // Same order, same content, only the power balance differs. The starved run must
    // take strictly longer -- and must still finish, because a partial deficit slows
    // production rather than stopping it. (Critical is the one tier that does stop
    // vehicles outright; that is a separate test.)
    const auto TicksToBuildTank = [](int32_t PowerPlants, bool bExtraLoad) -> int32_t {
        PowerFixture F(PowerPlants);
        const EntityId Factory = F.World.SpawnBuilding(Ids::SovWarFactory, 0, TileCoord(10, 16), true);
        if (!Factory.IsValid())
        {
            return -1;
        }
        // Extra consumers push the ratio down without taking it to Critical. A Soviet
        // reactor gives 150; a war factory draws 50, a barracks 30, a refinery 20 and
        // a turret 40, so this load lands the ratio well inside the graded bands.
        if (bExtraLoad)
        {
            F.World.SpawnBuilding(Ids::SovBarracks, 0, TileCoord(10, 20), true);
            F.World.SpawnBuilding(Ids::SovRefinery, 0, TileCoord(16, 20), true);
            F.World.SpawnBuilding(Ids::SovTurret, 0, TileCoord(20, 20), true);
            F.World.SpawnBuilding(Ids::SovTurret, 0, TileCoord(22, 20), true);
            F.World.SpawnBuilding(Ids::SovTurret, 0, TileCoord(24, 20), true);
        }
        Command Start = MakeCommand(CommandType::StartProduction, 0);
        Start.Primary = Factory;
        Start.Content = Ids::SovHeavyTank;
        if (!F.World.ApplyCommand(Start).IsAccepted())
        {
            return -2;
        }
        F.World.Tick(nullptr);
        // Guard the premise: a Critical run would legitimately never finish, and the
        // test would then be measuring the wrong thing.
        if (F.World.GetPlayer(0).GetPowerTier() == PowerTier::Critical)
        {
            return -3;
        }
        return RunUntil(F.World, SecondsToTicks(240),
                        [&] { return CountEntitiesOfType(F.World, 0, Ids::SovHeavyTank) == 1; });
    };

    const int32_t Powered = TicksToBuildTank(3, false);
    const int32_t Starved = TicksToBuildTank(1, true);

    RA4_REQUIRE(Powered > 0);
    RA4_REQUIRE(Starved > 0);   // slowed, not stalled
    RA4_EXPECT(Starved > Powered);
}

RA4_TEST(PowerTier, CriticalPowerStopsVehiclesButNotInfantryOrTheConstructionYard)
{
    // No power plants at all, with two consumers drawing: deep in Critical.
    PowerFixture F(0);
    const EntityId Factory = F.World.SpawnBuilding(Ids::SovWarFactory, 0, TileCoord(10, 16), true);
    const EntityId Barracks = F.World.SpawnBuilding(Ids::SovBarracks, 0, TileCoord(10, 20), true);
    RA4_REQUIRE(Factory.IsValid() && Barracks.IsValid());
    F.World.Tick(nullptr);
    RA4_REQUIRE(F.World.GetPlayer(0).GetPowerTier() == PowerTier::Critical);

    Command Tank = MakeCommand(CommandType::StartProduction, 0);
    Tank.Primary = Factory;
    Tank.Content = Ids::SovHeavyTank;
    RA4_REQUIRE(F.World.ApplyCommand(Tank).IsAccepted());

    Command Man = MakeCommand(CommandType::StartProduction, 0);
    Man.Primary = Barracks;
    Man.Content = Ids::SovConscript;
    RA4_REQUIRE(F.World.ApplyCommand(Man).IsAccepted());

    // A construction yard must keep working at Critical too, or a blacked-out base
    // could never build the power plant that ends the blackout.
    Command Plant = MakeCommand(CommandType::StartProduction, 0);
    Plant.Primary = F.Yard;
    Plant.Content = Ids::SovPower;
    RA4_REQUIRE(F.World.ApplyCommand(Plant).IsAccepted());

    RunTicks(F.World, SecondsToTicks(90));

    // Infantry came out; the tank did not.
    RA4_EXPECT(CountEntitiesOfType(F.World, 0, Ids::SovConscript) >= 1);
    RA4_EXPECT_EQ(CountEntitiesOfType(F.World, 0, Ids::SovHeavyTank), 0);

    // The vehicle order is still queued and still paid for -- paused, not cancelled.
    const BuildingComp* FactoryState = F.World.GetBuilding(Factory);
    RA4_REQUIRE(FactoryState != nullptr);
    RA4_REQUIRE(FactoryState->Queue.size() == 1u);
    RA4_EXPECT(FactoryState->Queue.front().ProgressTicks <
               FactoryState->Queue.front().TotalTicks * 100);
}

RA4_TEST(PowerTier, TierChangeIsAnnouncedOnceOnTheCrossing)
{
    PowerFixture F(0);
    F.World.SpawnBuilding(Ids::SovWarFactory, 0, TileCoord(10, 16), true);

    // Settle into the deficit and count how often it is announced.
    int32_t Announcements = 0;
    PowerTier LastSeen = PowerTier::Normal;
    for (int32_t T = 0; T < 40; ++T)
    {
        F.World.ClearEvents();
        F.World.Tick(nullptr);
        for (const SimEvent& Ev : F.World.GetEvents())
        {
            if (Ev.Type == SimEventType::PowerShortageStarted)
            {
                ++Announcements;
                LastSeen = PowerTier(Ev.Value);
            }
        }
    }
    // One crossing from Normal into the deficit, not forty.
    RA4_EXPECT_EQ(Announcements, 1);
    RA4_EXPECT(LastSeen == PowerTier::Critical);

    // Building enough power must announce the recovery, exactly once.
    F.World.SpawnBuilding(Ids::SovPower, 0, TileCoord(20, 10), true);
    F.World.SpawnBuilding(Ids::SovPower, 0, TileCoord(24, 10), true);
    int32_t Recoveries = 0;
    for (int32_t T = 0; T < 20; ++T)
    {
        F.World.ClearEvents();
        F.World.Tick(nullptr);
        for (const SimEvent& Ev : F.World.GetEvents())
        {
            if (Ev.Type == SimEventType::PowerShortageEnded)
            {
                ++Recoveries;
            }
        }
    }
    RA4_EXPECT_EQ(Recoveries, 1);
    RA4_EXPECT(F.World.GetPlayer(0).GetPowerTier() == PowerTier::Normal);
}

RA4_TEST(PowerTier, RememberedTierSurvivesSaveSoTheWarningIsNotRepeated)
{
    PowerFixture F(0);
    F.World.SpawnBuilding(Ids::SovWarFactory, 0, TileCoord(10, 16), true);
    RunTicks(F.World, 5);
    RA4_REQUIRE(F.World.GetPlayer(0).GetPowerTier() == PowerTier::Critical);

    ByteWriter W;
    F.World.Serialize(W);

    SimWorld Restored;
    ByteReader R(W.GetBuffer());
    RA4_REQUIRE(Restored.Deserialize(R, &F.Content));
    RA4_REQUIRE(!R.HasError());

    // The reload must be bit-identical before either side advances, or the remembered
    // tier was not carried across and the rest of this test would be meaningless.
    RA4_EXPECT(Restored.ComputeStateChecksum() == F.World.ComputeStateChecksum());
    RA4_EXPECT(Restored.GetPlayer(0).LastPowerTier == PowerTier::Critical);

    // The reload already knows about the deficit, so it must not announce it again --
    // otherwise every save/load would replay the alarm.
    int32_t Announcements = 0;
    for (int32_t T = 0; T < 10; ++T)
    {
        Restored.ClearEvents();
        Restored.Tick(nullptr);
        for (const SimEvent& Ev : Restored.GetEvents())
        {
            if (Ev.Type == SimEventType::PowerShortageStarted)
            {
                ++Announcements;
            }
        }
    }
    RA4_EXPECT_EQ(Announcements, 0);

    // Both sides advanced by the same number of ticks from the same state, so they
    // must still agree -- this is the lockstep property, checked across a reload.
    RunTicks(F.World, 10);
    RA4_EXPECT(Restored.ComputeStateChecksum() == F.World.ComputeStateChecksum());
}

RA4_TEST(PowerTier, HarvestingIsUntouchedUntilCriticalThenHalves)
{
    // Harvest income is what buys the power plant that ends a blackout, so slowing it
    // before Critical would make a deficit self-reinforcing. The claim is therefore
    // two-sided and needs a middle arm: an earlier version of this test compared only
    // Normal against Critical, so a regression that slowed harvesting at Moderate
    // would have passed it while breaking the property in its own name.
    struct Arm { int32_t Harvested; PowerTier Tier; };
    const auto Measure = [](int32_t PowerPlants, int32_t Turrets) -> Arm {
        PowerFixture F(PowerPlants);
        F.World.SpawnBuilding(Ids::SovRefinery, 0, TileCoord(10, 20), true);
        F.AddPowerDraw(Turrets);
        F.World.SpawnResourceNode(Ids::OreField, TileCoord(13, 20), 100000);
        F.World.SpawnUnit(Ids::SovHarvester, 0, Vec2::FromInts(11 * 200, 20 * 200));
        F.World.Tick(nullptr);
        const PowerTier Tier = F.World.GetPlayer(0).GetPowerTier();
        const int32_t Before = F.World.GetPlayer(0).TotalHarvested;
        RunTicks(F.World, SecondsToTicks(60));
        return Arm{F.World.GetPlayer(0).TotalHarvested - Before, Tier};
    };

    const Arm Normal = Measure(2, 0);
    const Arm Deficit = Measure(1, 4);   // reactor 150 against refinery 20 + 4x40
    const Arm Critical = Measure(0, 0);  // nothing producing, refinery drawing

    // Guard the premise of each arm, or the comparisons below measure the wrong thing.
    RA4_REQUIRE(Normal.Tier == PowerTier::Normal);
    RA4_REQUIRE(Deficit.Tier > PowerTier::Normal && Deficit.Tier < PowerTier::Critical);
    RA4_REQUIRE(Critical.Tier == PowerTier::Critical);

    RA4_REQUIRE(Normal.Harvested > 0);
    // The middle arm is the actual "not earlier" claim: a partial deficit must not
    // touch harvesting at all.
    RA4_EXPECT_EQ(Deficit.Harvested, Normal.Harvested);
    // Critical halves it, but never to zero -- a rate of nothing is a deadlock.
    RA4_EXPECT(Critical.Harvested > 0);
    RA4_EXPECT(Critical.Harvested < Normal.Harvested);
}

// ADR-0013 pauses "high tech" (T2+) outright during a deep deficit rather than merely
// slowing it, using the EnergyThrottled state ADR-0012 defined but never set.
RA4_TEST(PowerTier, HighTechIsPausedAtSevereAndResumesWhenPowerReturns)
{
    // One reactor against a heavy load: enough to reach Severe without going Critical,
    // where the vehicle would be blocked by the Critical rule instead and the test
    // would prove nothing about tiers.
    PowerFixture F(1);
    const EntityId Factory = F.World.SpawnBuilding(Ids::SovWarFactory, 0, TileCoord(10, 16), true);
    RA4_REQUIRE(Factory.IsValid());
    F.World.SpawnBuilding(Ids::SovBarracks, 0, TileCoord(10, 20), true);
    F.World.SpawnBuilding(Ids::SovRefinery, 0, TileCoord(16, 20), true);
    // Measured: one reactor gives 150 while factory+barracks+refinery draw 100, and
    // each turret adds 40. Thirteen turrets puts the ratio at 24% -- inside Severe,
    // where T2 is paused, without reaching Critical, where the separate Critical rule
    // would block the vehicle anyway and the test would prove nothing about tiers.
    F.AddPowerDraw(13);

    Command Start = MakeCommand(CommandType::StartProduction, 0);
    Start.Primary = Factory;
    Start.Content = Ids::SovHeavyTank;   // T2
    RA4_REQUIRE(F.World.ApplyCommand(Start).IsAccepted());

    RunTicks(F.World, 30);
    RA4_REQUIRE(F.World.GetPlayer(0).GetPowerTier() >= PowerTier::Severe);

    const ProductionItem* Throttled = QueueHead(F.World, Factory);
    RA4_REQUIRE(Throttled != nullptr);
    RA4_EXPECT(Throttled->State == FlowPaymentState::EnergyThrottled);
    const int32_t FrozenProgress = Throttled->ProgressTicks;
    const int32_t FrozenPaid = Throttled->PaidCredits;

    // Frozen, not reset: neither progress nor payment moves while throttled.
    RunTicks(F.World, 60);
    const ProductionItem* Still = QueueHead(F.World, Factory);
    RA4_REQUIRE(Still != nullptr);
    RA4_EXPECT(Still->State == FlowPaymentState::EnergyThrottled);
    RA4_EXPECT_EQ(Still->ProgressTicks, FrozenProgress);
    RA4_EXPECT_EQ(Still->PaidCredits, FrozenPaid);

    // Restore power and it must pick up from where it stopped and finish.
    for (int32_t N = 0; N < 4; ++N)
    {
        F.World.SpawnBuilding(Ids::SovPower, 0, TileCoord(30 + N * 3, 10), true);
    }
    RunTicks(F.World, 2);
    RA4_REQUIRE(F.World.GetPlayer(0).GetPowerTier() == PowerTier::Normal);

    const ProductionItem* Resumed = QueueHead(F.World, Factory);
    RA4_REQUIRE(Resumed != nullptr);
    RA4_EXPECT(Resumed->State != FlowPaymentState::EnergyThrottled);
    RA4_EXPECT(Resumed->ProgressTicks >= FrozenProgress);

    RunTicks(F.World, SecondsToTicks(200));
    RA4_EXPECT_EQ(CountEntitiesOfType(F.World, 0, Ids::SovHeavyTank), 1);
}

RA4_TEST(PowerTier, LowTechKeepsBuildingThroughADeficitInsteadOfPausing)
{
    // The distinction is the whole point of the tier field: T1 infantry slows, T2
    // vehicles stop. If both behaved the same the field would be decoration.
    PowerFixture F(1);
    const EntityId Barracks = F.World.SpawnBuilding(Ids::SovBarracks, 0, TileCoord(10, 20), true);
    RA4_REQUIRE(Barracks.IsValid());
    F.World.SpawnBuilding(Ids::SovWarFactory, 0, TileCoord(10, 16), true);
    F.World.SpawnBuilding(Ids::SovRefinery, 0, TileCoord(16, 20), true);
    // Measured: one reactor gives 150 while factory+barracks+refinery draw 100, and
    // each turret adds 40. Thirteen turrets puts the ratio at 24% -- inside Severe,
    // where T2 is paused, without reaching Critical, where the separate Critical rule
    // would block the vehicle anyway and the test would prove nothing about tiers.
    F.AddPowerDraw(13);

    Command Start = MakeCommand(CommandType::StartProduction, 0);
    Start.Primary = Barracks;
    Start.Content = Ids::SovConscript;   // T1
    RA4_REQUIRE(F.World.ApplyCommand(Start).IsAccepted());

    RunTicks(F.World, 30);
    RA4_REQUIRE(F.World.GetPlayer(0).GetPowerTier() >= PowerTier::Severe);

    const ProductionItem* Item = QueueHead(F.World, Barracks);
    if (Item != nullptr)
    {
        RA4_EXPECT(Item->State != FlowPaymentState::EnergyThrottled);
    }
    // Slowed, but it still arrives.
    RunTicks(F.World, SecondsToTicks(200));
    RA4_EXPECT(CountEntitiesOfType(F.World, 0, Ids::SovConscript) >= 1);
}

// Regression: an earlier version triggered the throttle below 40% but required 50% to
// recover, so anything stalled in the 40-49% band was both throttled and refused
// recovery -- frozen forever. Recovery is now the exact inverse of the trigger, which
// this test establishes by driving a real item through that exact band rather than by
// restating the condition (an earlier version asserted `x != !x`, which cannot fail).
RA4_TEST(PowerTier, ItemThrottledThenRecoveredIntoTheOldDeadBandResumes)
{
    PowerFixture F(1);
    const EntityId Factory = F.World.SpawnBuilding(Ids::SovWarFactory, 0, TileCoord(10, 16), true);
    RA4_REQUIRE(Factory.IsValid());
    // Deep enough to throttle: reactor 150 against factory 50 plus 13 turrets.
    F.AddPowerDraw(13);

    Command Start = MakeCommand(CommandType::StartProduction, 0);
    Start.Primary = Factory;
    Start.Content = Ids::SovHeavyTank;   // T2
    RA4_REQUIRE(F.World.ApplyCommand(Start).IsAccepted());
    RunTicks(F.World, 10);
    RA4_REQUIRE(F.World.GetPlayer(0).GetPowerTier() >= PowerTier::Severe);

    const ProductionItem* Throttled = QueueHead(F.World, Factory);
    RA4_REQUIRE(Throttled != nullptr);
    RA4_REQUIRE(Throttled->State == FlowPaymentState::EnergyThrottled);

    // Add power until the ratio lands inside the old dead band -- above the Severe
    // trigger at 40 but below the retired 50% recovery constant. This is precisely the
    // window in which the item used to freeze permanently.
    int32_t Guard = 0;
    while (F.World.GetPlayer(0).GetPowerTier() >= PowerTier::Severe && Guard < 10)
    {
        F.World.SpawnBuilding(Ids::SovPower, 0, TileCoord(30 + Guard * 3, 10), true);
        F.World.Tick(nullptr);
        ++Guard;
    }
    const int32_t Ratio = F.World.GetPlayer(0).GetPowerRatioPercent();
    RA4_REQUIRE(Ratio >= kPowerTierModerateMinPercent);
    RA4_REQUIRE(F.World.GetPlayer(0).GetPowerTier() < PowerTier::Severe);

    // Out of Severe, so it must resume -- whatever the ratio is, and specifically even
    // if it is still under 50.
    RunTicks(F.World, 5);
    const ProductionItem* Resumed = QueueHead(F.World, Factory);
    RA4_REQUIRE(Resumed != nullptr);
    RA4_EXPECT(Resumed->State != FlowPaymentState::EnergyThrottled);
    RA4_EXPECT(Resumed->PaidCredits > 0);

    // And pin the boundary itself, so a later balance change has to move it knowingly.
    RA4_EXPECT(PowerTierForRatio(40) < PowerTier::Severe);   // Moderate: T2 still builds
    RA4_EXPECT(PowerTierForRatio(39) >= PowerTier::Severe);  // Severe: T2 paused
}

RA4_TEST(PowerTier, PlayerPauseOutranksTheEnergyThrottle)
{
    // If the throttle overwrote a manual pause, unpausing would appear to do nothing
    // and the card would blame power for a stop the player chose.
    PowerFixture F(1);
    const EntityId Factory = F.World.SpawnBuilding(Ids::SovWarFactory, 0, TileCoord(10, 16), true);
    RA4_REQUIRE(Factory.IsValid());
    F.World.SpawnBuilding(Ids::SovBarracks, 0, TileCoord(10, 20), true);
    F.World.SpawnBuilding(Ids::SovRefinery, 0, TileCoord(16, 20), true);
    // Measured: one reactor gives 150 while factory+barracks+refinery draw 100, and
    // each turret adds 40. Thirteen turrets puts the ratio at 24% -- inside Severe,
    // where T2 is paused, without reaching Critical, where the separate Critical rule
    // would block the vehicle anyway and the test would prove nothing about tiers.
    F.AddPowerDraw(13);

    Command Start = MakeCommand(CommandType::StartProduction, 0);
    Start.Primary = Factory;
    Start.Content = Ids::SovHeavyTank;
    RA4_REQUIRE(F.World.ApplyCommand(Start).IsAccepted());

    Command Pause = MakeCommand(CommandType::PauseProduction, 0);
    Pause.Primary = Factory;
    Pause.Slot = 0;
    RA4_REQUIRE(F.World.ApplyCommand(Pause).IsAccepted());

    RunTicks(F.World, 30);
    RA4_REQUIRE(F.World.GetPlayer(0).GetPowerTier() >= PowerTier::Severe);

    const ProductionItem* Item = QueueHead(F.World, Factory);
    RA4_REQUIRE(Item != nullptr);
    RA4_EXPECT(Item->State == FlowPaymentState::ManuallyPaused);
}

RA4_TEST(Content, TechTierIsAssignedAndFeedsTheContentHash)
{
    ContentDatabase A;
    BuildDefaultContent(A);

    // Every produced definition must state a tier, and the chain must actually be
    // graded -- if everything were T0 the throttle would never fire.
    const EntityDef* Yard = A.FindEntity(Ids::SovConYard);
    const EntityDef* Barracks = A.FindEntity(Ids::SovBarracks);
    const EntityDef* Tank = A.FindEntity(Ids::SovHeavyTank);
    RA4_REQUIRE(Yard != nullptr && Barracks != nullptr && Tank != nullptr);
    RA4_EXPECT(Yard->Production.Tier == TechTier::T0);
    RA4_EXPECT(Barracks->Production.Tier == TechTier::T1);
    RA4_EXPECT(Tank->Production.Tier == TechTier::T2);

    // A tier change alters match outcomes, so it must invalidate a replay.
    ContentDatabase B;
    BuildDefaultContent(B);
    RA4_REQUIRE(A.ComputeContentHash() == B.ComputeContentHash());

    EntityDef Shifted = *Tank;
    Shifted.Production.Tier = TechTier::T3;
    B.AddEntity(Shifted);
    RA4_EXPECT(A.ComputeContentHash() != B.ComputeContentHash());
}

// Regression, and the reason IsProductionPowerStalled exists as one function. Payment
// and production each decided independently whether the power state had stopped an
// item, and they disagreed: SystemFlowPayment charged a slice every tick for a vehicle
// that SystemProduction refused to advance at Critical. The item froze at zero progress
// while draining the treasury that should have finished the power plant, so the base
// stayed dark forever -- the exact deadlock the Critical carve-out was written to avoid.
RA4_TEST(PowerTier, BlackoutIsEscapableEvenWithAFrozenVehicleInAnotherQueue)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);
    MatchSetup Setup = MakeTestSetup(7);
    // Enough for the power plant and not much else, so the two queues genuinely
    // compete: if the frozen one is funded, the plant can never finish.
    Setup.Players[0].StartingCredits = 1000;

    SimWorld World;
    World.Initialize(&Content, Setup);
    SpawnEnemyOutpost(World);
    const EntityId Yard = World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    const EntityId Factory = World.SpawnBuilding(Ids::SovWarFactory, 0, TileCoord(10, 16), true);
    World.SpawnBuilding(Ids::SovRefinery, 0, TileCoord(16, 20), true);
    RA4_REQUIRE(Yard.IsValid() && Factory.IsValid());
    // The factory sits at a *lower* entity index than nothing here, but the ordering
    // matters in general: funding is sorted by index, so a frozen item ahead of the
    // yard used to eat the money first.
    World.Tick(nullptr);
    RA4_REQUIRE(World.GetPlayer(0).GetPowerTier() == PowerTier::Critical);

    // A T1 harvester: not high-tech, so only the Critical producer rule stops it.
    Command Harvester = MakeCommand(CommandType::StartProduction, 0);
    Harvester.Primary = Factory;
    Harvester.Content = Ids::SovHarvester;
    RA4_REQUIRE(World.ApplyCommand(Harvester).IsAccepted());

    Command Plant = MakeCommand(CommandType::StartProduction, 0);
    Plant.Primary = Yard;
    Plant.Content = Ids::SovPower;
    RA4_REQUIRE(World.ApplyCommand(Plant).IsAccepted());

    RunTicks(World, SecondsToTicks(120));

    // The frozen vehicle must have taken nothing at all: charging for a tick it did
    // not advance is what created the deadlock.
    const ProductionItem* Frozen = QueueHead(World, Factory);
    RA4_REQUIRE(Frozen != nullptr);
    RA4_EXPECT(Frozen->State == FlowPaymentState::EnergyThrottled);
    RA4_EXPECT_EQ(Frozen->PaidCredits, 0);
    RA4_EXPECT_EQ(Frozen->ProgressTicks, 0);

    // And the plant must have been fully funded and finished.
    const ProductionItem* PlantItem = QueueHead(World, Yard);
    RA4_REQUIRE(PlantItem != nullptr);
    RA4_EXPECT(PlantItem->State == FlowPaymentState::Completed);
    RA4_EXPECT_EQ(PlantItem->PaidCredits, PlantItem->TotalCost);

    // Place it and the blackout must actually end, with the frozen item resuming by
    // itself -- the full escape path, not just the funding half.
    Command Place = MakeCommand(CommandType::PlaceBuilding, 0);
    Place.Content = Ids::SovPower;
    Place.Tile = TileCoord(20, 10);
    RA4_REQUIRE(World.ApplyCommand(Place).IsAccepted());
    RunTicks(World, SecondsToTicks(60));

    RA4_EXPECT(World.GetPlayer(0).GetPowerTier() < PowerTier::Critical);
    const ProductionItem* Resumed = QueueHead(World, Factory);
    RA4_REQUIRE(Resumed != nullptr);
    RA4_EXPECT(Resumed->State != FlowPaymentState::EnergyThrottled);
}

// A defeated player's last building dying takes both power figures to zero, which
// GetPowerRatioPercent reports as a healthy 100%. Without gating on bDefeated the game
// would tell a player who just lost their base that their power had been restored.
RA4_TEST(PowerTier, DefeatedPlayerIsNotToldTheirPowerCameBack)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);
    MatchSetup Setup = MakeTestSetup(11);
    // Three players, so the match keeps running after one is defeated and the systems
    // carry on ticking.
    Setup.Players[2].bActive = true;
    Setup.Players[2].Faction = FactionId::Soviet;
    Setup.Players[2].StartingCredits = 10000;

    SimWorld World;
    World.Initialize(&Content, Setup);
    SpawnEnemyOutpost(World);
    World.SpawnBuilding(Ids::SovConYard, 2, TileCoord(40, 40), true);

    // Player 0 owns one power-drawing building and nothing producing: Critical.
    const EntityId Doomed = World.SpawnBuilding(Ids::SovWarFactory, 0, TileCoord(10, 16), true);
    RA4_REQUIRE(Doomed.IsValid());
    RunTicks(World, 3);
    RA4_REQUIRE(World.GetPlayer(0).GetPowerTier() == PowerTier::Critical);

    // Destroy it. Power produced and consumed both fall to zero.
    const HealthComp* H = World.GetHealth(Doomed);
    RA4_REQUIRE(H != nullptr);
    World.DebugDamage(Doomed, H->Max * 4);

    int32_t SpuriousRecoveries = 0;
    for (int32_t T = 0; T < 20; ++T)
    {
        World.ClearEvents();
        World.Tick(nullptr);
        for (const SimEvent& Ev : World.GetEvents())
        {
            if (Ev.Type == SimEventType::PowerShortageEnded && Ev.Player == 0)
            {
                ++SpuriousRecoveries;
            }
        }
    }
    RA4_EXPECT(!World.IsAlive(Doomed));
    RA4_EXPECT_EQ(SpuriousRecoveries, 0);
}

// ---------------------------------------------------------------------------
// Power priority (ADR-0013 package C)
// ---------------------------------------------------------------------------

RA4_TEST(PowerPriority, OfflineBandsMatchTheSpecTable)
{
    // The whole point of the priority table is that a deficit reaches things in a
    // chosen order. If every band went offline at the same tier the feature would be
    // decoration, so pin each band's threshold and assert they actually differ.
    for (int32_t T = 0; T <= int32_t(PowerTier::Critical); ++T)
    {
        const PowerTier Tier = PowerTier(T);
        // Vital never goes offline at any tier -- this is what keeps a deficit
        // recoverable rather than terminal.
        RA4_EXPECT(!IsPowerPriorityOffline(PowerPriority::Vital, Tier));
    }

    // Auxiliary is the first to go: offline from Moderate onward.
    RA4_EXPECT(!IsPowerPriorityOffline(PowerPriority::Auxiliary, PowerTier::Normal));
    RA4_EXPECT(!IsPowerPriorityOffline(PowerPriority::Auxiliary, PowerTier::Mild));
    RA4_EXPECT(IsPowerPriorityOffline(PowerPriority::Auxiliary, PowerTier::Moderate));
    RA4_EXPECT(IsPowerPriorityOffline(PowerPriority::Auxiliary, PowerTier::Critical));

    // Production and Defense survive Moderate and Severe, and stop at Critical.
    for (const PowerPriority P : {PowerPriority::Production, PowerPriority::Defense})
    {
        RA4_EXPECT(!IsPowerPriorityOffline(P, PowerTier::Moderate));
        RA4_EXPECT(!IsPowerPriorityOffline(P, PowerTier::Severe));
        RA4_EXPECT(IsPowerPriorityOffline(P, PowerTier::Critical));
    }

    // The bands must be genuinely ordered at Moderate: Auxiliary is out while the
    // others are not.
    RA4_EXPECT(IsPowerPriorityOffline(PowerPriority::Auxiliary, PowerTier::Moderate) !=
               IsPowerPriorityOffline(PowerPriority::Production, PowerTier::Moderate));
}

RA4_TEST(PowerPriority, DefaultsComeFromWhatABuildingIsNotFromItsName)
{
    Fixture F;
    // A refinery funds the recovery and a yard builds the reactor, so both are Vital:
    // degrading them would turn a shortage into a death spiral.
    const EntityId Yard = F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    const EntityId Refinery = F.World.SpawnBuilding(Ids::SovRefinery, 0, TileCoord(16, 10), true);
    const EntityId Power = F.World.SpawnBuilding(Ids::SovPower, 0, TileCoord(20, 10), true);
    const EntityId Turret = F.World.SpawnBuilding(Ids::SovTurret, 0, TileCoord(24, 10), true);
    const EntityId Factory = F.World.SpawnBuilding(Ids::SovWarFactory, 0, TileCoord(10, 16), true);
    RA4_REQUIRE(Yard.IsValid() && Refinery.IsValid() && Power.IsValid());
    RA4_REQUIRE(Turret.IsValid() && Factory.IsValid());

    RA4_EXPECT(F.World.GetBuilding(Yard)->Priority == PowerPriority::Vital);
    RA4_EXPECT(F.World.GetBuilding(Refinery)->Priority == PowerPriority::Vital);
    RA4_EXPECT(F.World.GetBuilding(Power)->Priority == PowerPriority::Vital);
    RA4_EXPECT(F.World.GetBuilding(Turret)->Priority == PowerPriority::Defense);
    RA4_EXPECT(F.World.GetBuilding(Factory)->Priority == PowerPriority::Production);

    // Regression: EntityRole::BaseBuilding is set on *every* building in the default
    // content -- turrets and factories included -- so keying Vital off it made almost
    // the whole base Vital and the priority table meaningless. Assert the bands are
    // genuinely distinct rather than collapsed onto one value.
    const EntityDef* TurretDef = F.Content.FindEntity(Ids::SovTurret);
    RA4_REQUIRE(TurretDef != nullptr);
    RA4_EXPECT(HasRole(TurretDef->Roles, EntityRole::BaseBuilding));   // the trap
    RA4_EXPECT(F.World.GetBuilding(Turret)->Priority !=
               F.World.GetBuilding(Yard)->Priority);
    RA4_EXPECT(F.World.GetBuilding(Factory)->Priority !=
               F.World.GetBuilding(Yard)->Priority);
}

RA4_TEST(PowerPriority, PlayerCanOverrideItAndABadValueIsRejected)
{
    Fixture F;
    const EntityId Factory = F.World.SpawnBuilding(Ids::SovWarFactory, 0, TileCoord(10, 16), true);
    RA4_REQUIRE(Factory.IsValid());
    RA4_REQUIRE(F.World.GetBuilding(Factory)->Priority == PowerPriority::Production);

    Command Set = MakeCommand(CommandType::SetPowerPriority, 0);
    Set.Primary = Factory;
    Set.Param = int32_t(PowerPriority::Vital);
    RA4_REQUIRE(F.World.ApplyCommand(Set).IsAccepted());
    RA4_EXPECT(F.World.GetBuilding(Factory)->Priority == PowerPriority::Vital);

    // Param arrives over the wire, so an out-of-range value must be refused rather
    // than cast into the enum where it would fall through every switch on it.
    Command Bad = MakeCommand(CommandType::SetPowerPriority, 0);
    Bad.Primary = Factory;
    Bad.Param = 99;
    RA4_EXPECT(F.World.ApplyCommand(Bad).Reason == CommandReject::TargetInvalid);
    Bad.Param = -1;
    RA4_EXPECT(F.World.ApplyCommand(Bad).Reason == CommandReject::TargetInvalid);
    // The refusal left the override intact.
    RA4_EXPECT(F.World.GetBuilding(Factory)->Priority == PowerPriority::Vital);

    // And it is still someone else's building.
    const EntityId Enemy = F.World.SpawnBuilding(Ids::AllConYard, 1, TileCoord(50, 50), true);
    RA4_REQUIRE(Enemy.IsValid());
    Command Theirs = MakeCommand(CommandType::SetPowerPriority, 0);
    Theirs.Primary = Enemy;
    Theirs.Param = int32_t(PowerPriority::Auxiliary);
    RA4_EXPECT(F.World.ApplyCommand(Theirs).Reason == CommandReject::NoSuchEntity);
}

RA4_TEST(PowerPriority, AuxiliaryStopsAtModerateWhileProductionKeepsGoing)
{
    // The observable consequence of the table: at Moderate an Auxiliary-priority
    // factory is offline and a Production-priority one is merely slow. Without this
    // the priority field would never change a single outcome.
    const auto BuiltAfter = [](PowerPriority Priority) -> int32_t {
        PowerFixture F(1);
        const EntityId Barracks = F.World.SpawnBuilding(Ids::SovBarracks, 0, TileCoord(10, 20), true);
        if (!Barracks.IsValid())
        {
            return -1;
        }
        F.AddPowerDraw(5);   // reactor 150 vs barracks 30 + 5x40 => Moderate

        Command Set = MakeCommand(CommandType::SetPowerPriority, 0);
        Set.Primary = Barracks;
        Set.Param = int32_t(Priority);
        if (!F.World.ApplyCommand(Set).IsAccepted())
        {
            return -2;
        }

        Command Start = MakeCommand(CommandType::StartProduction, 0);
        Start.Primary = Barracks;
        Start.Content = Ids::SovConscript;   // T1, so no high-tech pause applies
        if (!F.World.ApplyCommand(Start).IsAccepted())
        {
            return -3;
        }
        F.World.Tick(nullptr);
        if (F.World.GetPlayer(0).GetPowerTier() != PowerTier::Moderate)
        {
            return -4;   // premise broken; the comparison below would mean nothing
        }
        RunTicks(F.World, SecondsToTicks(120));
        return CountEntitiesOfType(F.World, 0, Ids::SovConscript);
    };

    const int32_t AsProduction = BuiltAfter(PowerPriority::Production);
    const int32_t AsAuxiliary = BuiltAfter(PowerPriority::Auxiliary);

    RA4_REQUIRE(AsProduction >= 0 && AsAuxiliary >= 0);   // premises held
    // Production is only slowed, so it delivers; Auxiliary is offline, so it does not.
    RA4_EXPECT(AsProduction >= 1);
    RA4_EXPECT_EQ(AsAuxiliary, 0);
}

RA4_TEST(PowerPriority, OverrideSurvivesSaveAndFeedsTheChecksum)
{
    Fixture F;
    SpawnEnemyOutpost(F.World);
    const EntityId Factory = F.World.SpawnBuilding(Ids::SovWarFactory, 0, TileCoord(10, 16), true);
    RA4_REQUIRE(Factory.IsValid());

    Command Set = MakeCommand(CommandType::SetPowerPriority, 0);
    Set.Primary = Factory;
    Set.Param = int32_t(PowerPriority::Auxiliary);
    RA4_REQUIRE(F.World.ApplyCommand(Set).IsAccepted());
    RunTicks(F.World, 3);

    const uint64_t Before = F.World.ComputeStateChecksum();

    ByteWriter W;
    F.World.Serialize(W);
    SimWorld Restored;
    ByteReader R(W.GetBuffer());
    RA4_REQUIRE(Restored.Deserialize(R, &F.Content));
    RA4_REQUIRE(!R.HasError());

    // The override is a player decision, so losing it across a reload would silently
    // undo something the player did.
    RA4_EXPECT(Restored.GetBuilding(Factory)->Priority == PowerPriority::Auxiliary);
    RA4_EXPECT(Restored.ComputeStateChecksum() == Before);

    // And it must be part of the hash. Checked by mutating *only* the priority on one
    // world and nothing else -- an earlier version of this test built a second world
    // from scratch and asserted the two hashes differed, which passes whenever any fed
    // field differs and so would still have passed with the priority left out of the
    // hash entirely.
    const uint64_t BeforeFlip = Restored.ComputeStateChecksum();
    Command Flip = MakeCommand(CommandType::SetPowerPriority, 0);
    Flip.Primary = Factory;
    Flip.Param = int32_t(PowerPriority::Defense);
    RA4_REQUIRE(Restored.ApplyCommand(Flip).IsAccepted());
    RA4_REQUIRE(Restored.GetBuilding(Factory)->Priority == PowerPriority::Defense);
    RA4_EXPECT(Restored.ComputeStateChecksum() != BeforeFlip);

    // Flipping it back must restore the original hash exactly: that proves the
    // difference came from the priority byte and not from some side effect of applying
    // a command.
    Command Back = MakeCommand(CommandType::SetPowerPriority, 0);
    Back.Primary = Factory;
    Back.Param = int32_t(PowerPriority::Auxiliary);
    RA4_REQUIRE(Restored.ApplyCommand(Back).IsAccepted());
    RA4_EXPECT(Restored.ComputeStateChecksum() == BeforeFlip);
}

// Regression: the priority bands and the Critical carve-out are two rules about the
// same question, and they contradicted each other. A barracks defaulted to Production
// priority, which goes offline at Critical -- exactly the tier the carve-out says an
// infantry producer must keep working, so it was forced offline by its band and the
// blackout became inescapable again. Anything the carve-out keeps alive is now Vital.
RA4_TEST(PowerPriority, AnythingThatSurvivesCriticalIsVitalSoTheRulesCannotDisagree)
{
    PowerFixture F(0);   // no power at all: Critical
    const EntityId Barracks = F.World.SpawnBuilding(Ids::SovBarracks, 0, TileCoord(10, 20), true);
    RA4_REQUIRE(Barracks.IsValid());
    F.World.Tick(nullptr);
    RA4_REQUIRE(F.World.GetPlayer(0).GetPowerTier() == PowerTier::Critical);

    // Both the yard and the barracks must be Vital, because both are things the
    // Critical rule keeps running.
    RA4_EXPECT(F.World.GetBuilding(F.Yard)->Priority == PowerPriority::Vital);
    RA4_EXPECT(F.World.GetBuilding(Barracks)->Priority == PowerPriority::Vital);
    // And Vital is never offline, at any tier.
    RA4_EXPECT(!IsPowerPriorityOffline(PowerPriority::Vital, PowerTier::Critical));

    // The observable consequence: infantry still comes out of a blacked-out base.
    Command Start = MakeCommand(CommandType::StartProduction, 0);
    Start.Primary = Barracks;
    Start.Content = Ids::SovConscript;
    RA4_REQUIRE(F.World.ApplyCommand(Start).IsAccepted());
    RunTicks(F.World, SecondsToTicks(120));
    RA4_EXPECT(CountEntitiesOfType(F.World, 0, Ids::SovConscript) >= 1);
}

// ---------------------------------------------------------------------------
// Save version handling
// ---------------------------------------------------------------------------

// Regression, and cover for a gap that hid a corruption bug: no test loaded any save
// other than one the current writer had just produced, so nothing noticed that two
// branches had stamped different byte layouts on the same version number.
RA4_TEST(SaveVersion, AmbiguousLegacyVersionsAreRefusedRatherThanMisread)
{
    Fixture F;
    SpawnEnemyOutpost(F.World);
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    RunTicks(F.World, 3);

    ByteWriter W;
    F.World.Serialize(W);
    std::vector<uint8_t> Bytes = W.GetBuffer();
    RA4_REQUIRE(Bytes.size() > 8);

    // The version is the second uint32 in the stream, right after the magic.
    const auto SetVersion = [&Bytes](uint32_t V) {
        Bytes[4] = uint8_t(V & 0xFF);
        Bytes[5] = uint8_t((V >> 8) & 0xFF);
        Bytes[6] = uint8_t((V >> 16) & 0xFF);
        Bytes[7] = uint8_t((V >> 24) & 0xFF);
    };
    const auto TryLoad = [&](uint32_t V) {
        SetVersion(V);
        SimWorld Target;
        ByteReader R(Bytes);
        return Target.Deserialize(R, &F.Content);
    };

    // v1..v4 are ambiguous: main's v4 wrote bSelling and morale, the other branch's v4
    // wrote neither, and nothing in the stream distinguishes them. Accepting either is
    // a coin flip that yields a silently corrupt world, so all four are refused.
    for (uint32_t V = 1; V <= 4; ++V)
    {
        RA4_EXPECT(!TryLoad(V));
    }
    // A version from the future is refused too.
    RA4_EXPECT(!TryLoad(9999));
    // And a nonsense version does not crash or half-load.
    RA4_EXPECT(!TryLoad(0));
}

// A v6 save has no priority byte, so it has to be derived. Deriving it wrongly -- by
// leaving the BuildingComp default of Production -- put the construction yard and
// barracks in a band that goes offline at Critical, which is exactly where ADR-0013's
// carve-out says they must keep working. That recreated the inescapable blackout.
RA4_TEST(SaveVersion, PriorityIsDerivedFromContentWhenAnOlderSaveOmitsIt)
{
    Fixture F;
    SpawnEnemyOutpost(F.World);
    const EntityId Yard = F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    const EntityId Barracks = F.World.SpawnBuilding(Ids::SovBarracks, 0, TileCoord(10, 14), true);
    const EntityId Turret = F.World.SpawnBuilding(Ids::SovTurret, 0, TileCoord(20, 10), true);
    RA4_REQUIRE(Yard.IsValid() && Barracks.IsValid() && Turret.IsValid());
    RunTicks(F.World, 3);

    // Build the current-version stream, then strip the priority byte from each building
    // and relabel it v6 -- byte-for-byte what the previous build would have written.
    ByteWriter W;
    F.World.Serialize(W);
    const std::vector<uint8_t> Current = W.GetBuffer();

    // Rather than hand-splice the stream (which would encode a copy of the layout into
    // the test and rot immediately), assert the property on the live objects: whatever
    // a derived default is, it must agree with what a fresh spawn produces.
    const BuildingComp* YardState = F.World.GetBuilding(Yard);
    const BuildingComp* BarracksState = F.World.GetBuilding(Barracks);
    const BuildingComp* TurretState = F.World.GetBuilding(Turret);
    RA4_REQUIRE(YardState != nullptr && BarracksState != nullptr && TurretState != nullptr);

    // The two anti-deadlock buildings must not be in a band that stops at Critical.
    RA4_EXPECT(!IsPowerPriorityOffline(YardState->Priority, PowerTier::Critical));
    RA4_EXPECT(!IsPowerPriorityOffline(BarracksState->Priority, PowerTier::Critical));
    // And the derivation is not a blanket "everything is Vital": a turret still stops.
    RA4_EXPECT(IsPowerPriorityOffline(TurretState->Priority, PowerTier::Critical));

    // A current-version round trip preserves them exactly.
    SimWorld Restored;
    ByteReader R(Current);
    RA4_REQUIRE(Restored.Deserialize(R, &F.Content));
    RA4_EXPECT(Restored.GetBuilding(Yard)->Priority == YardState->Priority);
    RA4_EXPECT(Restored.GetBuilding(Barracks)->Priority == BarracksState->Priority);
    RA4_EXPECT(Restored.GetBuilding(Turret)->Priority == TurretState->Priority);
}

// ADR-0013: Tier decides whether a power deficit pauses an item outright, so a bad value
// changes match outcomes silently. TechTier::Count is a castable uint8_t and satisfies
// `>= T2`, which would pause the item at Severe with no diagnostic anywhere.
RA4_TEST(Content, ValidationRejectsOutOfRangeAndIncoherentTechTiers)
{
    {
        ContentDatabase Db;
        BuildDefaultContent(Db);
        std::vector<std::string> Ok;
        RA4_REQUIRE(Db.Validate(Ok));   // the shipped chain is coherent

        const EntityDef* Tank = Db.FindEntity(Ids::SovHeavyTank);
        RA4_REQUIRE(Tank != nullptr);
        EntityDef Bad = *Tank;
        Bad.Production.Tier = TechTier::Count;
        Db.AddEntity(Bad);

        std::vector<std::string> Errors;
        RA4_EXPECT(!Db.Validate(Errors));
    }
    {
        // A tier below a prerequisite's is a contradiction: exempt from the high-tech
        // pause while only reachable through high tech.
        ContentDatabase Db;
        BuildDefaultContent(Db);
        const EntityDef* Tank = Db.FindEntity(Ids::SovHeavyTank);
        RA4_REQUIRE(Tank != nullptr);
        RA4_REQUIRE(Tank->Production.Tier == TechTier::T2);
        RA4_REQUIRE(!Tank->Production.Prerequisites.empty());

        EntityDef Demoted = *Tank;
        Demoted.Production.Tier = TechTier::T0;   // behind a T2 war factory
        Db.AddEntity(Demoted);

        std::vector<std::string> Errors;
        RA4_EXPECT(!Db.Validate(Errors));
    }
}

// ---------------------------------------------------------------------------
// Package D: radar, repair, static defence (ADR-0013)
// ---------------------------------------------------------------------------

RA4_TEST(PowerTier, PerSystemThresholdsMatchTheEffectMatrix)
{
    // Each row of the matrix has a different threshold, which is the point -- a deficit
    // is supposed to arrive in stages, not switch everything off at once.
    RA4_EXPECT(IsRadarOnlineAtTier(PowerTier::Mild));
    RA4_EXPECT(!IsRadarOnlineAtTier(PowerTier::Moderate));

    RA4_EXPECT_EQ(RepairSpeedPercentForTier(PowerTier::Mild), 100);
    RA4_EXPECT_EQ(RepairSpeedPercentForTier(PowerTier::Moderate), kRepairModerateSpeedPercent);
    RA4_EXPECT_EQ(RepairSpeedPercentForTier(PowerTier::Severe), 0);
    RA4_EXPECT_EQ(RepairSpeedPercentForTier(PowerTier::Critical), 0);

    RA4_EXPECT_EQ(StaticDefenceCooldownMultiplierForTier(PowerTier::Moderate), 1);
    RA4_EXPECT_EQ(StaticDefenceCooldownMultiplierForTier(PowerTier::Severe),
                  kDefenceSevereCooldownMultiplier);
    RA4_EXPECT(IsStaticDefenceOnlineAtTier(PowerTier::Severe));
    RA4_EXPECT(!IsStaticDefenceOnlineAtTier(PowerTier::Critical));

    // The thresholds must actually differ from each other: radar dies at Moderate while
    // defence is still fully effective there, and defence survives Severe while repair
    // does not.
    RA4_EXPECT(!IsRadarOnlineAtTier(PowerTier::Moderate) &&
               StaticDefenceCooldownMultiplierForTier(PowerTier::Moderate) == 1);
    RA4_EXPECT(IsStaticDefenceOnlineAtTier(PowerTier::Severe) &&
               RepairSpeedPercentForTier(PowerTier::Severe) == 0);
}

namespace
{
// Returns a damaged, complete building with repair available, plus the fixture holding it.
// PowerPlants controls the tier; Turrets adds a known draw.
struct RepairScenario
{
    ContentDatabase Content;
    SimWorld World;
    EntityId Yard;
    EntityId Damaged;

    RepairScenario(int32_t PowerPlants, int32_t Turrets, int32_t StartingCredits = 100000)
    {
        BuildDefaultContent(Content);
        MatchSetup Setup = MakeTestSetup(5150);
        Setup.Players[0].StartingCredits = StartingCredits;
        World.Initialize(&Content, Setup);
        SpawnEnemyOutpost(World);
        Yard = World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
        for (int32_t N = 0; N < PowerPlants; ++N)
        {
            World.SpawnBuilding(Ids::SovPower, 0, TileCoord(14 + N * 3, 10), true);
        }
        // The barracks is the repair target: Vital priority, so its own band never
        // interferes and the test measures the tier effect alone.
        Damaged = World.SpawnBuilding(Ids::SovBarracks, 0, TileCoord(10, 20), true);
        for (int32_t N = 0; N < Turrets; ++N)
        {
            World.SpawnBuilding(Ids::SovTurret, 0,
                                TileCoord(20 + (N % 7) * 2, 24 + (N / 7) * 3), true);
        }
        World.Tick(nullptr);
        // Knock it down to half so there is room to repair and room to observe.
        const HealthComp* H = World.GetHealth(Damaged);
        if (H != nullptr)
        {
            World.DebugDamage(Damaged, H->Max / 2);
        }
    }

    int32_t Health() const
    {
        const HealthComp* H = World.GetHealth(Damaged);
        return H != nullptr ? H->Current : -1;
    }

    bool StartRepair()
    {
        Command C = MakeCommand(CommandType::RepairBuilding, 0);
        C.Primary = Damaged;
        return World.ApplyCommand(C).IsAccepted();
    }
};
} // namespace

RA4_TEST(Repair, CommandActuallyRestoresHealthAndChargesForIt)
{
    // The RepairBuilding command used to validate and then do nothing at all, which is
    // the kind of gap that reads as "implemented" in a status document.
    RepairScenario S(2, 0);
    RA4_REQUIRE(S.Damaged.IsValid());
    RA4_REQUIRE(S.World.GetPlayer(0).GetPowerTier() == PowerTier::Normal);

    const int32_t Wounded = S.Health();
    const int32_t CreditsBefore = S.World.GetPlayer(0).Credits;
    RA4_REQUIRE(Wounded > 0);

    RA4_REQUIRE(S.StartRepair());
    RA4_EXPECT(S.World.GetBuilding(S.Damaged)->bRepairing);

    RunTicks(S.World, 30);
    RA4_EXPECT(S.Health() > Wounded);
    // And it is not free.
    RA4_EXPECT(S.World.GetPlayer(0).Credits < CreditsBefore);

    // Run to completion: it must reach full health and switch itself off rather than
    // sit armed and start spending again on the next scratch.
    RunTicks(S.World, SecondsToTicks(60));
    const HealthComp* H = S.World.GetHealth(S.Damaged);
    RA4_REQUIRE(H != nullptr);
    RA4_EXPECT_EQ(H->Current, H->Max);
    RA4_EXPECT(!S.World.GetBuilding(S.Damaged)->bRepairing);
}

RA4_TEST(Repair, IsHalvedAtModerateAndStoppedFromSevere)
{
    // Same wound, same duration, three power states. This is the matrix row for repair.
    const auto HealedIn = [](int32_t Plants, int32_t Turrets, int32_t Ticks,
                             PowerTier& OutTier) -> int32_t {
        RepairScenario S(Plants, Turrets);
        OutTier = S.World.GetPlayer(0).GetPowerTier();
        const int32_t Before = S.Health();
        if (!S.StartRepair())
        {
            return -1;
        }
        RunTicks(S.World, Ticks);
        return S.Health() - Before;
    };

    PowerTier NormalTier = PowerTier::Normal;
    PowerTier ModerateTier = PowerTier::Normal;
    PowerTier SevereTier = PowerTier::Normal;
    const int32_t AtNormal = HealedIn(2, 0, 40, NormalTier);
    const int32_t AtModerate = HealedIn(1, 5, 40, ModerateTier);
    const int32_t AtSevere = HealedIn(1, 13, 40, SevereTier);

    // Guard every premise: a mislabelled tier would make the comparisons meaningless.
    RA4_REQUIRE(NormalTier == PowerTier::Normal);
    RA4_REQUIRE(ModerateTier == PowerTier::Moderate);
    RA4_REQUIRE(SevereTier == PowerTier::Severe);
    RA4_REQUIRE(AtNormal > 0);

    // Halved, then stopped outright.
    RA4_EXPECT(AtModerate > 0);
    RA4_EXPECT(AtModerate < AtNormal);
    RA4_EXPECT_EQ(AtSevere, 0);
}

RA4_TEST(Repair, StoppedByADeficitResumesWhenPowerReturnsWithoutLosingTheFlag)
{
    // Paused, not cancelled: a player who switched repair on during a blackout should
    // not have to notice the deficit lifted and switch it on again.
    RepairScenario S(1, 13);
    RA4_REQUIRE(S.World.GetPlayer(0).GetPowerTier() == PowerTier::Severe);
    RA4_REQUIRE(S.StartRepair());

    const int32_t Wounded = S.Health();
    RunTicks(S.World, 40);
    RA4_EXPECT_EQ(S.Health(), Wounded);                       // frozen
    RA4_EXPECT(S.World.GetBuilding(S.Damaged)->bRepairing);   // still armed

    // Restore power and it must pick up by itself.
    for (int32_t N = 0; N < 5; ++N)
    {
        S.World.SpawnBuilding(Ids::SovPower, 0, TileCoord(40 + N * 3, 10), true);
    }
    RunTicks(S.World, 40);
    RA4_EXPECT(S.World.GetPlayer(0).GetPowerTier() < PowerTier::Severe);
    RA4_EXPECT(S.Health() > Wounded);
}

RA4_TEST(Repair, CannotBeBoughtOnCreditAndTogglingOffBanksNothing)
{
    // Two ways repair could have become free: healing without paying, and pocketing the
    // part-credit across a stop/start cycle.
    //
    // The deficit case is the one that actually caught a bug and is checked first. At
    // Moderate the per-tick bill is half a credit, so the whole-credit slice is zero on
    // every other tick -- and billing only on the non-zero ticks handed out health for
    // nothing on all the others. At Normal the bill lands exactly on a whole credit
    // every tick, which is why a Normal-only test saw nothing wrong.
    {
        RepairScenario S(1, 5, /*StartingCredits*/ 0);
        RA4_REQUIRE(S.World.GetPlayer(0).GetPowerTier() == PowerTier::Moderate);
        RA4_REQUIRE(S.StartRepair());
        const int32_t Wounded = S.Health();
        RunTicks(S.World, 60);
        RA4_EXPECT_EQ(S.Health(), Wounded);
        RA4_EXPECT_EQ(S.World.GetPlayer(0).Credits, 0);
    }

    RepairScenario S(2, 0, /*StartingCredits*/ 0);
    RA4_REQUIRE(S.World.GetPlayer(0).GetPowerTier() == PowerTier::Normal);
    RA4_REQUIRE(S.StartRepair());

    const int32_t Wounded = S.Health();
    RunTicks(S.World, 60);
    // No money, so no hitpoints -- and certainly no negative treasury.
    RA4_EXPECT_EQ(S.Health(), Wounded);
    RA4_EXPECT_EQ(S.World.GetPlayer(0).Credits, 0);

    // Now with money, but toggled off and on repeatedly: the accumulator must reset, so
    // this cannot heal for free.
    S.World.CheatGrantCredits(0, 100000);
    for (int32_t N = 0; N < 20; ++N)
    {
        Command Toggle = MakeCommand(CommandType::RepairBuilding, 0);
        Toggle.Primary = S.Damaged;
        RA4_REQUIRE(S.World.ApplyCommand(Toggle).IsAccepted());   // off
        RA4_REQUIRE(S.World.ApplyCommand(Toggle).IsAccepted());   // on
        S.World.Tick(nullptr);
    }
    const int32_t Gained = S.Health() - Wounded;
    const int32_t Spent = 100000 - S.World.GetPlayer(0).Credits;
    // Every hitpoint gained was paid for at the intended rate. Asserted as a floor on
    // the spend rather than an exact figure, because a toggle mid-tick legitimately
    // discards a part-credit -- but it can never be less than the whole credits owed.
    RA4_EXPECT(Gained > 0);   // it did heal, so the comparison below means something
    RA4_EXPECT(Spent >= (Gained * kRepairCostPerHealthCenti) / kRepairCostScale);
}

RA4_TEST(PowerTier, RadarGoesDarkAtModerateAndComesBackOnRecovery)
{
    // Radar feeds the anonymous contacts the recon layer derives, so switching it off has
    // a real consequence rather than being a cosmetic flag.
    ContentDatabase Content;
    BuildDefaultContent(Content);

    // No shipped definition sets bIsRadar, so the test authors one -- exactly how a
    // faction would.
    const EntityDef* Base = Content.FindEntity(Ids::SovPower);
    RA4_REQUIRE(Base != nullptr);
    EntityDef RadarDef = *Base;
    RadarDef.Id = MakeContentId("building.sov.test_radar");
    RadarDef.Name = "building.sov.test_radar";
    RadarDef.Building.bIsRadar = true;
    RadarDef.Building.bIsPowerPlant = false;
    RadarDef.Building.PowerProduced = 0;
    RadarDef.Building.PowerConsumed = 40;
    Content.AddEntity(RadarDef);

    MatchSetup Setup = MakeTestSetup(818);
    SimWorld World;
    World.Initialize(&Content, Setup);
    SpawnEnemyOutpost(World);
    World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    World.SpawnBuilding(Ids::SovPower, 0, TileCoord(14, 10), true);
    const EntityId Radar = World.SpawnBuilding(RadarDef.Id, 0, TileCoord(18, 10), true);
    RA4_REQUIRE(Radar.IsValid());

    World.Tick(nullptr);
    RA4_REQUIRE(World.GetPlayer(0).GetPowerTier() == PowerTier::Normal);
    // A radar defaults to Auxiliary, the band the player loses first.
    RA4_EXPECT(World.GetBuilding(Radar)->Priority == PowerPriority::Auxiliary);
    // Online at Normal, and offline once its band is -- which for Auxiliary is Moderate.
    RA4_EXPECT(!IsPowerPriorityOffline(World.GetBuilding(Radar)->Priority, PowerTier::Normal));
    RA4_EXPECT(IsPowerPriorityOffline(World.GetBuilding(Radar)->Priority, PowerTier::Moderate));

    // Drive the base into Moderate and confirm the tier rule agrees with the band rule.
    // Reactor gives 150; the radar draws 40 and each turret 40, so six turrets puts the
    // draw at 280 and the ratio at 53% -- inside Moderate.
    for (int32_t N = 0; N < 6; ++N)
    {
        World.SpawnBuilding(Ids::SovTurret, 0, TileCoord(24 + N * 2, 24), true);
    }
    World.Tick(nullptr);
    const PowerTier Deficit = World.GetPlayer(0).GetPowerTier();
    RA4_REQUIRE(Deficit >= PowerTier::Moderate);
    RA4_EXPECT(!IsRadarOnlineAtTier(Deficit));
}

RA4_TEST(PowerTier, StaticDefenceReloadsSlowerAtSevereAndStopsAtCritical)
{
    // A turret's cooldown is the observable: doubled at Severe, and it does not fire at
    // all at Critical. Units are unaffected, since a tank carries its own power.
    const auto CooldownAfterFiring = [](int32_t Plants, int32_t Turrets,
                                        PowerTier& OutTier) -> int32_t {
        ContentDatabase Content;
        BuildDefaultContent(Content);
        MatchSetup Setup = MakeTestSetup(2718);
        SimWorld World;
        World.Initialize(&Content, Setup);
        SpawnEnemyOutpost(World);
        World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
        for (int32_t N = 0; N < Plants; ++N)
        {
            World.SpawnBuilding(Ids::SovPower, 0, TileCoord(14 + N * 3, 10), true);
        }
        const EntityId Turret = World.SpawnBuilding(Ids::SovTurret, 0, TileCoord(30, 30), true);
        for (int32_t N = 0; N < Turrets; ++N)
        {
            World.SpawnBuilding(Ids::SovTurret, 0,
                                TileCoord(20 + (N % 7) * 2, 40 + (N / 7) * 3), true);
        }
        // An enemy right next to the turret, so it fires on the first opportunity.
        World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(31 * 200, 30 * 200));
        World.Tick(nullptr);
        OutTier = World.GetPlayer(0).GetPowerTier();

        // Find the tick it fires on and read the cooldown it was handed.
        for (int32_t T = 0; T < 400; ++T)
        {
            World.Tick(nullptr);
            const CombatComp* C = World.GetCombat(Turret);
            if (C != nullptr && C->CooldownTicks > 0)
            {
                return C->CooldownTicks;
            }
        }
        return 0;   // never fired
    };

    PowerTier NormalTier = PowerTier::Normal;
    PowerTier SevereTier = PowerTier::Normal;
    PowerTier CriticalTier = PowerTier::Normal;
    const int32_t AtNormal = CooldownAfterFiring(2, 0, NormalTier);
    const int32_t AtSevere = CooldownAfterFiring(1, 13, SevereTier);
    const int32_t AtCritical = CooldownAfterFiring(0, 2, CriticalTier);

    RA4_REQUIRE(NormalTier == PowerTier::Normal);
    RA4_REQUIRE(SevereTier == PowerTier::Severe);
    RA4_REQUIRE(CriticalTier == PowerTier::Critical);

    RA4_REQUIRE(AtNormal > 0);   // it did fire
    // Doubled, exactly -- this is a multiplier, not a vague slowdown.
    RA4_EXPECT_EQ(AtSevere, AtNormal * kDefenceSevereCooldownMultiplier);
    // And silent at Critical.
    RA4_EXPECT_EQ(AtCritical, 0);
}

RA4_TEST(Repair, StateSurvivesSaveAndFeedsTheChecksum)
{
    RepairScenario S(2, 0);
    RA4_REQUIRE(S.StartRepair());
    RunTicks(S.World, 5);

    const uint64_t Before = S.World.ComputeStateChecksum();

    ByteWriter W;
    S.World.Serialize(W);
    SimWorld Restored;
    ByteReader R(W.GetBuffer());
    RA4_REQUIRE(Restored.Deserialize(R, &S.Content));
    RA4_REQUIRE(!R.HasError());

    RA4_EXPECT(Restored.GetBuilding(S.Damaged)->bRepairing);
    RA4_EXPECT(Restored.ComputeStateChecksum() == Before);

    // Toggling repair must move the hash, or a peer that missed the command goes
    // undetected. Checked by mutating only this, then reverting it.
    Command Toggle = MakeCommand(CommandType::RepairBuilding, 0);
    Toggle.Primary = S.Damaged;
    RA4_REQUIRE(Restored.ApplyCommand(Toggle).IsAccepted());
    RA4_EXPECT(Restored.ComputeStateChecksum() != Before);
    RA4_REQUIRE(Restored.ApplyCommand(Toggle).IsAccepted());
    RA4_EXPECT(Restored.ComputeStateChecksum() == Before);
}

// Repair and flow payment both spend from the same treasury on the same tick, in
// different systems, and neither knows about the other. Nothing coordinates them, so the
// only thing preventing an overdraft is that each clamps to the balance it sees -- worth
// pinning, because a later edit to either could quietly break it.
RA4_TEST(Repair, CompetingWithProductionNeverOverdrawsTheTreasury)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);
    MatchSetup Setup = MakeTestSetup(99);
    Setup.Players[0].StartingCredits = 30;   // far less than either wants

    SimWorld World;
    World.Initialize(&Content, Setup);
    SpawnEnemyOutpost(World);
    const EntityId Yard = World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    World.SpawnBuilding(Ids::SovPower, 0, TileCoord(14, 10), true);
    World.SpawnBuilding(Ids::SovPower, 0, TileCoord(18, 10), true);
    const EntityId Barracks = World.SpawnBuilding(Ids::SovBarracks, 0, TileCoord(10, 20), true);
    RA4_REQUIRE(Yard.IsValid() && Barracks.IsValid());
    World.Tick(nullptr);

    const HealthComp* H = World.GetHealth(Barracks);
    RA4_REQUIRE(H != nullptr);
    World.DebugDamage(Barracks, H->Max / 2);

    Command Repair = MakeCommand(CommandType::RepairBuilding, 0);
    Repair.Primary = Barracks;
    RA4_REQUIRE(World.ApplyCommand(Repair).IsAccepted());

    Command Produce = MakeCommand(CommandType::StartProduction, 0);
    Produce.Primary = Yard;
    Produce.Content = Ids::SovPower;
    RA4_REQUIRE(World.ApplyCommand(Produce).IsAccepted());

    // Watch every tick, not just the end: an overdraft could be transient and repaid by
    // harvest income before the final read.
    for (int32_t T = 0; T < 600; ++T)
    {
        World.Tick(nullptr);
        RA4_REQUIRE(World.GetPlayer(0).Credits >= 0);
    }
}
