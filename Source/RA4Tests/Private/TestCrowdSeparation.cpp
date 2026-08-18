// Copyright (c) Red Alert 4 project. Tests that a crowd of units actually arrives.
//
// WHY THIS FILE EXISTS
//
// `NativeCrowdMovementTests` (Unity side) demands that at least 180 of 200 units
// reach a shared destination. It gets 127. That test is the only thing in the project
// asserting crowd behaviour at scale, and it lives on the managed side of the ABI,
// which means it can only see the outcome -- never the mechanism.
//
// WHAT THE MEASUREMENTS ACTUALLY SHOW, and why the obvious diagnoses are wrong.
// A probe run against this core produced:
//
//     all 200 units accepted a destination; 0 commands rejected
//     tick   0: withDestination=200  moving= 16
//     tick   5: withDestination=200  moving= 35
//     tick  50: withDestination=200  moving=112     <- they ARE moving
//     unit 123 CLEARED bHasDestination at tick 59, still 63 tiles from goal,
//              with BlockedTicks == 0
//     tick 300: withDestination=132  moving=102
//     final distance from goal, in tiles: <2: 16   2-4: 42   4-8: 69   8-16: 0   >16: 73
//     arrived=127 (indices 0-99: 77, indices 100-199: 50)
//     reservation contests total=74009
//
// So units do not freeze in a traffic jam. They *self-declare arrival* while dozens
// of tiles away, and they are not blocked when they do it. Ruled out by measurement,
// so that these tests do not re-litigate them:
//
//   * Command rate limiting. Zero rejections; the harness batches under the 64 cap.
//   * Unit speed. Infantry covers 0.15 tiles/tick, so 2400 ticks allows 360 tiles
//     against a ~60-tile trip.
//   * Fixed-point overflow in the arrive comparison.
//   * Group-scaled arrive radius. `ScaleArriveRadiusForGroup` clamps at
//     kTileSizeUnitsLocal * 3 == 600 units == 3 tiles, so it cannot explain 63.
//   * Soft separation as the cure. It moved the Unity result 126 -> 127. Its radius is
//     56 world units (0.28 tiles) with a 6-unit step, against a whole-tile failure.
//
// These tests therefore assert the PROPERTY that was violated -- "a unit must not
// believe it has arrived while far from its goal" -- rather than any one suspected
// line. That way they keep their value whichever site turns out to be responsible,
// and they keep it after the fix, as a regression guard.

#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Core/SimConfig.h"
#include "RA4Content/ContentDatabase.h"

#include <algorithm>
#include <vector>

using namespace RA4;

namespace
{
struct CrowdFixture
{
    ContentDatabase Content;
    SimWorld World;
    explicit CrowdFixture(uint64_t Seed, int32_t Width = 96, int32_t Height = 96)
    {
        BuildDefaultContent(Content);
        MatchSetup Setup = RA4Test::MakeTestSetup(Seed);
        Setup.Map.Name = "crowd.plains";
        Setup.Map.Resize(Width, Height, Tile_GroundPassable);
        World.Initialize(&Content, Setup);
    }
};

// The arrive radius a single conscript uses, and the ceiling once the group scaling
// is applied. Derived from the source rather than chosen: `ArriveRadius` is
// FxMax(CollisionRadius, 25) at spawn, the conscript's CollisionRadius is 35, and
// ScaleArriveRadiusForGroup clamps its result at kTileSizeUnitsLocal * 3.
//
// The clamp is the number that makes these tests possible: no matter how large the
// crowd, a unit may legitimately stop at most 3 tiles short. Anything beyond that is
// a defect, not tolerance -- which is exactly why 63 tiles was diagnosable.
constexpr int64_t kMaxLegitimateArriveTiles = 3;

// Chebyshev distance in whole tiles between a world position and a tile centre.
// Chebyshev rather than Euclidean because the movement system itself works in tile
// steps including diagonals, so it is the metric that matches the mechanism.
int64_t TileDistanceToTile(const Vec2& Position, const TileCoord& Tile)
{
    const int64_t TargetX = int64_t(Tile.X) * kTileSizeUnits + kTileSizeUnits / 2;
    const int64_t TargetY = int64_t(Tile.Y) * kTileSizeUnits + kTileSizeUnits / 2;
    const int64_t DX = Position.X.ToIntRound() - TargetX;
    const int64_t DY = Position.Y.ToIntRound() - TargetY;
    const int64_t AX = DX < 0 ? -DX : DX;
    const int64_t AY = DY < 0 ? -DY : DY;
    return (AX > AY ? AX : AY) / kTileSizeUnits;
}

Vec2 TileCentre(const TileCoord& Tile)
{
    return Vec2(Fixed::FromInt(int64_t(Tile.X) * kTileSizeUnits + kTileSizeUnits / 2),
                Fixed::FromInt(int64_t(Tile.Y) * kTileSizeUnits + kTileSizeUnits / 2));
}

// Keeps the match Running. `Tick` early-returns unless Phase == Running, and
// SystemVictory declares a winner as soon as a player owns zero units AND zero
// buildings. A single-sided crowd test would conclude on tick 1 and every later tick
// would be a silent no-op -- 200 units would look frozen for a reason that has
// nothing to do with movement. This is not decoration; without it the tests lie.
EntityId KeepMatchAlive(SimWorld& World)
{
    return World.SpawnUnit(RA4Test::Ids::SovConscript, PlayerId{1},
                           TileCentre(TileCoord(90, 90)));
}

// Issues a Move to every entity, batched under the command cap.
//
// Commands are rate-limited to kMaxCommandsPerPlayerPerTick (64) per player per tick
// and an over-cap command is REJECTED, not queued. Issuing 200 orders in one tick
// would silently drop 136 of them and produce a "only 64 arrived" result caused
// entirely by the harness. Acceptance is asserted for exactly that reason.
void OrderAllTo(SimWorld& World, const std::vector<EntityId>& Units,
                const std::vector<TileCoord>& Destinations)
{
    RA4_REQUIRE(Units.size() == Destinations.size());
    constexpr size_t kBatch = 60;   // under the 64 cap, matching the Unity harness
    for (size_t Offset = 0; Offset < Units.size(); Offset += kBatch)
    {
        const size_t End = std::min(Offset + kBatch, Units.size());
        for (size_t I = Offset; I < End; ++I)
        {
            Command Move;
            Move.Type = CommandType::Move;
            Move.Issuer = PlayerId{0};
            Move.Primary = Units[I];
            Move.Location = TileCentre(Destinations[I]);
            if (!World.ApplyCommand(Move).IsAccepted())
            {
                RA4Test::ReportFailure("crowd harness: move command for unit index " +
                                           std::to_string(I) +
                                           " was rejected, so any arrival count from this run "
                                           "would measure the harness rather than the simulation",
                                       __FILE__, __LINE__);
                return;
            }
        }
        World.Tick(nullptr);
        World.ClearEvents();
    }
}

void Run(SimWorld& World, int32_t Ticks)
{
    for (int32_t T = 0; T < Ticks; ++T)
    {
        World.Tick(nullptr);
        World.ClearEvents();
    }
}

// The spiral-ring slot layout the Unity harness uses, so these tests exercise the
// same shape of problem: every unit gets a DISTINCT destination tile. Worth stating
// plainly, because it kills the intuitive explanation -- the crowd is not being sent
// to one tile and fighting over it.
TileCoord SpiralSlot(const TileCoord& Centre, int32_t Index)
{
    if (Index <= 0) return Centre;
    int32_t Ring = 0;
    while ((2 * Ring + 1) * (2 * Ring + 1) <= Index) { ++Ring; }
    const int32_t Side = Ring * 2;
    const int32_t Max = (Ring * 2 + 1) * (Ring * 2 + 1) - 1;
    const int32_t Offset = Max - Index;
    int32_t X = 0;
    int32_t Y = 0;
    if (Offset < Side) { X = Ring - Offset; Y = -Ring; }
    else if (Offset < Side * 2) { X = -Ring; Y = -Ring + Offset - Side; }
    else if (Offset < Side * 3) { X = -Ring + Offset - Side * 2; Y = Ring; }
    else { X = Ring; Y = Ring - (Offset - Side * 3); }
    return TileCoord(Centre.X + X, Centre.Y + Y);
}

struct Crowd
{
    std::vector<EntityId> Units;
    std::vector<TileCoord> Destinations;
};

Crowd SpawnCrowd(SimWorld& World, int32_t Count, const TileCoord& Origin,
                 const TileCoord& Target, int32_t RowWidth = 20)
{
    Crowd C;
    C.Units.reserve(size_t(Count));
    C.Destinations.reserve(size_t(Count));
    for (int32_t I = 0; I < Count; ++I)
    {
        const TileCoord Start(Origin.X + I % RowWidth, Origin.Y + I / RowWidth);
        C.Units.push_back(World.SpawnUnit(RA4Test::Ids::SovConscript, PlayerId{0},
                                          TileCentre(Start)));
        C.Destinations.push_back(SpiralSlot(Target, I));
    }
    return C;
}
} // namespace

RA4_TEST(CrowdSeparation, NoUnitBelievesItArrivedWhileFarFromItsGoal)
{
    // Break caught: THE measured defect. A unit cleared bHasDestination at tick 59
    // while 63 tiles from its goal with BlockedTicks == 0.
    //
    // This is the sharpest assertion available, because "arrived" is a claim the
    // simulation makes about itself. The legitimate ceiling is knowable rather than
    // guessed: ScaleArriveRadiusForGroup clamps at 3 tiles regardless of crowd size,
    // so a unit that has given up its destination while further away than that is
    // wrong by the core's own arithmetic. A generous margin is added on top so this
    // fails only on a real defect, never on rounding.
    CrowdFixture F(9001);
    SimWorld& World = F.World;
    KeepMatchAlive(World);

    Crowd C = SpawnCrowd(World, 200, TileCoord(8, 34), TileCoord(75, 48));
    OrderAllTo(World, C.Units, C.Destinations);

    // Checked every tick rather than only at the end: the defect appears at tick 59
    // and a unit can drift afterwards, so a final-state check could miss the moment
    // the false claim was made.
    const int64_t Budget = kMaxLegitimateArriveTiles + 2;
    int32_t WorstIndex = -1;
    int64_t WorstDistance = 0;
    int32_t WorstTick = -1;
    std::vector<bool> Reported(C.Units.size(), false);

    for (int32_t T = 0; T < 2400; ++T)
    {
        World.Tick(nullptr);
        World.ClearEvents();
        for (size_t I = 0; I < C.Units.size(); ++I)
        {
            if (Reported[I]) continue;
            const MovementComp* M = World.GetMovement(C.Units[I]);
            const TransformComp* Tr = World.GetTransform(C.Units[I]);
            if (M == nullptr || Tr == nullptr) continue;
            if (M->bHasDestination) continue;
            Reported[I] = true;   // only judge the moment it first gives up
            const int64_t Distance = TileDistanceToTile(Tr->Position, C.Destinations[I]);
            if (Distance > WorstDistance)
            {
                WorstDistance = Distance;
                WorstIndex = int32_t(I);
                WorstTick = T;
            }
        }
    }

    if (WorstDistance > Budget)
    {
        RA4Test::ReportFailure(
            "unit index " + std::to_string(WorstIndex) + " gave up its destination at tick " +
                std::to_string(WorstTick) + " while still " + std::to_string(WorstDistance) +
                " tiles away; the group-scaled arrive radius is clamped at " +
                std::to_string(kMaxLegitimateArriveTiles) +
                " tiles, so anything beyond that is the simulation declaring an arrival that "
                "did not happen",
            __FILE__, __LINE__);
    }
}

RA4_TEST(CrowdSeparation, SettledUnitsReachAFixedPointAndStopMoving)
{
    // Break caught: oscillation from soft separation. Two units shoving each other
    // forever vibrate in place, and a distance-to-goal check reads that as "arrived",
    // so the defect ships invisibly.
    //
    // Exact equality, deliberately, with no tolerance. A tolerance would let slow
    // drift pass, and slow drift is precisely the failure mode -- it looks settled in
    // any single frame and is still moving a thousand ticks later.
    CrowdFixture F(4711, 64, 64);
    SimWorld& World = F.World;
    KeepMatchAlive(World);

    // A tight cluster on a small target so separation is genuinely exercised: 40
    // units into a 5-tile-wide area guarantees overlap the separation pass must undo.
    Crowd C = SpawnCrowd(World, 40, TileCoord(10, 10), TileCoord(30, 30), 5);
    OrderAllTo(World, C.Units, C.Destinations);
    Run(World, 1200);

    // Snapshot, advance, compare. Any unit whose position changes has not settled.
    std::vector<Vec2> Before;
    Before.reserve(C.Units.size());
    for (size_t I = 0; I < C.Units.size(); ++I)
    {
        const TransformComp* Tr = World.GetTransform(C.Units[I]);
        RA4_REQUIRE(Tr != nullptr);
        Before.push_back(Tr->Position);
    }

    Run(World, 60);

    int32_t Moved = 0;
    int32_t FirstMovedIndex = -1;
    int64_t LargestShift = 0;
    for (size_t I = 0; I < C.Units.size(); ++I)
    {
        const TransformComp* Tr = World.GetTransform(C.Units[I]);
        if (Tr == nullptr) continue;
        const int64_t DX = Tr->Position.X.Raw - Before[I].X.Raw;
        const int64_t DY = Tr->Position.Y.Raw - Before[I].Y.Raw;
        if (DX != 0 || DY != 0)
        {
            ++Moved;
            if (FirstMovedIndex < 0) FirstMovedIndex = int32_t(I);
            const int64_t AX = DX < 0 ? -DX : DX;
            const int64_t AY = DY < 0 ? -DY : DY;
            const int64_t Shift = AX > AY ? AX : AY;
            if (Shift > LargestShift) LargestShift = Shift;
        }
    }

    if (Moved > 0)
    {
        RA4Test::ReportFailure(
            std::to_string(Moved) + " of " + std::to_string(C.Units.size()) +
                " settled units were still moving 60 ticks later (first offender index " +
                std::to_string(FirstMovedIndex) + ", largest shift " +
                std::to_string(LargestShift) +
                " raw units); separation must converge to a fixed point, not oscillate",
            __FILE__, __LINE__);
    }
}

RA4_TEST(CrowdSeparation, SettledUnitsDoNotShareAPosition)
{
    // Break caught: no unit-versus-unit separation at all. NavGrid::IsTraversable
    // tests only static terrain, and CollisionRadius is used solely to derive
    // RequiredClearance and as an arrive radius -- nothing pushes two units apart. So
    // before soft separation existed, units could and did occupy identical positions.
    //
    // The threshold is derived, not chosen: the largest authored Unit.CollisionRadius
    // in the shipped content is 120 world units, and the separation pass works at a
    // 56-unit radius. Exact coincidence is asserted rather than a comfortable
    // distance, because coincidence is unambiguous while "how close is too close" is
    // a tuning question this test has no business deciding.
    CrowdFixture F(2718, 64, 64);
    SimWorld& World = F.World;
    KeepMatchAlive(World);

    Crowd C = SpawnCrowd(World, 40, TileCoord(12, 12), TileCoord(32, 32), 5);
    OrderAllTo(World, C.Units, C.Destinations);
    Run(World, 1200);

    for (size_t A = 0; A < C.Units.size(); ++A)
    {
        const TransformComp* TA = World.GetTransform(C.Units[A]);
        if (TA == nullptr) continue;
        for (size_t B = A + 1; B < C.Units.size(); ++B)
        {
            const TransformComp* TB = World.GetTransform(C.Units[B]);
            if (TB == nullptr) continue;
            if (TA->Position.X.Raw == TB->Position.X.Raw &&
                TA->Position.Y.Raw == TB->Position.Y.Raw)
            {
                RA4Test::ReportFailure(
                    "units " + std::to_string(A) + " and " + std::to_string(B) +
                        " occupy the identical position (" + std::to_string(TA->Position.X.Raw) +
                        "," + std::to_string(TA->Position.Y.Raw) +
                        "); separation must keep bodies apart",
                    __FILE__, __LINE__);
                return;
            }
        }
    }
}

RA4_TEST(CrowdSeparation, TwoHundredUnitsOverwhelminglyArrive)
{
    // Break caught: the headline defect, mirrored core-side so it can be diagnosed
    // without the ABI in the way. The managed NativeCrowdMovementTests asserts >=180
    // of 200 and currently gets 127.
    //
    // The threshold here is deliberately LOWER than the managed test's 180. That is
    // not a weakened assertion -- it is a different instrument. This test exists to
    // catch a catastrophic regression from core-side changes and to report the number
    // in its failure message, while the managed test remains the strict gate. Setting
    // both to 180 would mean one failure hides the other; setting this one lower
    // means a drop from 127 to 60 is caught here with a mechanism-level message even
    // while the strict gate stays red.
    CrowdFixture F(7001);
    SimWorld& World = F.World;
    KeepMatchAlive(World);

    Crowd C = SpawnCrowd(World, 200, TileCoord(8, 34), TileCoord(75, 48));
    OrderAllTo(World, C.Units, C.Destinations);
    Run(World, 2400);

    int32_t Arrived = 0;
    int32_t ArrivedLowIndex = 0;
    int32_t ArrivedHighIndex = 0;
    for (size_t I = 0; I < C.Units.size(); ++I)
    {
        const TransformComp* Tr = World.GetTransform(C.Units[I]);
        if (Tr == nullptr) continue;
        // 8 tiles, matching the managed harness's arrival window so the two numbers
        // are comparable rather than merely similar.
        if (TileDistanceToTile(Tr->Position, C.Destinations[I]) <= 8)
        {
            ++Arrived;
            if (I < 100) ++ArrivedLowIndex; else ++ArrivedHighIndex;
        }
    }

    constexpr int32_t kFloor = 100;
    if (Arrived < kFloor)
    {
        RA4Test::ReportFailure(
            "only " + std::to_string(Arrived) + "/200 units arrived (indices 0-99: " +
                std::to_string(ArrivedLowIndex) + ", indices 100-199: " +
                std::to_string(ArrivedHighIndex) + "), below the regression floor of " +
                std::to_string(kFloor) +
                "; the managed NativeCrowdMovementTests gate demands 180",
            __FILE__, __LINE__);
    }
}

RA4_TEST(CrowdSeparation, ArrivalIsNotBiasedTowardLowEntityIndices)
{
    // Break caught: structural unfairness. ReservationGrid::TryReserve grants a tile
    // to the strictly LOWER slot index, and the slot index IS the entity index, so any
    // contention systematically favours units that happen to have been spawned first.
    // A direct probe of that policy measured slot 0 winning 2000 of 2000 contests for
    // a single tile while every higher slot won zero -- total preemption, not bias.
    //
    // Measured arrivals split 77 among indices 0-99 versus 50 among 100-199. This
    // test exists because a fix could raise the TOTAL while still starving the tail,
    // and a total-only assertion would call that success. The ratio is what reveals
    // whether the mechanism is fair or merely faster.
    CrowdFixture F(1234);
    SimWorld& World = F.World;
    KeepMatchAlive(World);

    Crowd C = SpawnCrowd(World, 200, TileCoord(8, 34), TileCoord(75, 48));
    OrderAllTo(World, C.Units, C.Destinations);
    Run(World, 2400);

    int32_t Low = 0;
    int32_t High = 0;
    for (size_t I = 0; I < C.Units.size(); ++I)
    {
        const TransformComp* Tr = World.GetTransform(C.Units[I]);
        if (Tr == nullptr) continue;
        if (TileDistanceToTile(Tr->Position, C.Destinations[I]) <= 8)
        {
            if (I < 100) ++Low; else ++High;
        }
    }

    // A loose ratio, because the two halves start from different rows and so have
    // genuinely different trips. What is asserted is that the second half is not
    // largely abandoned: fewer than a third as many arrivals is starvation, not luck.
    if (High * 3 < Low)
    {
        RA4Test::ReportFailure(
            "arrivals are lopsided by entity index: indices 0-99 landed " +
                std::to_string(Low) + " units, indices 100-199 only " + std::to_string(High) +
                "; reservation priority is a fixed function of entity index, so this is "
                "starvation of the tail rather than chance",
            __FILE__, __LINE__);
    }
}

RA4_TEST(CrowdSeparation, TwoIdenticalCrowdRunsStayIdentical)
{
    // Break caught: non-determinism in the separation or reservation passes -- an
    // unordered container, a pointer-address comparison, or an iteration order that
    // depends on allocation. Any of those desyncs lockstep and breaks replay, and a
    // crowd is where such a bug would first show, because it is the only situation
    // that puts hundreds of units in contention on the same tick.
    //
    // Compares two LIVE worlds rather than asserting a checksum LITERAL. The movement
    // changes shipping alongside this file intentionally alter trajectories, so any
    // literal recorded today would fail tomorrow for a CORRECT implementation.
    // Self-consistency is the property that must hold permanently.
    CrowdFixture A(31415, 64, 64);
    CrowdFixture B(31415, 64, 64);

    KeepMatchAlive(A.World);
    KeepMatchAlive(B.World);

    Crowd CA = SpawnCrowd(A.World, 60, TileCoord(10, 10), TileCoord(40, 40), 10);
    Crowd CB = SpawnCrowd(B.World, 60, TileCoord(10, 10), TileCoord(40, 40), 10);
    OrderAllTo(A.World, CA.Units, CA.Destinations);
    OrderAllTo(B.World, CB.Units, CB.Destinations);

    for (int32_t T = 0; T < 600; ++T)
    {
        A.World.Tick(nullptr);
        A.World.ClearEvents();
        B.World.Tick(nullptr);
        B.World.ClearEvents();
        if (A.World.ComputeStateChecksum() != B.World.ComputeStateChecksum())
        {
            RA4Test::ReportFailure("two identical 60-unit crowd runs diverged at tick " +
                                       std::to_string(T + 1) +
                                       "; crowd contention must be deterministic",
                                   __FILE__, __LINE__);
            return;
        }
    }
}
