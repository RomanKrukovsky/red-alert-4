// Copyright (c) Red Alert 4 project. RA3 air logistics: finite aircraft
// magazines, the return-to-base rearm cycle, and the infinite-magazine rule
// for everything that is not an aircraft.
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Content/ContentDatabase.h"
#include "RA4Core/SimConfig.h"
#include "RA4Simulation/SimWorld.h"

using namespace RA4;

namespace
{
// The strike craft under test: a stock Soviet bomber out of the default
// content (Layer == Air, bomb-armed). Duplicated by name on purpose -- if the
// content id changes this test must fail rather than silently exercise nothing.
constexpr ContentId kTestJet = MakeContentId("unit.sov.mig_bomber");
constexpr ContentId kTestVictim = MakeContentId("unit.all.rifleman");

EntityId SpawnVictim(SimWorld& World, const Vec2& At)
{
    return World.SpawnUnit(kTestVictim, PlayerId{1}, At);
}

// Counts this tick's shots fired by Shooter and keeps a live victim in range:
// bombs kill their target, so the test respawns one rather than pretending a
// rifleman survives a siege. Returns true when Pred holds after the tick.
template <typename Predicate>
bool StepCombatTick(SimWorld& World, EntityId Shooter, EntityId& Victim, const Vec2& VictimSpot,
                    int32_t& OutShots, Predicate&& Pred)
{
    if (!World.IsAlive(Victim))
    {
        Victim = SpawnVictim(World, VictimSpot);
    }
    World.Tick(nullptr);
    for (const SimEvent& Ev : World.GetEvents())
    {
        if (Ev.Type == SimEventType::WeaponFired && Ev.Entity == Shooter)
        {
            ++OutShots;
        }
    }
    World.ClearEvents();
    return Pred();
}

bool IsAt(const Vec2& A, const Vec2& B)
{
    return A.X.Raw == B.X.Raw && A.Y.Raw == B.Y.Raw;
}
} // namespace

// The full logistics loop: fire until dry, refuse to shoot while dry, fly to
// the air producer, reload, and re-enter combat.
RA4_TEST(AircraftOps, AircraftDrysOutAndReturnsToBase)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);

    // Test airfield: any COMPLETE building whose production category builds
    // aircraft is a legal rearm pad -- the sim must find it data-derived, with
    // no flags. 2x2 footprint, spawned complete.
    EntityDef Pad;
    Pad.Id = MakeContentId("building.test.airfield");
    Pad.Name = "building.test.airfield";
    Pad.DisplayNameKey = "test.building.airfield";
    Pad.Kind = EntityKind::Building;
    Pad.Faction = FactionId::Soviet;
    Pad.MaxHealth = 1000;
    Pad.Armor = ArmorClass::Building;
    Pad.Roles = EntityRole::Production;
    Pad.Building.FootprintX = 2;
    Pad.Building.FootprintY = 2;
    Pad.Production.Category = ProductionCategory::Aircraft;
    const ContentId PadId = Content.AddEntity(Pad);

    auto Setup = RA4Test::MakeTestSetup(42);
    SimWorld World;
    World.Initialize(&Content, Setup);

    // Player 1 must own something at all times or the match ends instantly.
    RA4Test::SpawnEnemyOutpost(World);
    const EntityId Airfield = World.SpawnBuilding(PadId, PlayerId{0}, TileCoord(8, 8), /*bInstantComplete=*/true);
    RA4_REQUIRE(Airfield.IsValid());

    // Far enough from the pad that the bomber cannot reach the dock radius by
    // accident, close enough that its bombs reach the victim.
    const Vec2 JetHome = Vec2::FromInts(2600, 2600);
    const EntityId Jet = World.SpawnUnit(kTestJet, PlayerId{0}, JetHome);
    RA4_REQUIRE(Jet.IsValid());
    const TransformComp* PadT = World.GetTransform(Airfield);
    RA4_REQUIRE(PadT != nullptr);

    // 1. Full magazine at spawn.
    const CombatComp* C = World.GetCombat(Jet);
    RA4_REQUIRE(C != nullptr);
    RA4_EXPECT_EQ(C->AmmoMax, 12);
    RA4_EXPECT_EQ(C->AmmoCurrent, 12);

    // 2. Fire until the magazine runs dry. Bomb cooldown is 50 ticks, so twelve
    // shots fit comfortably inside the budget; victims respawn as they die.
    const Vec2 VictimSpot = JetHome + Vec2::FromInts(150, 0);
    EntityId Victim = SpawnVictim(World, VictimSpot);
    int32_t Shots = 0;
    bool bDriedOut = false;
    for (int32_t I = 0; I < SecondsToTicks(120) && !bDriedOut; ++I)
    {
        bDriedOut = StepCombatTick(World, Jet, Victim, VictimSpot, Shots,
                                   [&] { C = World.GetCombat(Jet); return C != nullptr && C->AmmoCurrent <= 0; });
    }
    RA4_REQUIRE(bDriedOut);
    RA4_REQUIRE(Shots >= 1);   // the magazine drained through actual firing

    // 3. Dry: no more shots even with a fresh victim in range, and a homing
    // destination exactly at the pad.
    int32_t QuietShots = 0;
    bool bSawHomingDestination = false;
    for (int32_t I = 0; I < SecondsToTicks(3); ++I)
    {
        const bool bDone = StepCombatTick(World, Jet, Victim, VictimSpot, QuietShots, [] { return false; });
        RA4_REQUIRE(!bDone);
        const MovementComp* M = World.GetMovement(Jet);
        if (M != nullptr && M->bHasDestination && IsAt(M->Destination, PadT->Position))
        {
            bSawHomingDestination = true;
        }
    }
    RA4_EXPECT_EQ(QuietShots, 0);
    RA4_EXPECT(bSawHomingDestination);

    // 4. Arrive and reload back to a full magazine.
    int32_t RearmTicks = RA4Test::RunUntil(World, SecondsToTicks(120),
                                           [&] { C = World.GetCombat(Jet); return C != nullptr && C->AmmoCurrent >= C->AmmoMax && C->AmmoMax > 0; });
    RA4_REQUIRE(RearmTicks >= 0);
    C = World.GetCombat(Jet);
    RA4_REQUIRE(C != nullptr);
    RA4_EXPECT_EQ(C->AmmoCurrent, C->AmmoMax);

    // 5. Rearmed: it fights again. The victim sits next to the pad, but soft
    // separation may have parked the craft on either side of it, so the test
    // issues a real Attack order -- the same deterministic close-and-fire path
    // any player would use -- instead of betting on auto-acquisition range.
    const Vec2 PadSpot = PadT->Position + Vec2::FromInts(250, 0);
    Victim = SpawnVictim(World, PadSpot);
    Command Strike = RA4Test::MakeCommand(CommandType::Attack, 0);
    Strike.Primary = Jet;
    Strike.Target = Victim;
    RA4_REQUIRE(World.ApplyCommand(Strike).IsAccepted());

    int32_t RearmedShots = 0;
    bool bRefired = false;
    for (int32_t I = 0; I < SecondsToTicks(120) && !bRefired; ++I)
    {
        bRefired = StepCombatTick(World, Jet, Victim, PadSpot, RearmedShots,
                                  [&] { C = World.GetCombat(Jet); return C != nullptr && C->AmmoCurrent < C->AmmoMax; });
    }
    RA4_REQUIRE(bRefired);
}

// Everything without an Air layer has a 0/0 infinite magazine: it spawns so,
// fires freely, and never decrements.
RA4_TEST(AircraftOps, GroundUnitHasInfiniteAmmo)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);
    auto Setup = RA4Test::MakeTestSetup(42);
    SimWorld World;
    World.Initialize(&Content, Setup);
    RA4Test::SpawnEnemyOutpost(World);

    const Vec2 Post = Vec2::FromInts(4000, 4000);
    const EntityId Conscript = World.SpawnUnit(RA4Test::Ids::SovConscript, PlayerId{0}, Post);
    RA4_REQUIRE(Conscript.IsValid());

    const CombatComp* C = World.GetCombat(Conscript);
    RA4_REQUIRE(C != nullptr);
    RA4_EXPECT_EQ(C->AmmoMax, 0);
    RA4_EXPECT_EQ(C->AmmoCurrent, 0);

    const Vec2 VictimSpot = Post + Vec2::FromInts(150, 0);
    EntityId Victim = SpawnVictim(World, VictimSpot);
    int32_t Shots = 0;
    bool bProven = false;
    for (int32_t I = 0; I < SecondsToTicks(120) && !bProven; ++I)
    {
        bProven = StepCombatTick(World, Conscript, Victim, VictimSpot, Shots,
                                 [&] { return Shots >= 5; });
    }
    RA4_REQUIRE(bProven);

    // Still 0/0 after sustained firing: no decrement ever happened.
    C = World.GetCombat(Conscript);
    RA4_REQUIRE(C != nullptr);
    RA4_EXPECT_EQ(C->AmmoMax, 0);
    RA4_EXPECT_EQ(C->AmmoCurrent, 0);
}
