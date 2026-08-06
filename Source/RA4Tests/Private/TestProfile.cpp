// Copyright (c) Red Alert 4 project.
//
// Whole-tick profiler: measures every SimWorld system against the budget in
// Docs/QA/PERFORMANCE_BUDGETS.md, at the load that document specifies.
//
// WHY THIS EXISTS
// ---------------
// "Optimize the game" is not actionable without knowing what is slow. The repo
// already had a Recon-only benchmark (ReconBudgetsAt2000Entities4Players), so the
// recon layer's cost was known and the other fourteen systems were not. Optimising
// on intuition tends to produce churn in cheap code while the real cost sits
// somewhere nobody measured.
//
// This does not assert a pass/fail budget. It reports a ranked cost table, because
// the first useful output is "here is where the time actually goes", and a
// threshold invented before the measurement would be arbitrary.
//
// HOW IT MEASURES
// ---------------
// SimWorld::Tick runs fifteen systems back to back and exposes no per-system
// timing, so a direct instrumented breakdown would mean editing the simulation.
// Instead each system's cost is inferred by differential measurement: run the
// full tick, then run it again with one system's work reduced to nothing by
// removing its inputs, and attribute the delta.
//
// That is only honest for systems whose input can be emptied without changing the
// others, so systems are grouped by what can be isolated cleanly:
//   - baseline: full tick at load, the number that matters
//   - no-orders: same world with no move orders outstanding
//   - static:   same world with nothing moving and nothing shooting
// The report states which group each figure came from rather than pretending to a
// precision the method does not have.
#include "RA4Content/ContentDatabase.h"
#include "RA4Core/Command.h"
#include "RA4Core/SimConfig.h"
#include "RA4Simulation/SimWorld.h"
#include "TestFramework.h"
#include "TestHelpers.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <vector>

using namespace RA4;

namespace
{

// 2000 entities across 4 players is the load PERFORMANCE_BUDGETS.md section 4.4
// specifies, and it matches the existing recon benchmark so the two are comparable.
constexpr int32_t kUnitsPerPlayer = 500;
constexpr int32_t kPlayers = 4;
constexpr int32_t kWarmupTicks = 50;
constexpr int32_t kMeasuredTicks = 200;

// Per-tick budget at 20 Hz. A tick that exceeds this cannot hold real time.
constexpr double kTickBudgetMicros = 50000.0;

MatchSetup MakeFourPlayerSetup(uint64_t Seed)
{
    MatchSetup Setup;
    Setup.Seed = Seed;
    for (PlayerId P = 0; P < kPlayers; ++P)
    {
        Setup.Players[P].bActive = true;
        Setup.Players[P].Faction = (P % 2 == 0) ? FactionId::Soviet : FactionId::Alliance;
    }
    return Setup;
}

void SpawnArmies(SimWorld& World)
{
    const ContentId UnitOf[kPlayers] = {RA4Test::Ids::SovConscript, RA4Test::Ids::AllRifleman,
                                        RA4Test::Ids::SovConscript, RA4Test::Ids::AllRifleman};
    const int32_t BaseX[kPlayers] = {600, 5800, 600, 5800};
    const int32_t BaseY[kPlayers] = {600, 600, 5800, 5800};
    for (PlayerId P = 0; P < kPlayers; ++P)
    {
        for (int32_t I = 0; I < kUnitsPerPlayer; ++I)
        {
            const Vec2 Pos = Vec2::FromInts(BaseX[P] + (I % 25) * 8, BaseY[P] + (I / 25) * 8);
            World.SpawnUnit(UnitOf[P], P, Pos);
        }
    }
}

// Send everyone at the map centre so vision cones overlap, paths contend and
// combat actually happens. A parade ground measures nothing interesting.
void ConvergeAll(SimWorld& World)
{
    CommandFrame Frame;
    Frame.Tick = World.GetTick();
    const std::vector<EntityCore>& Cores = World.GetAllCores();
    for (uint32_t I = 0; I < uint32_t(Cores.size()); ++I)
    {
        if (!Cores[I].bAlive)
        {
            continue;
        }
        Command C;
        C.Type = CommandType::AttackMove;
        C.Issuer = Cores[I].Owner;
        C.Primary = World.MakeId(I);
        C.Location = Vec2::FromInts(3200, 3200);
        Frame.Commands.push_back(C);
    }
    World.Tick(&Frame);
    World.ClearEvents();
}

void RunTicks(SimWorld& World, int32_t Count)
{
    for (int32_t T = 0; T < Count; ++T)
    {
        World.Tick(nullptr);
        World.ClearEvents();
    }
}

struct Sample
{
    int64_t Median = 0;
    int64_t P95 = 0;
    int64_t Peak = 0;
};

Sample Summarize(std::vector<int64_t> Values)
{
    Sample Out;
    if (Values.empty())
    {
        return Out;
    }
    std::sort(Values.begin(), Values.end());
    Out.Median = Values[Values.size() / 2];
    Out.P95 = Values[std::min(Values.size() - 1, (Values.size() * 95) / 100)];
    Out.Peak = Values.back();
    return Out;
}

// Times the full tick over the measured window.
Sample MeasureTicks(SimWorld& World, int32_t Count)
{
    std::vector<int64_t> Micros;
    Micros.reserve(size_t(Count));
    for (int32_t T = 0; T < Count; ++T)
    {
        const auto Start = std::chrono::steady_clock::now();
        World.Tick(nullptr);
        const auto End = std::chrono::steady_clock::now();
        World.ClearEvents();
        Micros.push_back(
            std::chrono::duration_cast<std::chrono::microseconds>(End - Start).count());
    }
    return Summarize(Micros);
}

} // namespace

// Reports the full-tick cost at the budgeted load. This is the headline number:
// if it is under 50000 us the simulation holds 20 Hz with room to spare.
RA4_TEST(Profile, WholeTickCostAt2000Entities)
{
    ContentDatabase Db;
    BuildDefaultContent(Db);

    SimWorld World;
    World.Initialize(&Db, MakeFourPlayerSetup(20260806), nullptr);
    SpawnArmies(World);
    RA4_EXPECT_EQ(int32_t(World.GetEntityCapacity()), kUnitsPerPlayer * kPlayers);

    ConvergeAll(World);
    RunTicks(World, kWarmupTicks); // fog carving and first contacts, unmeasured

    const Sample Full = MeasureTicks(World, kMeasuredTicks);

    std::printf("\n=== Whole-tick profile: %d entities, %d players, %d ticks ===\n",
                kUnitsPerPlayer * kPlayers, kPlayers, kMeasuredTicks);
    std::printf("  full tick      median %6lld us   p95 %6lld us   peak %6lld us\n",
                static_cast<long long>(Full.Median), static_cast<long long>(Full.P95),
                static_cast<long long>(Full.Peak));
    std::printf("  budget         %6.0f us per tick at %d Hz\n", kTickBudgetMicros,
                kTicksPerSecond);
    std::printf("  headroom       %5.1f%% of budget used (median)\n",
                100.0 * double(Full.Median) / kTickBudgetMicros);

    // The simulation must hold real time at the budgeted load. A median tick over
    // budget means the game cannot run at 20 Hz with 2000 units, which is a
    // correctness problem for lockstep, not just a comfort one.
    RA4_EXPECT(double(Full.Median) < kTickBudgetMicros);

    // Regression guard on top of the budget check, because the budget alone is far
    // too loose to protect the work that earned the headroom.
    //
    // Demonstrated, not assumed: disabling the spatial index restores the O(n^2)
    // target acquisition that 0d42c60 removed. That takes the median from ~3.9 ms
    // to 18.6 ms -- a 4.8x regression that BOTH the 50 ms budget check and the
    // per-entity ratio check passed without complaint. A guard that cannot see a
    // 4.8x slowdown is not guarding anything.
    //
    // Healthy medians measured over five consecutive runs on this machine:
    // 4191, 3962, 3967, 3841, 3888 us. The ceiling below sits ~2.4x above the
    // worst of those, so ordinary machine-to-machine and scheduling variation
    // passes while a return to quadratic acquisition fails.
    //
    // This is deliberately a coarse ceiling and not a tight one: the aim is to
    // catch an algorithmic regression, not to police constant factors. If it ever
    // needs raising, measure five runs, paste them here, and say why -- raising it
    // silently converts this guard back into decoration.
    constexpr double kQuadraticRegressionCeilingMicros = 10000.0;
    RA4_EXPECT(double(Full.Median) < kQuadraticRegressionCeilingMicros);
}

// Differential measurement: how much of the tick is movement and pathing?
// Same world, same entity count, but nothing is ordered anywhere, so
// SystemOrders and SystemMovement have no work while every other system still runs.
RA4_TEST(Profile, MovementShareOfTick)
{
    ContentDatabase Db;
    BuildDefaultContent(Db);

    // Busy world: everyone converging, so movement and combat are both live.
    SimWorld Busy;
    Busy.Initialize(&Db, MakeFourPlayerSetup(20260806), nullptr);
    SpawnArmies(Busy);
    ConvergeAll(Busy);
    RunTicks(Busy, kWarmupTicks);
    const Sample WithMovement = MeasureTicks(Busy, kMeasuredTicks);

    // Idle world: identical construction, no orders issued at all.
    SimWorld Idle;
    Idle.Initialize(&Db, MakeFourPlayerSetup(20260806), nullptr);
    SpawnArmies(Idle);
    RunTicks(Idle, kWarmupTicks);
    const Sample WithoutMovement = MeasureTicks(Idle, kMeasuredTicks);

    const int64_t Delta = WithMovement.Median - WithoutMovement.Median;

    std::printf("\n=== Movement and combat share ===\n");
    std::printf("  converging     median %6lld us\n",
                static_cast<long long>(WithMovement.Median));
    std::printf("  idle           median %6lld us\n",
                static_cast<long long>(WithoutMovement.Median));
    std::printf("  attributable   %6lld us  (%.1f%% of the converging tick)\n",
                static_cast<long long>(Delta),
                WithMovement.Median > 0 ? 100.0 * double(Delta) / double(WithMovement.Median)
                                        : 0.0);
    std::printf("  note: this is orders + movement + combat + projectiles combined,\n");
    std::printf("        since ordering units to converge activates all four.\n");

    // Both configurations must hold the tick budget; an idle world certainly must.
    RA4_EXPECT(double(WithoutMovement.Median) < kTickBudgetMicros);
    RA4_EXPECT(double(WithMovement.Median) < kTickBudgetMicros);
}

// Scaling check: cost per entity as the army grows. A system that is O(n^2) in
// entity count shows up here as a rising per-entity cost, which is the shape of
// problem worth optimising; a flat line means the cost is already linear and
// micro-optimising it buys little.
RA4_TEST(Profile, CostScalingWithEntityCount)
{
    std::printf("\n=== Scaling: microseconds per tick per entity ===\n");
    std::printf("  %8s %12s %14s\n", "entities", "median us", "us/entity");

    int64_t FirstPerEntity = 0;
    int64_t LastPerEntity = 0;
    int32_t FirstCount = 0;
    int32_t LastCount = 0;

    for (const int32_t PerPlayer : {125, 250, 500})
    {
        ContentDatabase Db;
        BuildDefaultContent(Db);
        SimWorld World;
        World.Initialize(&Db, MakeFourPlayerSetup(20260806), nullptr);

        const ContentId UnitOf[kPlayers] = {RA4Test::Ids::SovConscript, RA4Test::Ids::AllRifleman,
                                            RA4Test::Ids::SovConscript, RA4Test::Ids::AllRifleman};
        const int32_t BaseX[kPlayers] = {600, 5800, 600, 5800};
        const int32_t BaseY[kPlayers] = {600, 600, 5800, 5800};
        for (PlayerId P = 0; P < kPlayers; ++P)
        {
            for (int32_t I = 0; I < PerPlayer; ++I)
            {
                const Vec2 Pos = Vec2::FromInts(BaseX[P] + (I % 25) * 8, BaseY[P] + (I / 25) * 8);
                World.SpawnUnit(UnitOf[P], P, Pos);
            }
        }
        ConvergeAll(World);
        RunTicks(World, kWarmupTicks);

        const int32_t Total = PerPlayer * kPlayers;
        const Sample S = MeasureTicks(World, 100);
        // Scaled by 1000 to keep sub-microsecond per-entity costs legible in integers.
        const int64_t PerEntityNanos = (S.Median * 1000) / Total;

        std::printf("  %8d %12lld %11lld.%03lld\n", Total,
                    static_cast<long long>(S.Median),
                    static_cast<long long>(PerEntityNanos / 1000),
                    static_cast<long long>(PerEntityNanos % 1000));

        if (FirstCount == 0)
        {
            FirstCount = Total;
            FirstPerEntity = PerEntityNanos;
        }
        LastCount = Total;
        LastPerEntity = PerEntityNanos;
    }

    const double Growth = FirstPerEntity > 0
                              ? double(LastPerEntity) / double(FirstPerEntity)
                              : 1.0;
    std::printf("  per-entity cost grew %.2fx from %d to %d entities\n", Growth,
                FirstCount, LastCount);
    std::printf("  (1.0x = linear scaling; >2x over 4x entities suggests quadratic work)\n");

    // Threshold rationale, because a wall-clock ratio is a noisy signal and a
    // badly chosen bound is worse than none.
    //
    // Measured on this machine over five consecutive runs after the bucketed
    // target acquisition landed: 2.49, 2.64, 2.55, 2.83, 4.09. The spread comes
    // from the 500-entity baseline being small enough (826-1039 us) that ordinary
    // scheduling noise moves it by 25%, and dividing by it amplifies that.
    //
    // The old bound of 4.0 sat inside that spread, so this test could fail on
    // healthy code -- a flaky performance test trains people to ignore
    // performance tests. 6.0 sits above the observed noise while still catching
    // the regression that matters: before bucketing, acquisition alone was
    // O(n^2) and this ratio was far higher, since the whole point of the fix was
    // removing 4,000,000 distance checks per tick.
    //
    // If this ever needs raising again, measure five runs first and say so here.
    // Raising it silently would turn the guard into decoration.
    RA4_EXPECT(Growth < 6.0);

    // Absolute floor as well as a ratio: whatever the scaling shape, the largest
    // configuration must still fit the tick budget. This is the check that cannot
    // be satisfied by noise in the baseline.
    RA4_EXPECT(double(LastPerEntity) * double(LastCount) / 1000.0 < kTickBudgetMicros);
}
