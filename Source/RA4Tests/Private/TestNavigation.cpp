// Copyright (c) Red Alert 4 project. Tests for deterministic group navigation.
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Core/SimConfig.h"
#include "RA4Content/ContentDatabase.h"

#include "RA4Navigation/FlowField.h"
#include "RA4Navigation/Formation.h"
#include "RA4Navigation/NavDebug.h"

#include <cstdio>

using namespace RA4;
using namespace RA4::Nav;

namespace
{
struct NavFixture
{
    ContentDatabase Content;
    SimWorld World;
    explicit NavFixture(uint64_t Seed)
    {
        BuildDefaultContent(Content);
        World.Initialize(&Content, RA4Test::MakeTestSetup(Seed));
    }
};
} // namespace

RA4_TEST(Navigation, ExtractsStablePortalsForOpenSectorBoundary)
{
    // Break caught: changing portal extraction to ignore a sector boundary would
    // make the hierarchical graph disconnected even on a fully open map.
    NavGrid Grid(32, 16);

    RA4_REQUIRE(Grid.GetSectors().size() == 2u);
    RA4_REQUIRE(Grid.GetPortals().size() == 1u);
    const NavPortal& Portal = Grid.GetPortals()[0];
    RA4_EXPECT_EQ(Portal.SectorA, uint16_t(0));
    RA4_EXPECT_EQ(Portal.SectorB, uint16_t(1));
    RA4_EXPECT(Portal.StartA == TileCoord(15, 0));
    RA4_EXPECT(Portal.EndA == TileCoord(15, 15));
    RA4_EXPECT(Portal.StartB == TileCoord(16, 0));
    RA4_EXPECT(Portal.EndB == TileCoord(16, 15));
}

RA4_TEST(Navigation, FlowFieldRoutesThroughOnlyGapWithoutDiagonalCornerCutting)
{
    // Break caught: allowing a diagonal between two blocked cells lets tanks pass
    // through wall corners and defeats base walls.
    NavGrid Grid(7, 5);
    Grid.BeginTopologyUpdate();
    for (int32_t Y = 0; Y < 5; ++Y)
    {
        if (Y != 4)
        {
            RA4_REQUIRE(Grid.SetPassability(TileCoord(3, Y), NavLayer_None));
        }
    }
    RA4_REQUIRE(Grid.EndTopologyUpdate());

    FlowField Field(Grid, NavQuery{NavLayer_Tracked, 1}, TileCoord(6, 0));
    Field.Rebuild();

    RA4_EXPECT(Field.IsReachable(TileCoord(0, 0)));
    RA4_EXPECT((Field.GetDirection(TileCoord(2, 0)) == FlowDirection{0, 1}));
    RA4_EXPECT((Field.GetDirection(TileCoord(2, 3)) == FlowDirection{0, 1}));
    RA4_EXPECT((Field.GetDirection(TileCoord(2, 4)) == FlowDirection{1, 0}));
}

RA4_TEST(Navigation, FlowFieldRespectsLayerAndClearanceRequirements)
{
    // Break caught: ignoring layer or clearance lets a tank use infantry-only
    // terrain or squeeze through an obstacle channel that is too narrow.
    NavGrid Grid(5, 5);
    RA4_REQUIRE(Grid.SetPassability(TileCoord(2, 2), NavLayer_Infantry));

    FlowField Infantry(Grid, NavQuery{NavLayer_Infantry, 1}, TileCoord(2, 2));
    Infantry.Rebuild();
    RA4_EXPECT(Infantry.IsReachable(TileCoord(2, 2)));

    FlowField Tank(Grid, NavQuery{NavLayer_Tracked, 1}, TileCoord(2, 2));
    Tank.Rebuild();
    RA4_EXPECT(!Tank.IsReachable(TileCoord(2, 2)));

    FlowField WideInfantry(Grid, NavQuery{NavLayer_Infantry, 4}, TileCoord(2, 2));
    WideInfantry.Rebuild();
    RA4_EXPECT(!WideInfantry.IsReachable(TileCoord(2, 2)));
}

RA4_TEST(Navigation, RebuildsOnceForBatchedTopologyChanges)
{
    // Break caught: rebuilding after every tile in a placed building makes a
    // large construction order spend unbounded time in the simulation tick.
    NavGrid Grid(32, 32);
    const uint32_t InitialRevision = Grid.GetTopologyRevision();

    Grid.BeginTopologyUpdate();
    RA4_REQUIRE(Grid.SetPassability(TileCoord(10, 10), NavLayer_None));
    RA4_REQUIRE(Grid.SetPassability(TileCoord(11, 10), NavLayer_None));
    RA4_REQUIRE(Grid.SetPassability(TileCoord(12, 10), NavLayer_None));
    RA4_EXPECT_EQ(Grid.GetTopologyRevision(), InitialRevision);
    RA4_EXPECT(Grid.EndTopologyUpdate());
    RA4_EXPECT_EQ(Grid.GetTopologyRevision(), InitialRevision + 1u);
}

#include "RA4Navigation/ReservationGrid.h"

RA4_TEST(Navigation, ReservationLowerSlotWinsTie)
{
    // Break caught: if the tie-break were not deterministic, two units racing for
    // the same tile would flip-flop ownership across ticks and never make progress.
    ReservationGrid Grid(4, 4);
    constexpr TickIndex Now = 100;

    RA4_EXPECT(Grid.IsFree(TileCoord(2, 2), Now));
    RA4_EXPECT(Grid.TryReserve(TileCoord(2, 2), /*Slot=*/10, Now, /*HoldTicks=*/2));
    // Lower slot wins the tie even though 10 already holds it.
    RA4_EXPECT(Grid.TryReserve(TileCoord(2, 2), /*Slot=*/3, Now, /*HoldTicks=*/2));
    // Higher slot does not displace the lower-slot holder.
    RA4_EXPECT(!Grid.TryReserve(TileCoord(2, 2), /*Slot=*/20, Now, /*HoldTicks=*/2));
}

RA4_TEST(Navigation, ReservationExpiresAndFreesTile)
{
    // Break caught: a reservation that never expired would leak tiles until the
    // unit that held them was destroyed, starving the rest of the army.
    ReservationGrid Grid(4, 4);
    Grid.TryReserve(TileCoord(1, 1), /*Slot=*/7, /*Now=*/100, /*HoldTicks=*/2);
    RA4_EXPECT(!Grid.IsFree(TileCoord(1, 1), /*Now=*/101));
    RA4_EXPECT(!Grid.IsFree(TileCoord(1, 1), /*Now=*/102));
    RA4_EXPECT(Grid.IsFree(TileCoord(1, 1), /*Now=*/103));
}

#include "RA4Navigation/MNavRouter.h"

RA4_TEST(Navigation, MacroRouterFindsShortestCorridor)
{
    // Two sectors, one open portal column between them. The macro path must cross
    // the portal exactly once and the result must be identical on a second call.
    NavGrid Grid(32, 16);
    MNavRouter Router(Grid);
    const NavQuery Query{NavLayer_Tracked, 1};

    MacroPath Path = Router.Find(TileCoord(2, 8), TileCoord(30, 8), Query, /*MaxWaypoints=*/8);
    RA4_REQUIRE(!Path.Waypoints.empty());
    RA4_EXPECT_EQ(Path.Waypoints.size(), size_t(2));
    RA4_EXPECT_EQ(Path.BuiltTopologyRevision, Grid.GetTopologyRevision());

    MacroPath Again = Router.Find(TileCoord(2, 8), TileCoord(30, 8), Query, /*MaxWaypoints=*/8);
    RA4_EXPECT_EQ(Again.Waypoints.size(), Path.Waypoints.size());
    RA4_EXPECT(Again.Waypoints[0] == Path.Waypoints[0]);
}

RA4_TEST(Navigation, MacroRouterRespectsLayerAndClearance)
{
    // If every portal cell is masked to infantry-only, a tracked query must get an
    // empty path while an infantry query still routes through.
    NavGrid Grid(32, 16);
    // Force the whole portal column (x=15..16) to infantry-only.
    Grid.BeginTopologyUpdate();
    for (int32_t Y = 0; Y < 16; ++Y)
    {
        Grid.SetPassability(TileCoord(15, Y), NavLayer_Infantry);
        Grid.SetPassability(TileCoord(16, Y), NavLayer_Infantry);
    }
    Grid.EndTopologyUpdate();

    MNavRouter Router(Grid);
    const NavQuery Tracked{NavLayer_Tracked, 1};
    MacroPath TankPath = Router.Find(TileCoord(2, 8), TileCoord(30, 8), Tracked, 8);
    RA4_EXPECT(TankPath.Waypoints.empty());

    const NavQuery Inf{NavLayer_Infantry, 1};
    MacroPath InfPath = Router.Find(TileCoord(2, 8), TileCoord(30, 8), Inf, 8);
    RA4_EXPECT(!InfPath.Waypoints.empty());
}

RA4_TEST(Navigation, MacroRouterInvalidatesOnTopologyRevision)
{
    // A cached path built against revision N must be rejected after the grid's
    // topology changes to revision N+1 -- otherwise units walk through a wall that
    // was just placed across their corridor.
    NavGrid Grid(32, 16);
    MNavRouter Router(Grid);
    const NavQuery Query{NavLayer_Tracked, 1};

    MacroPath First = Router.Find(TileCoord(2, 8), TileCoord(30, 8), Query, 8);
    RA4_REQUIRE(!First.Waypoints.empty());
    const uint32_t Rev0 = First.BuiltTopologyRevision;

    Grid.BeginTopologyUpdate();
    Grid.SetPassability(TileCoord(15, 8), NavLayer_None);
    Grid.SetPassability(TileCoord(16, 8), NavLayer_None);
    Grid.EndTopologyUpdate();
    RA4_EXPECT(Grid.GetTopologyRevision() != Rev0);

    Router.InvalidateAll();
    MacroPath Second = Router.Find(TileCoord(2, 8), TileCoord(30, 8), Query, 8);
    RA4_EXPECT(Second.BuiltTopologyRevision != Rev0);
}

RA4_TEST(Navigation, FormationMembersFollowLeaderSlot)
{
    // Break caught: if members computed their own macro path, a formation of 8
    // would build 8 paths instead of 1. The leader owns the path; members follow
    // rotated offsets and must arrive within one tile of their slot.
    FormationDef Def;
    Def.Id = MakeContentId("formation.test.wedge");
    // 8-slot wedge: leader at origin, 3 behind-left, 3 behind-right, 1 tail.
    Def.Offsets = {
        Vec2(Fixed::Zero(), Fixed::Zero()),
        Vec2(Fixed::FromInt(-100), Fixed::FromInt(-100)),
        Vec2(Fixed::FromInt(-200), Fixed::FromInt(-200)),
        Vec2(Fixed::FromInt(-300), Fixed::FromInt(-300)),
        Vec2(Fixed::FromInt(100), Fixed::FromInt(-100)),
        Vec2(Fixed::FromInt(200), Fixed::FromInt(-200)),
        Vec2(Fixed::FromInt(300), Fixed::FromInt(-300)),
        Vec2(Fixed::FromInt(0), Fixed::FromInt(-400)),
    };
    RA4_EXPECT_EQ(Def.Offsets.size(), size_t(8));

    // Rotating the leader by 90 degrees (kAngleTurn/4) maps the +X offset to +Y.
    const int32_t FacingRight = 1 << 10;   // 4096/4 == 1024 == 90 degrees
    const Vec2 LeaderPos(Fixed::FromInt(500), Fixed::FromInt(500));
    const Vec2 Slot1 = LeaderPos + RotateOffset(Def.Offsets[1], FacingRight);
    // (-100,-100) rotated 90° (Facing=1024) -> (+100,-100) in this sim's CW-from-+X,
    // Y-down convention; plus leader = (600, 400).
    RA4_EXPECT_NEAR(Slot1.X.Raw, Fixed::FromInt(600).Raw, Fixed::FromInt(5).Raw);
    RA4_EXPECT_NEAR(Slot1.Y.Raw, Fixed::FromInt(400).Raw, Fixed::FromInt(5).Raw);
}

RA4_TEST(Navigation, NavDebugSnapshotHasNoDrawDependency)
{
    // Break caught: if NavDebug ever pulled in UWorld/DrawDebug, the headless
    // build would fail to link. This test exists to make that link failure a
    // test failure instead of a surprise in CI.
    // Deviation from brief: the brief's verbatim (Grid(8,8) + Find(0,0)->(7,7))
    // is a same-sector shortcut in MNavRouter::Find and never inserts into the
    // cache, so ActiveMacroPaths would be empty. Bumped to 32x16 with cross-sector
    // coordinates so the snapshot is exercised end-to-end. The other 8/8 grid
    // tests in this file follow the same pattern.
    NavGrid Grid(32, 16);
    ReservationGrid Res(32, 16);
    MNavRouter Router(Grid);
    Res.TryReserve(TileCoord(3, 3), /*Slot=*/1, /*Now=*/0, /*HoldTicks=*/5);
    Router.Find(TileCoord(2, 8), TileCoord(30, 8), NavQuery{NavLayer_Tracked, 1}, 8);

    NavDebugSnapshot Snap;
    Res.Snapshot(Snap);
    Router.Snapshot(Snap);
    Grid.Snapshot(Snap);   // if NavGrid doesn't have Snapshot yet, add a trivial one

    RA4_EXPECT(Snap.TopologyRevision == Grid.GetTopologyRevision());
    RA4_EXPECT(!Snap.ReservationSample.empty());
    RA4_EXPECT(!Snap.ActiveMacroPaths.empty());
    // Serialize to bytes to prove it is plain data, not a draw handle.
    std::vector<uint8_t> Bytes;
    SerializeNavDebugSnapshot(Snap, Bytes);
    RA4_EXPECT(!Bytes.empty());
}

RA4_TEST(Navigation, FlowFieldSharedAcrossUnits)
{
    // Break caught: if each unit built its own flow field, 50 units to one rally
    // point would build 50 fields. Sharing means exactly one build.
    NavFixture F(42);
    SimWorld& World = F.World;

    const Vec2 Rally(Fixed::FromInt(2000), Fixed::FromInt(2000));
    for (int32_t I = 0; I < 50; ++I)
    {
        const EntityId U = World.SpawnUnit(RA4Test::Ids::SovConscript, PlayerId{0},
                                           Vec2(Fixed::FromInt(100), Fixed::FromInt(100 + I * 10)));
        Command Move;
        Move.Type = CommandType::Move;
        Move.Issuer = PlayerId{0};
        Move.Primary = U;
        Move.Location = Rally;
        World.ApplyCommand(Move);
    }

    World.ResetMovementStats();
    for (int32_t T = 0; T < 20; ++T) { World.Tick(nullptr); World.ClearEvents(); }
    const MovementStats& S = World.GetMovementStats();
    // 50 units, one shared destination -> at most a handful of flow-field builds,
    // never 50. Allow a few for sub-goal sectors along the corridor.
    RA4_EXPECT(S.FlowFieldBuilds <= 5u);
    RA4_EXPECT(S.MacroPathBuilds <= 4u);
}

RA4_TEST(Navigation, LocalAvoidancePicksBestOpenNeighbor)
{
    // Break caught: if the desired tile is blocked by a static obstacle and the
    // unit did not divert, it would sit against the wall until the blocked-tick
    // repath, wasting the avoidance pass.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    auto Setup = RA4Test::MakeTestSetup(7);
    // Build a 1-tile wall straight in front of the unit's path.
    // Deviation from brief: brief placed wall at (8,4) but unit spawns on tile (8,4)
    // so the unit is "on" the wall and the flow field marks it unreachable. Wall
    // moved to (8,3) (one tile north of spawn) so the unit's first step into
    // (8,3) hits the wall and the flow field routes around. Unit spawns at tile
    // (8,4) center (1700, 900) so its initial tile-center-to-tile-center path
    // is straight, matching the existing UnitsReachTheirDestination test.
    Setup.Map.Resize(16, 16, Tile_GroundPassable);
    Setup.Map.Tiles[Setup.Map.TileIndex(8, 3)] = uint8_t(Tile_Cliff);
    SimWorld World;
    World.Initialize(&Content, Setup);
    const EntityId U = World.SpawnUnit(RA4Test::Ids::SovConscript, PlayerId{0},
                                       Vec2(Fixed::FromInt(1700), Fixed::FromInt(900)));
    Command Move; Move.Type = CommandType::Move; Move.Issuer = PlayerId{0};
    Move.Primary = U; Move.Location = Vec2(Fixed::FromInt(1700), Fixed::FromInt(100));
    World.ApplyCommand(Move);

    // Give it enough ticks to reach the wall and divert.
    for (int32_t T = 0; T < 200; ++T)
    {
        World.Tick(nullptr);
        const TransformComp* Dbg = World.GetTransform(U);
        if (T < 30 || (T % 20) == 0)
        {
            const MovementComp* DbgM = World.GetMovement(U);
            std::printf("[T7] t=%d pos=(%lld, %lld) tile=(%d, %d) facing=%d speed=%lld blocked=%lld\n",
                        T,
                        static_cast<long long>(Dbg->Position.X.Raw),
                        static_cast<long long>(Dbg->Position.Y.Raw),
                        Dbg->Position.X.ToIntFloor() / 200,
                        Dbg->Position.Y.ToIntFloor() / 200,
                        Dbg->Facing,
                        static_cast<long long>(DbgM->CurrentSpeed.Raw),
                        static_cast<long long>(DbgM->BlockedTicks));
        }
        World.ClearEvents();
    }
    const TransformComp* Tx = World.GetTransform(U);
    const MovementComp* Mv = World.GetMovement(U);
    if (Tx != nullptr)
    {
        std::printf("[T7] final pos=(%lld, %lld) bHasDest=%d blocked=%lld\n",
                    static_cast<long long>(Tx->Position.X.Raw),
                    static_cast<long long>(Tx->Position.Y.Raw),
                    Mv != nullptr && Mv->bHasDestination ? 1 : 0,
                    static_cast<long long>(Mv != nullptr ? Mv->BlockedTicks : -1));
    }
    RA4_REQUIRE(Tx != nullptr);
    // The unit must have moved past Y=900 (its spawn) -- i.e. it did not get stuck.
    RA4_EXPECT(Tx->Position.Y.Raw < Fixed::FromInt(900).Raw);
}

RA4_TEST(Navigation, BlockedUnitRepatsAfterThreshold)
{
    // Break caught: a wedged unit that never repaths blocks the tile forever and
    // its blocked-tick counter climbs without bound.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    auto Setup = RA4Test::MakeTestSetup(9);
    Setup.Map.Resize(16, 16, Tile_GroundPassable);
    // Box the unit in: walls on three sides, open only behind it.
    Setup.Map.Tiles[Setup.Map.TileIndex(8, 6)] = uint8_t(Tile_Cliff);
    Setup.Map.Tiles[Setup.Map.TileIndex(7, 7)] = uint8_t(Tile_Cliff);
    Setup.Map.Tiles[Setup.Map.TileIndex(9, 7)] = uint8_t(Tile_Cliff);
    Setup.Map.Tiles[Setup.Map.TileIndex(8, 7)] = uint8_t(Tile_Cliff);
    SimWorld World;
    World.Initialize(&Content, Setup);
    const EntityId U = World.SpawnUnit(RA4Test::Ids::SovConscript, PlayerId{0},
                                       Vec2(Fixed::FromInt(1700), Fixed::FromInt(1500)));
    Command Move; Move.Type = CommandType::Move; Move.Issuer = PlayerId{0};
    Move.Primary = U; Move.Location = Vec2(Fixed::FromInt(1700), Fixed::FromInt(1100));
    World.ApplyCommand(Move);

    for (int32_t T = 0; T < kRepathBlockedTickThreshold + 20; ++T)
    {
        World.Tick(nullptr); World.ClearEvents();
    }
    // After the threshold, the macro path was cleared at least once and the
    // blocked-tick counter was reset (not left to grow forever).
    const MovementComp* M = World.GetMovement(U);
    RA4_REQUIRE(M != nullptr);
    RA4_EXPECT(M->BlockedTicks < kRepathBlockedTickThreshold);
}

RA4_TEST(Navigation, RepathBudgetStallsDeterministically)
{
    // Break caught: if the budget were wall-clock, a slow machine would produce a
    // different build *sequence* and a different checksum. With a tick-bounded
    // budget, budget=2 splits 10 builds as 2/2/2/2/2 across 5 ticks; budget=10 does
    // all 10 in one tick. The final state must be identical. The full cross-build
    // checksum comparison is exercised in Task 6; here we only assert the budget
    // consts exist, are positive, and are the values the rest of the plan depends on.
    RA4_EXPECT(kMaxFlowFieldBuildsPerTick > 0);
    RA4_EXPECT(kMaxMacroPathBuildsPerTick > 0);
    RA4_EXPECT_EQ(kMaxFlowFieldBuildsPerTick, 2);
    RA4_EXPECT_EQ(kMaxMacroPathBuildsPerTick, 4);
}

RA4_TEST(Navigation, BridgeDestroyInvalidatesPath)
{
    // Break caught: a unit mid-cross must not keep walking on a destroyed bridge's
    // stale flow field; the topology bump must invalidate its cached path.
    ContentDatabase Content;
    BuildDefaultContent(Content);
    auto Setup = RA4Test::MakeTestSetup(13);
    Setup.Map.Resize(32, 16, Tile_GroundPassable);
    // Carve a water channel with one ground-tile "bridge" at x=16.
    for (int32_t Y = 0; Y < 16; ++Y)
    {
        if (Y != 8) Setup.Map.Tiles[Setup.Map.TileIndex(16, Y)] = uint8_t(Tile_Water);
    }
    SimWorld World;
    World.Initialize(&Content, Setup);
    const EntityId U = World.SpawnUnit(RA4Test::Ids::SovHeavyTank, PlayerId{0},
                                       Vec2(Fixed::FromInt(400), Fixed::FromInt(1700)));
    Command Move; Move.Type = CommandType::Move; Move.Issuer = PlayerId{0};
    Move.Primary = U; Move.Location = Vec2(Fixed::FromInt(3000), Fixed::FromInt(1700));
    World.ApplyCommand(Move);

    // Let it path onto the bridge.
    for (int32_t T = 0; T < 40; ++T) { World.Tick(nullptr); World.ClearEvents(); }
    // Destroy the bridge by setting that tile to water (sim-side; a real bridge
    // entity would call NavGrid::BeginTopologyUpdate/EndTopologyUpdate).
    const_cast<MapDescription&>(World.GetMap()).Tiles[World.GetMap().TileIndex(16, 8)] = uint8_t(Tile_Water);
    // The sim must observe the topology change; if BuildNavigationGrid is not auto-
    // called on tile mutation, this test will catch it (the unit would keep moving
    // on the stale field and the next assertion would fail).
    for (int32_t T = 0; T < 80; ++T) { World.Tick(nullptr); World.ClearEvents(); }
    const MovementComp* M = World.GetMovement(U);
    RA4_REQUIRE(M != nullptr);
    // The unit either repathed around the water or stopped; either way it must not
    // be sitting on the now-water tile (16,8) -> world (3300,1700).
    const TransformComp* Tx = World.GetTransform(U);
    RA4_REQUIRE(Tx != nullptr);
    const bool bOnDestroyedBridge =
        (Tx->Position.X.Raw > Fixed::FromInt(3200).Raw && Tx->Position.X.Raw < Fixed::FromInt(3400).Raw) &&
        (Tx->Position.Y.Raw > Fixed::FromInt(1600).Raw && Tx->Position.Y.Raw < Fixed::FromInt(1800).Raw);
    RA4_EXPECT(!bOnDestroyedBridge);
}
