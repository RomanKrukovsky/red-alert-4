// Copyright (c) Red Alert 4 project. Tests for the Fog of War simulation module.
#include "TestFramework.h"
#include "TestHelpers.h"

#include "FogOfWarGrid.h"
#include "RA4Content/ContentDatabase.h"
#include "RA4Simulation/SimWorld.h"

using namespace RA4;

RA4_TEST(FogOfWar, GridInitialization)
{
    FFogOfWarGrid Grid(64, 64, 2);

    RA4_EXPECT_EQ(Grid.GetWidth(), 64);
    RA4_EXPECT_EQ(Grid.GetHeight(), 64);
    RA4_EXPECT_EQ(Grid.GetNumPlayers(), 2);

    // Initial state must be NeverSeen everywhere
    for (int32_t Y = 0; Y < 64; ++Y)
    {
        for (int32_t X = 0; X < 64; ++X)
        {
            RA4_EXPECT(Grid.GetVisibility(0, X, Y) == VisibilityState::NeverSeen);
            RA4_EXPECT(Grid.GetVisibility(1, X, Y) == VisibilityState::NeverSeen);
        }
    }
}

RA4_TEST(FogOfWar, RevealCircularArea)
{
    FFogOfWarGrid Grid(32, 32, 2);

    // Reveal 3 tile radius circle around (10, 10) for Player 0
    Grid.RevealCircularArea(0, 10, 10, 3);

    // Center tile must be CurrentlyVisible
    RA4_EXPECT(Grid.GetVisibility(0, 10, 10) == VisibilityState::CurrentlyVisible);
    RA4_EXPECT(Grid.GetVisibility(0, 10, 13) == VisibilityState::CurrentlyVisible);
    RA4_EXPECT(Grid.GetVisibility(0, 13, 10) == VisibilityState::CurrentlyVisible);

    // Out of bounds distance (e.g. 10, 20) must stay NeverSeen
    RA4_EXPECT(Grid.GetVisibility(0, 10, 20) == VisibilityState::NeverSeen);

    // Player 1 fog must be unaffected
    RA4_EXPECT(Grid.GetVisibility(1, 10, 10) == VisibilityState::NeverSeen);

    // Dirty regions must be tracked
    const auto& Dirty = Grid.GetDirtyRegions(0);
    RA4_EXPECT(!Dirty.empty());

    Grid.ClearDirtyRegions(0);
    RA4_EXPECT(Grid.GetDirtyRegions(0).empty());
}

RA4_TEST(FogOfWar, ClearCurrentVisibility)
{
    FFogOfWarGrid Grid(32, 32, 2);

    Grid.RevealCircularArea(0, 15, 15, 4);
    RA4_EXPECT(Grid.GetVisibility(0, 15, 15) == VisibilityState::CurrentlyVisible);

    // Reset current visibility (simulation tick start)
    Grid.ClearCurrentVisibility(0);

    // CurrentlyVisible cells transition to PreviouslySeen
    RA4_EXPECT(Grid.GetVisibility(0, 15, 15) == VisibilityState::PreviouslySeen);

    // NeverSeen cells remain NeverSeen
    RA4_EXPECT(Grid.GetVisibility(0, 0, 0) == VisibilityState::NeverSeen);
}

RA4_TEST(FogOfWar, SimWorldIntegration)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);

    MatchSetup Setup = RA4Test::MakeTestSetup();
    SimWorld World;
    World.Initialize(&Content, Setup);

    const Vec2 SpawnPos(Fixed::FromInt(500), Fixed::FromInt(500)); // Tile (2, 2)
    World.SpawnUnit(RA4Test::Ids::SovConscript, 0, SpawnPos);

    CommandFrame Frame;
    World.Tick(&Frame);

    const FFogOfWarGrid* Fog = World.GetFogGrid();
    RA4_EXPECT(Fog != nullptr);

    // Tile (2, 2) and surrounding area should be CurrentlyVisible for Player 0
    RA4_EXPECT(Fog->GetVisibility(0, 2, 2) == VisibilityState::CurrentlyVisible);

    // Far corner (60, 60) should remain NeverSeen
    RA4_EXPECT(Fog->GetVisibility(0, 60, 60) == VisibilityState::NeverSeen);
}

RA4_TEST(FogOfWar, UnitMovementUpdatesFog)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);

    MatchSetup Setup = RA4Test::MakeTestSetup();
    SimWorld World;
    World.Initialize(&Content, Setup);

    const EntityId UnitId = World.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(500), Fixed::FromInt(500))); // Tile (2, 2)

    CommandFrame Frame;
    World.Tick(&Frame);

    const FFogOfWarGrid* Fog = World.GetFogGrid();
    RA4_EXPECT(Fog->GetVisibility(0, 35, 35) == VisibilityState::NeverSeen);

    // Order unit to move to (7000, 7000) -> Tile (35, 35)
    Command MoveCmd;
    MoveCmd.Type = CommandType::Move;
    MoveCmd.Issuer = 0;
    MoveCmd.Primary = UnitId;
    MoveCmd.Location = Vec2(Fixed::FromInt(7000), Fixed::FromInt(7000));
    World.ApplyCommand(MoveCmd);

    // Step simulation for 200 ticks (10 seconds) so unit advances toward target
    for (int i = 0; i < 200; ++i)
    {
        World.Tick(nullptr);
    }

    // Current position of unit should be CurrentlyVisible
    const TransformComp* Trans = World.GetTransform(UnitId);
    RA4_EXPECT(Trans != nullptr);
    TileCoord NewTile = World.GetMap().WorldToTile(Trans->Position);
    RA4_EXPECT(Fog->GetVisibility(0, NewTile.X, NewTile.Y) == VisibilityState::CurrentlyVisible);
}

RA4_TEST(FogOfWar, EntityVisibilityGateAnswersPerViewer)
{
    // V-A/V-B regression pin (VISIBILITY_CALLSITE_INVENTORY.md): the
    // presentation layer now gates actor visibility and cursor picking through
    // SimWorld::IsEntityVisibleTo. This pins the helper's contract from the
    // presentation side: own entities always visible, fogged enemies not,
    // seen enemies yes -- per viewer, not globally.
    ContentDatabase Content;
    BuildDefaultContent(Content);

    SimWorld World;
    World.Initialize(&Content, RA4Test::MakeTestSetup());

    // Player 0's scout at (2,2); player 1's units -- one adjacent (seen),
    // one across the map (fogged for player 0).
    const EntityId OwnScout =
        World.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(500), Fixed::FromInt(500)));
    const EntityId NearEnemy =
        World.SpawnUnit(RA4Test::Ids::AllRifleman, 1, Vec2(Fixed::FromInt(700), Fixed::FromInt(500)));
    const EntityId FarEnemy = World.SpawnUnit(RA4Test::Ids::AllRifleman, 1,
                                              Vec2(Fixed::FromInt(15000), Fixed::FromInt(15000)));

    CommandFrame Frame;
    World.Tick(&Frame);

    // Player 0: sees own unit and the adjacent enemy; not the far one.
    RA4_EXPECT(World.IsEntityVisibleTo(0, OwnScout.Index));
    RA4_EXPECT(World.IsEntityVisibleTo(0, NearEnemy.Index));
    RA4_EXPECT(!World.IsEntityVisibleTo(0, FarEnemy.Index));

    // Player 1: both its units are its own -- always visible to itself,
    // including the one player 0 cannot see. Per-viewer, not global.
    RA4_EXPECT(World.IsEntityVisibleTo(1, NearEnemy.Index));
    RA4_EXPECT(World.IsEntityVisibleTo(1, FarEnemy.Index));

    // Out-of-range indices are not visible to anyone -- including in a match
    // without fog, where the helper's range check must fire before its
    // fog-is-optional early-out (review MINOR-2 pinned this ordering).
    RA4_EXPECT(!World.IsEntityVisibleTo(0, World.GetEntityCapacity() + 100));
    {
        SimWorld NoFogCheck; // Initialize builds fog only per map config; default test setup has it,
                             // so exercise the fogless order via an uninitialized world's empty core.
        RA4_EXPECT(!NoFogCheck.IsEntityVisibleTo(0, 5));
    }
}

// DirtyRegions is a producer/consumer list for texture uploads: every reveal appends a rect
// and the consumer drains it. Nothing in the shipping path ever drained it, so the vectors
// grew by one rect per revealing entity per tick and were never freed -- measured at 2400
// rects after 600 ticks with three buildings, for a list nobody reads. Over a half-hour match
// that is roughly 144k rects per player.
//
// Found by an independent reviewer's probe, not by the author of the radar work.
RA4_TEST(FogOfWar, DirtyRegionsDoNotGrowWithoutBound)
{
    ContentDatabase Content;
    RA4Test::BuildDefaultContent(Content);
    SimWorld World;
    World.Initialize(&Content, RA4Test::MakeTestSetup(777));
    RA4Test::SpawnEnemyOutpost(World);
    // Several revealing buildings, since the growth was per revealing entity per tick.
    World.SpawnBuilding(RA4Test::Ids::SovConYard, 0, TileCoord(10, 10), true);
    World.SpawnBuilding(RA4Test::Ids::SovPower, 0, TileCoord(14, 10), true);
    World.SpawnBuilding(RA4Test::Ids::SovRadar, 0, TileCoord(18, 10), true);

    RA4Test::RunTicks(World, 600);

    const FFogOfWarGrid* Fog = World.GetFogGrid();
    RA4_REQUIRE(Fog != nullptr);
    // Bounded by what one tick can produce, not by how long the match has run. The exact
    // count is a few rects; the assertion is deliberately loose so it tests boundedness
    // rather than pinning an implementation detail.
    RA4_EXPECT(Fog->GetDirtyRegions(0).size() < 64u);

    // And it stays bounded: ten times as long must not be ten times as many.
    const size_t After600 = Fog->GetDirtyRegions(0).size();
    RA4Test::RunTicks(World, 600);
    RA4_EXPECT(Fog->GetDirtyRegions(0).size() <= After600 + 8u);
}

// Radius * Radius and the per-cell distance were both computed in int32. Above a radius of
// 46340 that overflows, and signed overflow is undefined behaviour rather than merely a wrong
// answer. No shipping caller passes anything close today, but vision ranges come from content
// and content is data -- data must not be able to make a function undefined.
//
// Honest scope of this test: it pins the *correct* behaviour at a huge radius, and it would
// catch a wrap that produced a visibly wrong result. It does NOT prove the int64 change was
// necessary on this platform -- with int32 restored, arm64 -O2 happens to compute the same
// visible answer, so this test still passes. The overflow itself was demonstrated separately
// with UBSan on the identical arithmetic:
//
//     int32: "runtime error: signed integer overflow: 50000 * 50000 cannot be represented
//             in type 'int'", and the centre cell came out NeverSeen
//     int64: no diagnostic, centre cell CurrentlyVisible
//
// That is why the fix stands even though this test cannot fail without it. Reproducing UBSan
// inside the headless suite would mean a second sanitizer build configuration, which is worth
// doing but is not this change.
RA4_TEST(FogOfWar, HugeRevealRadiusBehavesCorrectly)
{
    FFogOfWarGrid Grid(64, 64, 2);

    // Well past the int32 square-root boundary.
    Grid.RevealCircularArea(0, 32, 32, 50000);
    RA4_EXPECT(Grid.GetVisibility(0, 32, 32) == VisibilityState::CurrentlyVisible);
    // A radius that large covers the whole map, including the far corner.
    RA4_EXPECT(Grid.GetVisibility(0, 0, 0) == VisibilityState::CurrentlyVisible);
    RA4_EXPECT(Grid.GetVisibility(0, 63, 63) == VisibilityState::CurrentlyVisible);

    FFogOfWarGrid RadarGrid(64, 64, 2);
    RadarGrid.RevealRadarArea(0, 32, 32, 50000);
    RA4_EXPECT(RadarGrid.GetVisibility(0, 32, 32) == VisibilityState::RadarDetected);
    RA4_EXPECT(RadarGrid.GetVisibility(0, 0, 0) == VisibilityState::RadarDetected);

    // A negative radius reveals nothing rather than wrapping into a huge one.
    FFogOfWarGrid NegGrid(64, 64, 2);
    NegGrid.RevealCircularArea(0, 32, 32, -5);
    RA4_EXPECT(NegGrid.GetVisibility(0, 32, 32) == VisibilityState::NeverSeen);
    NegGrid.RevealRadarArea(0, 32, 32, -5);
    RA4_EXPECT(NegGrid.GetVisibility(0, 32, 32) == VisibilityState::NeverSeen);
}
