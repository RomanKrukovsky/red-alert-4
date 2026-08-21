// Copyright (c) Red Alert 4 project. Tests for Stage 1 Foundation components.
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Core/Fixed.h"
#include "RA4Core/Vector.h"
#include "RA4Core/Checksum.h"
#include "RA4Core/ObjectPool.h"
#include "RA4Simulation/SimSnapshot.h"
#include "RA4Simulation/SimWorld.h"
#include "RA4Presentation/PresentationInterpolator.h"

#include <string>
#include <vector>

using namespace RA4;
using namespace RA4Test;

// --- 1. Soft-Math & Deterministic Math Primitives ---

RA4_TEST(Foundation, FixedMathExtendedPrimitives)
{
    // Sign
    RA4_EXPECT_EQ(FxSign(Fixed::FromInt(42)), 1);
    RA4_EXPECT_EQ(FxSign(Fixed::FromInt(-10)), -1);
    RA4_EXPECT_EQ(FxSign(Fixed::Zero()), 0);

    // Clamp01
    RA4_EXPECT(FxClamp01(Fixed::FromRatio(-1, 2)) == Fixed::Zero());
    RA4_EXPECT(FxClamp01(Fixed::FromRatio(3, 2)) == Fixed::One());
    RA4_EXPECT(FxClamp01(Fixed::FromRatio(1, 2)) == Fixed::FromRatio(1, 2));

    // Lerp
    const Fixed A = Fixed::FromInt(10);
    const Fixed B = Fixed::FromInt(30);
    RA4_EXPECT(FxLerp(A, B, Fixed::Zero()) == A);
    RA4_EXPECT(FxLerp(A, B, Fixed::One()) == B);
    RA4_EXPECT(FxLerp(A, B, Fixed::FromRatio(1, 2)) == Fixed::FromInt(20));
    RA4_EXPECT(FxLerp(A, B, Fixed::FromRatio(1, 4)) == Fixed::FromInt(15));

    // InvSqrt
    RA4_EXPECT(FxInvSqrt(Fixed::Zero()) == Fixed::Zero());
    RA4_EXPECT(FxInvSqrt(Fixed::FromInt(-5)) == Fixed::Zero());
    const Fixed InvSqrt4 = FxInvSqrt(Fixed::FromInt(4));
    RA4_EXPECT_NEAR(InvSqrt4.Raw, Fixed::FromRatio(1, 2).Raw, int64_t(10));
    const Fixed InvSqrt100 = FxInvSqrt(Fixed::FromInt(100));
    RA4_EXPECT_NEAR(InvSqrt100.Raw, Fixed::FromRatio(1, 10).Raw, int64_t(10));

    // Hermite Spline boundary conditions
    const Fixed P0 = Fixed::FromInt(0);
    const Fixed P1 = Fixed::FromInt(100);
    const Fixed M0 = Fixed::FromInt(50);
    const Fixed M1 = Fixed::FromInt(50);

    RA4_EXPECT(FxHermiteSpline(P0, P1, M0, M1, Fixed::Zero()) == P0);
    RA4_EXPECT(FxHermiteSpline(P0, P1, M0, M1, Fixed::One()) == P1);
    const Fixed Mid = FxHermiteSpline(P0, P1, M0, M1, Fixed::FromRatio(1, 2));
    RA4_EXPECT_NEAR(Mid.ToIntRound(), int64_t(50), int64_t(2));
}

RA4_TEST(Foundation, VectorLerpAndHermite)
{
    const Vec2 V0(Fixed::FromInt(0), Fixed::FromInt(10));
    const Vec2 V1(Fixed::FromInt(100), Fixed::FromInt(50));

    const Vec2 LerpMid = Vec2Lerp(V0, V1, Fixed::FromRatio(1, 2));
    RA4_EXPECT_EQ(LerpMid.X.ToIntRound(), int64_t(50));
    RA4_EXPECT_EQ(LerpMid.Y.ToIntRound(), int64_t(30));

    const Vec2 M0(Fixed::FromInt(20), Fixed::FromInt(0));
    const Vec2 M1(Fixed::FromInt(20), Fixed::FromInt(0));
    const Vec2 HermiteMid = Vec2Hermite(V0, V1, M0, M1, Fixed::FromRatio(1, 2));
    RA4_EXPECT_NEAR(HermiteMid.X.ToIntRound(), int64_t(50), int64_t(2));
    RA4_EXPECT_NEAR(HermiteMid.Y.ToIntRound(), int64_t(30), int64_t(2));

    // Cross product
    const Vec2 Right(Fixed::FromInt(1), Fixed::Zero());
    const Vec2 Up(Fixed::Zero(), Fixed::FromInt(1));
    RA4_EXPECT_EQ(Cross(Right, Up).ToIntRound(), int64_t(1));
    RA4_EXPECT_EQ(Cross(Up, Right).ToIntRound(), int64_t(-1));
}

// --- 2. Zero-Allocation Object Pool ---

struct TestPooledEntity
{
    int32_t Value = 0;
    char Tag[16] = {};

    TestPooledEntity() = default;
    explicit TestPooledEntity(int32_t InVal) : Value(InVal) {}
};

RA4_TEST(Foundation, ObjectPoolLifecycle)
{
    constexpr size_t PoolCap = 4;
    TObjectPool<TestPooledEntity, PoolCap> Pool;

    RA4_EXPECT_EQ(Pool.GetCapacity(), PoolCap);
    RA4_EXPECT_EQ(Pool.GetActiveCount(), size_t(0));
    RA4_EXPECT_EQ(Pool.GetFreeCount(), PoolCap);
    RA4_EXPECT(Pool.IsEmpty());
    RA4_EXPECT(!Pool.IsFull());

    // Acquire all 4 slots
    TestPooledEntity* E0 = Pool.Acquire(100);
    TestPooledEntity* E1 = Pool.Acquire(200);
    TestPooledEntity* E2 = Pool.Acquire(300);
    TestPooledEntity* E3 = Pool.Acquire(400);

    RA4_REQUIRE(E0 != nullptr);
    RA4_REQUIRE(E1 != nullptr);
    RA4_REQUIRE(E2 != nullptr);
    RA4_REQUIRE(E3 != nullptr);

    RA4_EXPECT_EQ(E0->Value, 100);
    RA4_EXPECT_EQ(E3->Value, 400);
    RA4_EXPECT(Pool.IsFull());
    RA4_EXPECT_EQ(Pool.GetActiveCount(), PoolCap);

    // 5th acquire must return nullptr
    TestPooledEntity* E4 = Pool.Acquire(500);
    RA4_EXPECT(E4 == nullptr);

    // Release middle slot and reacquire
    const int32_t Idx1 = Pool.GetIndex(E1);
    RA4_EXPECT_EQ(Idx1, 1);
    Pool.Release(E1);
    RA4_EXPECT(!Pool.IsFull());
    RA4_EXPECT_EQ(Pool.GetActiveCount(), size_t(3));

    TestPooledEntity* Reacquired = Pool.Acquire(999);
    RA4_REQUIRE(Reacquired != nullptr);
    RA4_EXPECT_EQ(Reacquired->Value, 999);
    RA4_EXPECT_EQ(Pool.GetIndex(Reacquired), 1);

    // Clear
    Pool.Clear();
    RA4_EXPECT(Pool.IsEmpty());
    RA4_EXPECT_EQ(Pool.GetActiveCount(), size_t(0));
}

// --- 3. Subsystem-Isolated State Hash Breakdown ---

RA4_TEST(Foundation, StateHashBreakdownIsolation)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);

    SimWorld WorldA;
    WorldA.Initialize(&Content, MakeTestSetup(5555));
    WorldA.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);

    SimWorld WorldB;
    WorldB.Initialize(&Content, MakeTestSetup(5555));
    WorldB.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);

    RunTicks(WorldA, 10);
    RunTicks(WorldB, 10);

    const StateHashBreakdown HashA = WorldA.ComputeDetailedChecksum();
    const StateHashBreakdown HashB = WorldB.ComputeDetailedChecksum();

    RA4_EXPECT(HashA == HashB);
    RA4_EXPECT_EQ(HashA.Overall, HashB.Overall);

    char DiffBuf[256] = {};
    const bool bHasDiff = HashA.FindDivergence(HashB, DiffBuf, sizeof(DiffBuf));
    RA4_EXPECT(!bHasDiff);

    // Mutate only WorldB economy (credits)
    WorldB.CheatGrantCredits(0, 5000);

    const StateHashBreakdown HashB_AfterCredit = WorldB.ComputeDetailedChecksum();
    RA4_EXPECT(HashA != HashB_AfterCredit);
    RA4_EXPECT(HashA.Overall != HashB_AfterCredit.Overall);
    RA4_EXPECT(HashA.Economy != HashB_AfterCredit.Economy);
    // Unaffected subsystems must remain identical
    RA4_EXPECT_EQ(HashA.Positions, HashB_AfterCredit.Positions);
    RA4_EXPECT_EQ(HashA.Entities, HashB_AfterCredit.Entities);
    RA4_EXPECT_EQ(HashA.Rng, HashB_AfterCredit.Rng);

    const bool bDetectedDiff = HashA.FindDivergence(HashB_AfterCredit, DiffBuf, sizeof(DiffBuf));
    RA4_EXPECT(bDetectedDiff);
    RA4_EXPECT(std::string(DiffBuf).find("Economy") != std::string::npos);
}

// --- 4. Deterministic Snapshots & Ring Buffer Rollback ---

RA4_TEST(Foundation, SnapshotCaptureAndRollback)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);

    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(9999));
    World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    World.SpawnBuilding(Ids::AllConYard, 1, TileCoord(50, 50), true);
    const EntityId Tank = World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2(Fixed::FromInt(2000), Fixed::FromInt(2000)));
    RA4_REQUIRE(Tank.IsValid());

    // Record 20 ticks of simulation
    for (int32_t I = 0; I < 20; ++I)
    {
        World.Tick(nullptr);
        World.RecordSnapshot();
        World.ClearEvents();
    }

    RA4_EXPECT_EQ(World.GetTick(), 20u);
    const auto& History = World.GetSnapshotHistory();
    RA4_EXPECT_EQ(History.NumSnapshots(), 20u);
    RA4_EXPECT_EQ(History.GetNewestTick(), 20u);
    RA4_EXPECT_EQ(History.GetOldestTick(), 1u);

    // Retrieve snapshot at tick 10
    SimSnapshot Snap10;
    const bool bFound10 = History.GetSnapshot(10, Snap10);
    RA4_REQUIRE(bFound10);
    RA4_EXPECT_EQ(Snap10.Tick, 10u);
    RA4_EXPECT(Snap10.bValid);

    // Capture state at tick 20 for later comparison
    const uint64_t ChecksumAt20 = World.ComputeStateChecksum();

    // Rollback World to tick 10
    const bool bRestored = World.RestoreFromSnapshot(Snap10);
    RA4_REQUIRE(bRestored);
    RA4_EXPECT_EQ(World.GetTick(), 10u);
    RA4_EXPECT_EQ(World.ComputeStateChecksum(), Snap10.Checksum);

    // Re-simulate 10 ticks forward from tick 10 to 20
    for (int32_t I = 0; I < 10; ++I)
    {
        World.Tick(nullptr);
        World.ClearEvents();
    }

    RA4_EXPECT_EQ(World.GetTick(), 20u);
    RA4_EXPECT_EQ(World.ComputeStateChecksum(), ChecksumAt20);
}

// --- 5. Presentation Interpolator ---

RA4_TEST(Foundation, PresentationInterpolatorSmoothing)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);

    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(7777));
    World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    SpawnEnemyOutpost(World, 1);

    const EntityId Tank = World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2(Fixed::FromInt(1000), Fixed::FromInt(1000)), 0);
    RA4_REQUIRE(Tank.IsValid());

    Presentation::PresentationInterpolator Interpolator;
    Interpolator.IngestSimTick(World);

    // Order tank to move to (2000, 1000)
    Command MoveCmd;
    MoveCmd.Type = CommandType::Move;
    MoveCmd.Issuer = 0;
    MoveCmd.Primary = Tank;
    MoveCmd.Location = Vec2(Fixed::FromInt(2000), Fixed::FromInt(1000));
    World.ApplyCommand(MoveCmd);


    // Run 5 ticks
    for (int32_t I = 0; I < 5; ++I)
    {
        World.Tick(nullptr);
        Interpolator.IngestSimTick(World);
        World.ClearEvents();
    }

    std::vector<Presentation::InterpolatedEntityState> States;
    Interpolator.InterpolateAll(0.0f, States);
    RA4_REQUIRE(!States.empty());

    Presentation::InterpolatedEntityState StateT0;
    Presentation::InterpolatedEntityState StateT50;
    Presentation::InterpolatedEntityState StateT100;

    const bool bG0 = Interpolator.GetInterpolatedEntity(Tank, 0.0f, StateT0);
    const bool bG50 = Interpolator.GetInterpolatedEntity(Tank, 0.5f, StateT50);
    const bool bG100 = Interpolator.GetInterpolatedEntity(Tank, 1.0f, StateT100);

    RA4_REQUIRE(bG0 && bG50 && bG100);
    RA4_EXPECT(StateT0.bAlive);
    RA4_EXPECT(StateT50.bAlive);
    RA4_EXPECT(StateT100.bAlive);

    // Interpolation must be smooth and monotonic
    if (StateT100.WorldX > StateT0.WorldX)
    {
        RA4_EXPECT(StateT50.WorldX >= StateT0.WorldX);
        RA4_EXPECT(StateT100.WorldX >= StateT50.WorldX);
    }
}
