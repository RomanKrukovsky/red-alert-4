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

// --- ADR-0028: fog as texture data ------------------------------------------
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
    // the whole point of the PreviouslySeen tier in ADR-0028's visual contract.
    const uint8_t OldArea = FullRebuild[4 * 16 + 4];
    RA4_EXPECT(OldArea > kFogTexelNeverSeen);
    RA4_EXPECT(OldArea < kFogTexelCurrentlyVisible);
}

RA4_TEST(FogOfWar, FogStrengthFloorIsAContractNotAdvice)
{
    // ADR-0028 section 4: fog strength may be softened for readability but never
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
