// Copyright (c) Red Alert 4 project. Tests for the Fog of War simulation module.
#include "TestFramework.h"
#include "TestHelpers.h"

#include "FogOfWarGrid.h"
#include "RA4Content/ContentDatabase.h"
#include "RA4Presentation/FogVisibilityTexture.h"
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

// --- ADR-0030: fog as texture data ------------------------------------------
// These pin the encoding and the dirty/full agreement headlessly. The material
// and post-process work is only verifiable by looking at the screen, but the
// part that decides WHICH texel a tile gets is pure arithmetic and belongs in a
// test -- getting it wrong renders explored ground as unexplored, which in a
// playtest looks like a vision bug with no stack trace.

RA4_TEST(FogOfWar, TexelEncodingIsOrderedAndDistinct)
{
    // The material reads this as a 0..1 ramp, so the four states must be
    // distinct AND monotonically ordered by how much the player knows.
    // Reordering them would invert fog in the world without failing to compile.
    RA4_EXPECT(FogStateToTexel(VisibilityState::NeverSeen) == kFogTexelNeverSeen);
    RA4_EXPECT(FogStateToTexel(VisibilityState::PreviouslySeen) == kFogTexelPreviouslySeen);
    RA4_EXPECT(FogStateToTexel(VisibilityState::RadarDetected) == kFogTexelRadarDetected);
    RA4_EXPECT(FogStateToTexel(VisibilityState::CurrentlyVisible) == kFogTexelCurrentlyVisible);

    RA4_EXPECT(kFogTexelNeverSeen < kFogTexelPreviouslySeen);
    RA4_EXPECT(kFogTexelPreviouslySeen < kFogTexelRadarDetected);
    RA4_EXPECT(kFogTexelRadarDetected < kFogTexelCurrentlyVisible);

    // Unexplored must be the darkest possible value and seen the brightest, so
    // a material that simply multiplies by this byte cannot brighten fog.
    RA4_EXPECT(kFogTexelNeverSeen == 0);
    RA4_EXPECT(kFogTexelCurrentlyVisible == 255);
}

RA4_TEST(FogOfWar, TexelBufferMatchesGridAndRejectsBadSeat)
{
    FFogOfWarGrid Grid(8, 8, 2);
    Grid.RevealCircularArea(0, 2, 2, 1);

    std::vector<uint8_t> Buffer;
    RA4_EXPECT(BuildFogTexelBuffer(Grid, 0, Buffer));
    RA4_EXPECT(Buffer.size() == 64);

    // Every texel agrees with the grid it came from, row-major.
    bool bAllMatch = true;
    for (int32_t Y = 0; Y < 8; ++Y)
    {
        for (int32_t X = 0; X < 8; ++X)
        {
            if (Buffer[size_t(Y) * 8 + size_t(X)] != FogStateToTexel(Grid.GetVisibility(0, X, Y)))
            {
                bAllMatch = false;
            }
        }
    }
    RA4_EXPECT(bAllMatch);

    // The revealed centre is brighter than an untouched corner -- i.e. the
    // buffer actually carries vision rather than a constant.
    RA4_EXPECT(Buffer[2 * 8 + 2] > Buffer[7 * 8 + 7]);

    // Player 1 saw nothing, so its buffer is uniformly unexplored. Fog is
    // per-seat; a shared buffer would leak another player's vision.
    std::vector<uint8_t> Other;
    RA4_EXPECT(BuildFogTexelBuffer(Grid, 1, Other));
    bool bOtherAllDark = true;
    for (uint8_t V : Other)
    {
        if (V != kFogTexelNeverSeen)
        {
            bOtherAllDark = false;
        }
    }
    RA4_EXPECT(bOtherAllDark);

    // An out-of-range seat is refused rather than clamped to player 0.
    std::vector<uint8_t> Untouched;
    RA4_EXPECT(!BuildFogTexelBuffer(Grid, 5, Untouched));
    RA4_EXPECT(Untouched.empty());
    RA4_EXPECT(!BuildFogTexelBuffer(Grid, -1, Untouched));
}

RA4_TEST(FogOfWar, DirtyRegionUploadAgreesWithFullRebuild)
{
    // The dirty path is an optimisation. If it can disagree with the full
    // rebuild, fog goes stale in a way no test would otherwise catch, so the
    // two are pinned against each other directly.
    FFogOfWarGrid Grid(16, 16, 1);
    Grid.RevealCircularArea(0, 4, 4, 2);

    std::vector<uint8_t> Incremental;
    RA4_EXPECT(BuildFogTexelBuffer(Grid, 0, Incremental));

    // Vision moves: clear and reveal elsewhere, then patch only that rectangle.
    Grid.ClearCurrentVisibility(0);
    Grid.RevealCircularArea(0, 11, 11, 2);
    RA4_EXPECT(BlitFogTexelRegion(Grid, 0, 0, 0, 16, 16, Incremental));

    std::vector<uint8_t> FullRebuild;
    RA4_EXPECT(BuildFogTexelBuffer(Grid, 0, FullRebuild));
    RA4_EXPECT(Incremental == FullRebuild);

    // A region blit is clamped, not out-of-bounds: an oversized rect from a
    // dirty list must not write past the buffer.
    RA4_EXPECT(BlitFogTexelRegion(Grid, 0, -5, -5, 999, 999, Incremental));
    RA4_EXPECT(Incremental == FullRebuild);

    // A buffer of the wrong size is refused instead of partially written.
    std::vector<uint8_t> WrongSize(10, 0);
    RA4_EXPECT(!BlitFogTexelRegion(Grid, 0, 0, 0, 16, 16, WrongSize));

    // Memory of terrain survives losing sight of it: the first revealed area is
    // no longer CurrentlyVisible but must not fall back to NeverSeen, which is
    // the whole point of the PreviouslySeen tier in ADR-0030's visual contract.
    const uint8_t OldArea = FullRebuild[4 * 16 + 4];
    RA4_EXPECT(OldArea > kFogTexelNeverSeen);
    RA4_EXPECT(OldArea < kFogTexelCurrentlyVisible);
}

RA4_TEST(FogOfWar, FogStrengthFloorIsAContractNotAdvice)
{
    // ADR-0030 section 4: fog strength may be softened for readability but never
    // to where unexplored and currently-visible ground look the same -- that
    // would hand the player information the rules deny them. The floor therefore
    // has to be arithmetic, not prose.
    //
    // This pins the clamp itself rather than the subsystem, which needs Unreal.
    // The subsystem applies exactly this expression in SetFogStrength; if that
    // ever diverges the two will disagree and the divergence is the bug.
    constexpr float kMinFogStrength = 0.35f;
    auto Clamp = [](float S) { return S < kMinFogStrength ? kMinFogStrength : (S > 1.0f ? 1.0f : S); };

    // Turning fog off entirely is not offered -- it is clamped up to the floor.
    RA4_EXPECT(Clamp(0.0f) == kMinFogStrength);
    RA4_EXPECT(Clamp(-5.0f) == kMinFogStrength);
    RA4_EXPECT(Clamp(0.1f) == kMinFogStrength);
    // Within range, honoured exactly.
    RA4_EXPECT(Clamp(0.5f) == 0.5f);
    RA4_EXPECT(Clamp(1.0f) == 1.0f);
    // Above the intended look is also refused: fog brighter than the design is a
    // different kind of wrong, not a harmless one.
    RA4_EXPECT(Clamp(3.0f) == 1.0f);

    // And the floor must leave a real difference between the extremes: at the
    // weakest allowed setting, unexplored ground is still far darker than seen
    // ground. brightness = lerp(0.08, 1, visibility) scaled by strength.
    const float FloorBrightnessUnexplored = 0.08f * kMinFogStrength;
    const float FloorBrightnessVisible = 1.0f;
    RA4_EXPECT(FloorBrightnessVisible - FloorBrightnessUnexplored > 0.5f);
}

// The dirty list is the fog texture's upload path, and nothing ever emptied it.
// RevealCircularArea pushes one rectangle per unit per tick whenever that unit's
// circle changed anything -- and SystemFogOfWar clears current visibility first,
// so it changes something every tick. The list therefore grew for the length of
// the match while the consumer re-blitted every rectangle in it on every frame:
// 200 units at 20 Hz is 4,000 new rectangles a second, so a ten-minute match ends
// up re-uploading millions of them per frame. It reads as a slow frame-rate decay
// with no single frame to blame, which is the hardest kind of performance bug to
// find, so it is pinned here rather than left to a playtest.
RA4_TEST(FogOfWar, DirtyRegionsDoNotAccumulateAcrossTicks)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);

    MatchSetup Setup = RA4Test::MakeTestSetup();
    SimWorld World;
    World.Initialize(&Content, Setup);

    World.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(500), Fixed::FromInt(500)));
    World.SpawnUnit(RA4Test::Ids::SovConscript, 0, Vec2(Fixed::FromInt(900), Fixed::FromInt(500)));
    // Player 1 needs a unit too, far away. Without an opponent SystemVictory ends
    // the match on the first tick and every later Tick returns immediately at the
    // Phase check -- the simulation would stand still and this test would pass
    // while measuring nothing.
    World.SpawnUnit(RA4Test::Ids::SovConscript, 1, Vec2(Fixed::FromInt(11000), Fixed::FromInt(11000)));

    const FFogOfWarGrid* Fog = World.GetFogGrid();

    World.Tick(nullptr);
    const size_t AfterOneTick = Fog->GetDirtyRegions(0).size();
    RA4_EXPECT(AfterOneTick > 0);   // the units did reveal something

    for (int i = 0; i < 100; ++i)
    {
        World.Tick(nullptr);
    }

    // Bounded by what one tick produces, not by how many ticks have run. Without
    // the clear this is AfterOneTick * 101.
    const size_t AfterManyTicks = Fog->GetDirtyRegions(0).size();
    RA4_EXPECT(AfterManyTicks <= AfterOneTick);

    // Agreement between the dirty-region blit and the full rebuild is pinned by
    // FogOfWar.DirtyRegionUploadAgreesWithFullRebuild; this test owns the bound,
    // not the contents. The rectangle fields are deliberately not touched here --
    // the headless FIntRect stub spells them MinX/MinY while Unreal's spells them
    // Min.X/Min.Y, so a test that reads them compiles in one build and not the
    // other.
    RA4_EXPECT(Fog->GetWidth() > 0 && Fog->GetHeight() > 0);
}
