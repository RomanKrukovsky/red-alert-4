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
                                              Vec2(Fixed::FromInt(12000), Fixed::FromInt(12000)));
    // 12000 / kTileSizeUnits(200) = tile 60, INSIDE the 64x64 test map. 15000 is
    // tile 75 -- off the map, where the fog grid answers NeverSeen from its bounds
    // guard, so the assertion would pass without fog being involved at all.

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

RA4_TEST(FogOfWar, LocationVisibilityGateMatchesEntityGate)
{
    // V-F pin: combat events carry a location, not a live entity (the shooter can
    // be dead by the time presentation reads the event), so tracers and impact
    // markers gate on IsLocationVisibleTo. This pins that the location overload
    // agrees with the entity one wherever both apply -- otherwise the two fog
    // gates could drift and one surface would leak while the other did not.
    ContentDatabase Content;
    BuildDefaultContent(Content);

    SimWorld World;
    World.Initialize(&Content, RA4Test::MakeTestSetup());

    // 12000 / kTileSizeUnits(200) = tile 60, inside the 64x64 test map. An
    // earlier draft used 15000 -> tile 75, which is OFF the map: the fog grid
    // answers NeverSeen for out-of-bounds tiles, so the location gate closed
    // while the entity gate stayed open on its own-unit short-circuit. That
    // divergence is correct behaviour for both, but it made the test assert the
    // wrong thing -- keep fogged fixtures on the map.
    const Vec2 SeenPos(Fixed::FromInt(500), Fixed::FromInt(500));
    const Vec2 FoggedPos(Fixed::FromInt(12000), Fixed::FromInt(12000));

    World.SpawnUnit(RA4Test::Ids::SovConscript, 0, SeenPos);
    const EntityId NearEnemy = World.SpawnUnit(RA4Test::Ids::AllRifleman, 1,
                                              Vec2(Fixed::FromInt(700), Fixed::FromInt(500)));
    const EntityId FarEnemy = World.SpawnUnit(RA4Test::Ids::AllRifleman, 1, FoggedPos);

    CommandFrame Frame;
    World.Tick(&Frame);

    // Where player 0 has vision, the location gate opens; where it does not, it closes.
    RA4_EXPECT(World.IsLocationVisibleTo(0, SeenPos));
    RA4_EXPECT(!World.IsLocationVisibleTo(0, FoggedPos));

    // And it agrees with the entity gate for entities standing on those tiles --
    // the property that keeps the tracer gate and the actor gate consistent.
    const TransformComp* NearT = World.GetTransform(NearEnemy);
    const TransformComp* FarT = World.GetTransform(FarEnemy);
    RA4_EXPECT(NearT != nullptr && FarT != nullptr);
    RA4_EXPECT(World.IsLocationVisibleTo(0, NearT->Position) ==
               World.IsEntityVisibleTo(0, NearEnemy.Index));
    RA4_EXPECT(World.IsLocationVisibleTo(0, FarT->Position) ==
               World.IsEntityVisibleTo(0, FarEnemy.Index));

    // Player 1 sees its own far unit's tile; the gate is per viewer, not global.
    RA4_EXPECT(World.IsLocationVisibleTo(1, FoggedPos));

    // Documented asymmetry: the entity gate short-circuits on ownership ("a side
    // always sees its own"), while a LOCATION has no owner, so the location gate
    // answers purely from the fog grid. Off-map points are therefore never
    // visible to anyone, even the owner of a unit standing there. V-F's callers
    // only ever pass in-bounds event locations, so this cannot hide a legitimate
    // tracer -- but the two helpers are not interchangeable and this pins why.
    const Vec2 OffMap(Fixed::FromInt(15000), Fixed::FromInt(15000));
    RA4_EXPECT(!World.IsLocationVisibleTo(1, OffMap));
}
