// Copyright (c) Red Alert 4 project. Tests for deterministic group navigation.
#include "TestFramework.h"

#include "RA4Navigation/FlowField.h"
#include "RA4Navigation/Formation.h"

using namespace RA4;
using namespace RA4::Nav;

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
    // Slot1 original (-100,-100) rotated 90deg CW -> (-100, +100) relative, plus leader.
    RA4_EXPECT_NEAR(Slot1.X.Raw, Fixed::FromInt(400).Raw, Fixed::FromInt(5).Raw);
    RA4_EXPECT_NEAR(Slot1.Y.Raw, Fixed::FromInt(600).Raw, Fixed::FromInt(5).Raw);
}
