// Copyright (c) Red Alert 4 project. Tests for the RA3-style tactical roster:
// transport boarding, the multigunner weapon swap, EMP stun locks and infection
// damage-over-time. Every mechanic under test is content-driven (WeaponDef
// on-hit payloads, UnitInfo passenger fields), so these tests fail if the
// DefaultContent definitions and the simulation ever drift apart.
//
// Fixture note: SystemVictory ends the match the moment either active side holds
// neither an armed unit nor a non-rubble building, and a lone WALL counts as
// rubble -- so tests that tick for a while give both sides a capability token
// (an extra armed unit, or SpawnEnemyOutpost), exactly like TestSimulation does.
//
// NOTE(main-agent): this file must be registered in Tools/HeadlessBuild/CMakeLists.txt
// (source list of the RA4Tests target). Deliberately not done here: that file is
// owned by the coordinating agent.
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Core/SimConfig.h"

#include <vector>

using namespace RA4;
using namespace RA4Test;

namespace
{

// Content ids used here. Duplicated from DefaultContent.cpp on purpose, matching
// the TestHelpers.h convention: a rename must fail loudly rather than test nothing.
constexpr ContentId SovEmpTrooper = MakeContentId("unit.sov.grom_trooper");
constexpr ContentId SovAmphTransport = MakeContentId("unit.sov.amphibious_transport");
constexpr ContentId SovRocketTrooper = MakeContentId("unit.sov.rocket_trooper");
constexpr ContentId AllMultigunnerIfv = MakeContentId("unit.all.multigunner_ifv");
constexpr ContentId AllWall = MakeContentId("building.all.wall");
constexpr ContentId EcSwarmInfector = MakeContentId("unit.ec.swarm_infector");
constexpr ContentId WpnRocketLauncher = MakeContentId("weapon.rocket_launcher");
constexpr ContentId WpnIfvAutocannon = MakeContentId("weapon.ifv_autocannon");

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

// True when Rider sits in some transport's bay.
inline bool bIsBoarded(const SimWorld& World, EntityId Rider)
{
    const PassengerComp* Bay = World.GetPassengerOf(Rider);
    return Bay != nullptr && Bay->Transport.IsValid();
}

} // namespace

// ---------------------------------------------------------------------------
// Transports
// ---------------------------------------------------------------------------

RA4_TEST(Tactics, TransportRoundTrip)
{
    Fixture F;
    // Capability tokens so neither side trips the defeat check mid-test.
    SpawnEnemyOutpost(F.World);

    const EntityId Transport = F.World.SpawnUnit(SovAmphTransport, 0, Vec2::FromInts(2000, 2000));
    const EntityId Rider = F.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(2100, 2000));
    RA4_REQUIRE(Transport.IsValid() && Rider.IsValid());

    Command Board = MakeCommand(CommandType::BoardTransport, 0);
    Board.Primary = Rider;      // the passenger
    Board.Target = Transport;   // the vehicle with the bay
    RA4_EXPECT(F.World.ApplyCommand(Board).IsAccepted());

    // Walk-in plus dock check at 100 units apart takes about a tick.
    const int32_t Boarded = RunUntil(F.World, SecondsToTicks(10),
                                     [&] { return bIsBoarded(F.World, Rider); });
    RA4_EXPECT(Boarded >= 0);

    const TransportComp* Hold = F.World.GetTransport(Transport);
    RA4_REQUIRE(Hold != nullptr);
    RA4_EXPECT_EQ(Hold->Passengers.size(), size_t(1));

    // A ridden transport carries its cargo along.
    Command Drive = MakeCommand(CommandType::Move, 0);
    Drive.Primary = Transport;
    Drive.Location = Vec2::FromInts(4000, 2000);
    RA4_REQUIRE(F.World.ApplyCommand(Drive).IsAccepted());

    RunTicks(F.World, SecondsToTicks(3));
    const Vec2 RiderMidway = F.World.GetTransform(Rider)->Position;
    RA4_EXPECT(DistanceSquared(RiderMidway, Vec2::FromInts(2100, 2000)) >
               Fixed::FromInt(100) * Fixed::FromInt(100));

    // Once the hull settles, the rider rides with it. Exact coincidence is not
    // asserted: soft separation keeps nudging a boarded pair apart by a step and
    // the passenger sync undoes it next tick, so they hover one nudge apart.
    const int32_t Arrived = RunUntil(F.World, SecondsToTicks(10), [&]
    {
        return !F.World.GetMovement(Transport)->bHasDestination
            && F.World.GetOrders(Transport)->IsEmpty();
    });
    RA4_EXPECT(Arrived >= 0);
    const Vec2 TPos = F.World.GetTransform(Transport)->Position;
    const Vec2 RPos = F.World.GetTransform(Rider)->Position;
    RA4_EXPECT(DistanceSquared(TPos, RPos) < Fixed::FromInt(100) * Fixed::FromInt(100));
    RA4_EXPECT(bIsBoarded(F.World, Rider));

    Command Unload = MakeCommand(CommandType::UnloadTransport, 0);
    Unload.Primary = Transport;
    RA4_EXPECT(F.World.ApplyCommand(Unload).IsAccepted());

    // Ejected: out of the bay, alive, bay empty again.
    const int32_t Dropped = RunUntil(F.World, SecondsToTicks(5),
                                     [&] { return !bIsBoarded(F.World, Rider); });
    RA4_EXPECT(Dropped >= 0);
    RA4_EXPECT(F.World.IsAlive(Rider));
    const TransportComp* After = F.World.GetTransport(Transport);
    RA4_REQUIRE(After != nullptr);
    RA4_EXPECT(After->Passengers.empty());
}

// ---------------------------------------------------------------------------
// Multigunner swap
// ---------------------------------------------------------------------------

// The multigunner rule lives in SimWorld::ResolveFireWeapon, which is private --
// so the public seam is behaviour: what the IFV actually fires, observable
// through WeaponFired events (Ev.Content carries the fired weapon id).
//
// Geometry note: soft separation keeps nudging a boarded pair apart and the
// passenger sync undoes it, so an occupied hull drifts steadily sideways. A
// single target would be left behind outside vision within seconds, so the
// victim here is a long WALL LINE spanning the whole drift path -- whatever the
// hull does, some wall segment stays adjacent, visible and in range.
RA4_TEST(Tactics, MultigunnerSwap)
{
    // Baseline A: an unarmed transport never fires, whatever stands next to it.
    {
        Fixture F;
        const EntityId Transport = F.World.SpawnUnit(SovAmphTransport, 0, Vec2::FromInts(2000, 2000));
        RA4_REQUIRE(Transport.IsValid());
        // Armed capability tokens: keeps both sides alive, parked out of the way.
        F.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(1000, 1000));
        F.World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(2200, 2000));   // hostile in range

        bool bFired = false;
        for (int32_t T = 0; T < SecondsToTicks(5); ++T)
        {
            F.World.Tick(nullptr);
            for (const SimEvent& E : F.World.GetEvents())
            {
                if (E.Type == SimEventType::WeaponFired && E.Entity == Transport)
                {
                    bFired = true;
                }
            }
            F.World.ClearEvents();
        }
        RA4_EXPECT(!bFired);
    }

    // West-to-east line through the IFV's start position.
    auto SpawnWallLine = [](SimWorld& World) -> std::vector<EntityId>
    {
        std::vector<EntityId> Walls;
        for (int32_t TX = 2; TX <= 16; ++TX)
        {
            const EntityId W = World.SpawnBuilding(AllWall, 1, TileCoord(TX, 10), true);
            if (!W.IsValid())
            {
                RA4_EXPECT(false); // wall spawn failed
            }
            else
            {
                Walls.push_back(W);
            }
        }
        return Walls;
    };

    const auto SumDamage = [&](const SimWorld& World, const std::vector<EntityId>& Walls) -> int32_t
    {
        int32_t Total = 0;
        for (const EntityId& W : Walls)
        {
            const HealthComp* H = World.GetHealth(W);
            if (H != nullptr)
            {
                Total += H->Max - H->Current;
            }
        }
        return Total;
    };

    // Shared driver: run an IFV arm for the window, return wall damage taken and
    // which weapons the hull fired.
    struct ArmResult
    {
        int32_t WallDamageTaken = 0;
        bool bFiredRocket = false;
        bool bFiredAutocannon = false;
    };
    const auto RunArm = [&](bool bWithGunner) -> ArmResult
    {
        ArmResult R;
        Fixture F;
        SpawnEnemyOutpost(F.World);
        const EntityId Ifv = F.World.SpawnUnit(AllMultigunnerIfv, 0, Vec2::FromInts(2000, 2000));
        const std::vector<EntityId> Walls = SpawnWallLine(F.World);
        if (!Ifv.IsValid() || Walls.empty())
        {
            RA4_EXPECT(false); // arm spawn failed
            return R;
        }

        if (bWithGunner)
        {
            const EntityId Gunner = F.World.SpawnUnit(SovRocketTrooper, 0, Vec2::FromInts(2100, 2000));
            if (!Gunner.IsValid())
            {
                RA4_EXPECT(false); // gunner spawn failed
                return R;
            }
            Command Board = MakeCommand(CommandType::BoardTransport, 0);
            Board.Primary = Gunner;
            Board.Target = Ifv;
            if (!F.World.ApplyCommand(Board).IsAccepted())
            {
                RA4_EXPECT(false); // board rejected
            }
            else if (RunUntil(F.World, SecondsToTicks(10),
                              [&] { return bIsBoarded(F.World, Gunner); }) < 0)
            {
                RA4_EXPECT(false); // never boarded
            }
        }

        for (int32_t T = 0; T < SecondsToTicks(25); ++T)
        {
            F.World.Tick(nullptr);
            for (const SimEvent& E : F.World.GetEvents())
            {
                if (E.Type != SimEventType::WeaponFired || E.Entity != Ifv)
                {
                    continue;
                }
                if (E.Content == WpnRocketLauncher)   { R.bFiredRocket = true; }
                if (E.Content == WpnIfvAutocannon)    { R.bFiredAutocannon = true; }
            }
            F.World.ClearEvents();
        }

        R.WallDamageTaken = SumDamage(F.World, Walls);
        return R;
    };

    const ArmResult Empty = RunArm(false);
    const ArmResult Loaded = RunArm(true);

    // Empty hull: fights with its own popgun only, barely scratches.
    RA4_EXPECT(!Empty.bFiredRocket);
    RA4_EXPECT(Empty.bFiredAutocannon);
    RA4_EXPECT(Empty.WallDamageTaken > 0);

    // Loaded hull: the swap itself -- the passenger's launcher fires through the
    // hull's hardpoint, the hull's own gun stays silent.
    RA4_EXPECT(Loaded.bFiredRocket);
    RA4_EXPECT(!Loaded.bFiredAutocannon);

    // And the outcome gap: what the popgun chipped, the borrowed launcher smashed.
    RA4_EXPECT(Loaded.WallDamageTaken > Empty.WallDamageTaken * 3);
}

// ---------------------------------------------------------------------------
// EMP stun lock
// ---------------------------------------------------------------------------

RA4_TEST(Tactics, EmpStunLocksTank)
{
    Fixture F;
    const EntityId Trooper = F.World.SpawnUnit(SovEmpTrooper, 0, Vec2::FromInts(2000, 2000));
    const EntityId Tank = F.World.SpawnUnit(Ids::SovHeavyTank, 1, Vec2::FromInts(2600, 2000));
    RA4_REQUIRE(Trooper.IsValid() && Tank.IsValid());

    // The stun must land on a target still inside the trooper's fog reveal
    // radius -- a tank ordered to flee from tick zero outruns acquisition -- so
    // the sequence under test is: lock first, then order the retreat.
    const int32_t Stunned = RunUntil(F.World, SecondsToTicks(5), [&]
    {
        const StatusComp* S = F.World.GetStatus(Tank);
        return S != nullptr && !S->bCanAct();
    });
    RA4_REQUIRE(Stunned >= 0);

    Command Flee = MakeCommand(CommandType::Move, 1);
    Flee.Primary = Tank;
    Flee.Location = Vec2::FromInts(12000, 2000);
    RA4_EXPECT(F.World.ApplyCommand(Flee).IsAccepted());

    // While the lock holds (60-tick stun against a 30-tick reload, refreshed by
    // every landed hit) the tank is a statue: position frozen despite an active
    // Move order, no shots fired back.
    const Vec2 FrozenPos = F.World.GetTransform(Tank)->Position;
    bool bMovedWhileStunned = false;
    bool bTankFired = false;
    bool bStillStunned = true;
    for (int32_t T = 0; T < SecondsToTicks(2); ++T)
    {
        F.World.Tick(nullptr);
        const StatusComp* S = F.World.GetStatus(Tank);
        RA4_REQUIRE(S != nullptr);
        if (S->bCanAct())
        {
            bStillStunned = false;
            break;
        }
        const Vec2 Now = F.World.GetTransform(Tank)->Position;
        if (Now.X != FrozenPos.X || Now.Y != FrozenPos.Y)
        {
            bMovedWhileStunned = true;
        }
        for (const SimEvent& E : F.World.GetEvents())
        {
            if (E.Type == SimEventType::WeaponFired && E.Entity == Tank)
            {
                bTankFired = true;
            }
        }
        F.World.ClearEvents();
    }

    RA4_EXPECT(bStillStunned);
    RA4_EXPECT(!bMovedWhileStunned);
    RA4_EXPECT(!bTankFired);
}

// ---------------------------------------------------------------------------
// Infection damage-over-time
// ---------------------------------------------------------------------------

RA4_TEST(Tactics, InfectionKillsOverTime)
{
    Fixture F;
    // Capability tokens parked far from the bite scene: once the infector is put
    // down below, its owner must not trip the defeat check and freeze the match
    // mid-drain.
    F.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(8000, 8000));

    const EntityId Infector = F.World.SpawnUnit(EcSwarmInfector, 0, Vec2::FromInts(2000, 2000));
    const EntityId Victim = F.World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(2200, 2000));
    RA4_REQUIRE(Infector.IsValid() && Victim.IsValid());

    // One bite is all this test needs; the spore's own damage is noise next to
    // the drain, so silence the biter before sampling.
    const int32_t Bitten = RunUntil(F.World, SecondsToTicks(5), [&]
    {
        const StatusComp* S = F.World.GetStatus(Victim);
        return S != nullptr && S->InfectionTicks > 0;
    });
    RA4_REQUIRE(Bitten >= 0);

    F.World.DebugDamage(Infector, 10000);
    RunTicks(F.World, 2);
    RA4_REQUIRE(!F.World.IsAlive(Infector));

    // Pure damage-over-time now: one hitpoint per tick, strictly monotonic,
    // until the victim dies well inside the infection duration -- with no
    // shooter left anywhere on the field.
    const HealthComp* H = F.World.GetHealth(Victim);
    RA4_REQUIRE(H != nullptr);
    const int32_t HpAtStart = H->Current;
    RA4_REQUIRE(HpAtStart > 0 && HpAtStart <= 200);   // the drain alone can finish

    int32_t Previous = HpAtStart;
    int32_t Ticks = 0;
    while (F.World.IsAlive(Victim) && Ticks < 300)
    {
        F.World.Tick(nullptr);
        F.World.ClearEvents();
        ++Ticks;
        if (!F.World.IsAlive(Victim))
        {
            break;   // drained to zero this tick
        }
        const HealthComp* Now = F.World.GetHealth(Victim);
        RA4_REQUIRE(Now != nullptr);
        RA4_EXPECT_EQ(Now->Current, Previous - 1);   // exactly the infestation drip
        Previous = Now->Current;
    }

    RA4_EXPECT(!F.World.IsAlive(Victim));
    // Death inside the 200-tick window proves the DoT did the killing: nothing
    // ever shot the victim after that single bite.
    RA4_EXPECT(Ticks < 200);
}

// NOTE(main-agent): register this file in Tools/HeadlessBuild/CMakeLists.txt
// (source list of the RA4Tests target) -- deliberately not done here because
// that file is owned by the coordinating agent.
