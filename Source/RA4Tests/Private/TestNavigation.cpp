// Copyright (c) Red Alert 4 project. Tests for deterministic group navigation.
#include "TestFramework.h"

#include "RA4Navigation/FlowField.h"

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
