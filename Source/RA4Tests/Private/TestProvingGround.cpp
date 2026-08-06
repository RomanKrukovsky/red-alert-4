// Copyright (c) Red Alert 4 project.
// Vertical Proving Ground: 500+ entity stress scenario, deterministic replay & desync testing.

#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Content/ContentDatabase.h"
#include "RA4Core/Checksum.h"
#include "RA4Core/SimConfig.h"
#include "RA4Recon/ReconConfig.h"
#include "RA4Recon/ReconSystem.h"
#include "RA4Simulation/SimWorld.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <vector>

using namespace RA4;
using namespace RA4Test;

namespace RA4
{

RA4_TEST(ProvingGround, HeadlessStressScenario500Entities)
{
    ContentDatabase Db;
    BuildDefaultContent(Db);

    SimWorld World;
    World.Initialize(&Db, MakeTestSetup(1337));

    // Spawn 250 units for Player 0 and 250 for Player 1 (Total 500 entities)
    std::vector<EntityId> HandlesP0;
    std::vector<EntityId> HandlesP1;

    for (int i = 0; i < 250; ++i)
    {
        Vec2 PosP0 = Vec2::FromInts(1000 + (i % 25) * 10, 1000 + (i / 25) * 10);
        EntityId H0 = World.SpawnUnit(Ids::SovConscript, 0, PosP0);
        HandlesP0.push_back(H0);

        Vec2 PosP1 = Vec2::FromInts(5000 - (i % 25) * 10, 5000 - (i / 25) * 10);
        EntityId H1 = World.SpawnUnit(Ids::AllRifleman, 1, PosP1);
        HandlesP1.push_back(H1);
    }

    RA4_EXPECT_EQ(HandlesP0.size() + HandlesP1.size(), 500);

    // Issue move commands for units
    Command MoveCmd0 = MakeCommand(CommandType::Move, 0);
    MoveCmd0.Primary = HandlesP0[0];
    MoveCmd0.Location = Vec2::FromInts(3000, 3000);

    Command MoveCmd1 = MakeCommand(CommandType::Move, 1);
    MoveCmd1.Primary = HandlesP1[0];
    MoveCmd1.Location = Vec2::FromInts(2000, 2000);

    CommandFrame Frame;
    Frame.Tick = 1;
    Frame.Commands.push_back(MoveCmd0);
    Frame.Commands.push_back(MoveCmd1);

    World.Tick(&Frame);

    // Run simulation for 200 ticks (10 seconds)
    RunTicks(World, 200);

    uint64_t HashRun1 = World.ComputeStateChecksum();
    RA4_EXPECT(HashRun1 != 0);

    // Run identical second simulation to verify deterministic hash
    SimWorld World2;
    World2.Initialize(&Db, MakeTestSetup(1337));

    std::vector<EntityId> Handles2P0;
    std::vector<EntityId> Handles2P1;

    for (int i = 0; i < 250; ++i)
    {
        Vec2 PosP0 = Vec2::FromInts(1000 + (i % 25) * 10, 1000 + (i / 25) * 10);
        Handles2P0.push_back(World2.SpawnUnit(Ids::SovConscript, 0, PosP0));

        Vec2 PosP1 = Vec2::FromInts(5000 - (i % 25) * 10, 5000 - (i / 25) * 10);
        Handles2P1.push_back(World2.SpawnUnit(Ids::AllRifleman, 1, PosP1));
    }

    MoveCmd0.Primary = Handles2P0[0];
    MoveCmd1.Primary = Handles2P1[0];

    CommandFrame Frame2;
    Frame2.Tick = 1;
    Frame2.Commands.push_back(MoveCmd0);
    Frame2.Commands.push_back(MoveCmd1);

    World2.Tick(&Frame2);

    RunTicks(World2, 200);

    uint64_t HashRun2 = World2.ComputeStateChecksum();
    RA4_EXPECT_EQ(HashRun1, HashRun2);
}

RA4_TEST(ProvingGround, ForcedDesyncDetection)
{
    ContentDatabase Db;
    BuildDefaultContent(Db);

    SimWorld MainWorld;
    MainWorld.Initialize(&Db, MakeTestSetup(42));

    SimWorld DesyncWorld;
    DesyncWorld.Initialize(&Db, MakeTestSetup(42));

    MainWorld.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(1000, 1000));
    DesyncWorld.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(1000, 1000));

    // Run 50 ticks in sync
    for (int t = 0; t < 50; ++t)
    {
        MainWorld.Tick(nullptr);
        DesyncWorld.Tick(nullptr);
        RA4_EXPECT_EQ(MainWorld.ComputeStateChecksum(), DesyncWorld.ComputeStateChecksum());
    }

    // Force desync mutation on frame 51
    DesyncWorld.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(2000, 2000));

    MainWorld.Tick(nullptr);
    DesyncWorld.Tick(nullptr);

    // Verify desync detection
    RA4_EXPECT(MainWorld.ComputeStateChecksum() != DesyncWorld.ComputeStateChecksum());
}

RA4_TEST(ProvingGround, HeadlessStressScenario1000Entities)
{
    ContentDatabase Db;
    BuildDefaultContent(Db);

    SimWorld World;
    World.Initialize(&Db, MakeTestSetup(4096));

    for (int i = 0; i < 500; ++i)
    {
        Vec2 PosP0 = Vec2::FromInts(1000 + (i % 50) * 8, 1000 + (i / 50) * 8);
        World.SpawnUnit(Ids::SovConscript, 0, PosP0);

        Vec2 PosP1 = Vec2::FromInts(5000 - (i % 50) * 8, 5000 - (i / 50) * 8);
        World.SpawnUnit(Ids::AllRifleman, 1, PosP1);
    }

    RA4_EXPECT_EQ(World.GetEntityCapacity(), 1000);

    // Run simulation for 100 ticks
    RunTicks(World, 100);

    uint64_t StateHash = World.ComputeStateChecksum();
    RA4_EXPECT(StateHash != 0);
}

RA4_TEST(ProvingGround, HeadlessStressScenario2000Entities)
{
    ContentDatabase Db;
    BuildDefaultContent(Db);

    SimWorld World;
    World.Initialize(&Db, MakeTestSetup(8192));

    for (int i = 0; i < 1000; ++i)
    {
        Vec2 PosP0 = Vec2::FromInts(1000 + (i % 50) * 8, 1000 + (i / 50) * 8);
        World.SpawnUnit(Ids::SovConscript, 0, PosP0);

        Vec2 PosP1 = Vec2::FromInts(5000 - (i % 50) * 8, 5000 - (i / 50) * 8);
        World.SpawnUnit(Ids::AllRifleman, 1, PosP1);
    }

    RA4_EXPECT_EQ(World.GetEntityCapacity(), 2000);

    // Run simulation for 100 ticks
    RunTicks(World, 100);

    uint64_t StateHash = World.ComputeStateChecksum();
    RA4_EXPECT(StateHash != 0);
}

// --- P-7: measure the provisional perception-warfare budgets ----------------------
//
// PERFORMANCE_BUDGETS.md section 4 marked its numbers "(p)" -- engineering
// estimates awaiting measurement at the agreed baseline (2,000 entities /
// 4 players, section 4.4). This benchmark produces those measurements and
// GATES on the hard maxima, so a regression that blows a budget fails CI
// rather than surfacing as late-game stutter. Targets stay advisory here;
// hard maxima are the assertions (budget doc: "fails CI above the hard max").
//
// Wall-clock caveat: microsecond timings on shared CI hardware are noisy.
// The gate therefore compares the MEDIAN of per-tick maxima across players
// against the hard max with a 2x tolerance factor recorded in the output;
// PERFORMANCE_BUDGETS.md records the measured medians, not the tolerated
// bound. Determinism is unaffected -- timing never feeds back into the sim.

namespace
{

Recon::ReconSettings MakeBenchmarkReconSettings()
{
    Recon::ReconSettings S;
    S.bEnabled = true;
    Recon::DistortionProfile P;
    P.Name = "profile.default";
    S.DistortionProfiles.push_back(P);
    Recon::CommsProfile C;
    C.Name = "comms.default";
    C.HopDelayTicksByLevel = {160, 80, 30, 5};
    S.CommsProfiles.push_back(C);
    return S;
}

MatchSetup MakeFourPlayerSetup(uint64_t Seed)
{
    MatchSetup Setup = MakeTestSetup(Seed);
    Setup.Players[2].bActive = true;
    Setup.Players[2].Faction = FactionId::Soviet;
    Setup.Players[2].StartingCredits = 10000;
    Setup.Players[3].bActive = true;
    Setup.Players[3].Faction = FactionId::Alliance;
    Setup.Players[3].StartingCredits = 10000;
    return Setup;
}

} // namespace

// Measures the recon layer at the gating baseline from PERFORMANCE_BUDGETS 4.4.
//
// This test used to pass while measuring nothing: it reported "median 0 us, track
// cap 0" because the four armies were spawned 5,200 units apart and expected to
// converge, the AttackMove order never executed (the first unit was still at its
// spawn point after 1,200 ticks), and so no contact was ever observed. A budget
// gate satisfied by an idle system is worse than no gate. Two changes fixed it:
// the armies now start within vision of each other, so the benchmark depends on
// recon rather than on movement; and the assertion below refuses to trust any
// timing unless belief actually formed.
//
// The order-execution defect this exposed is real but separate, and is recorded in
// NEXT_ACTIONS as I-M5-scenario rather than left implied by a red test here.
RA4_TEST(ProvingGround, ReconBudgetsAt2000Entities4Players)
{
    ContentDatabase Db;
    BuildDefaultContent(Db);
    Recon::ReconSettings Settings = MakeBenchmarkReconSettings();

    SimWorld World;
    World.Initialize(&Db, MakeFourPlayerSetup(20260806), &Settings);

    // 500 units per player in four separated blocks that converge on the map
    // centre, so fog boundaries are crossed and the observation phase has real
    // contacts to chew on rather than four static parade grounds.
    const ContentId UnitOf[4] = {Ids::SovConscript, Ids::AllRifleman, Ids::SovConscript,
                                 Ids::AllRifleman};
    // Armies start ALREADY in contact rather than 5,200 units apart. Vision range
    // is 10 m = 1,000 sim units, so the old separated blocks depended on the units
    // converging -- and they never did (order execution defect, see the note above
    // this test), leaving the pipeline idle and the budgets trivially met. A
    // performance benchmark for RECON must not be gated on MOVEMENT working: it
    // measures observation, report and track cost, all of which need contact, not
    // travel. Four 25x20 blocks around the map centre, ~700 units apart, so every
    // block sees two neighbours and the observation phase has real work every tick.
    const int32_t BaseX[4] = {2900, 3600, 2900, 3600};
    const int32_t BaseY[4] = {2900, 2900, 3600, 3600};
    // Each side needs a construction yard. Reports enter the chain of command only
    // at a receiving node, and nodes come from BUILDINGS -- so an army with no HQ
    // produces orphan reports that never land, and the perceived world stays empty.
    // Proved by ProvingGround.BenchmarkScenarioActuallyFormsBelief: with an HQ the
    // first track appears after 30 ticks, without one it never appears at all. This
    // is why the budget numbers below used to read "median 0 us, track cap 0" and
    // still passed -- they were timing an idle pipeline.
    const int32_t HqTileX[4] = {2, 28, 2, 28};
    const int32_t HqTileY[4] = {2, 2, 28, 28};
    for (PlayerId P = 0; P < 4; ++P)
    {
        const EntityId Hq = World.SpawnBuilding(Ids::SovConYard, P,
                                               TileCoord(HqTileX[P], HqTileY[P]),
                                               /*bInstantComplete*/ true);
        RA4_EXPECT(Hq.IsValid());
    }

    for (PlayerId P = 0; P < 4; ++P)
    {
        for (int i = 0; i < 500; ++i)
        {
            const Vec2 Pos = Vec2::FromInts(BaseX[P] + (i % 25) * 8, BaseY[P] + (i / 25) * 8);
            World.SpawnUnit(UnitOf[P], P, Pos);
        }
    }
    // 2,000 units plus 4 headquarters. The gating baseline is the 2,000 UNITS from
    // PERFORMANCE_BUDGETS 4.4; the HQs are scenario scaffolding, not extra load.
    RA4_EXPECT_EQ(int32_t(World.GetEntityCapacity()), 2004);

    // No movement order: the units are already within vision of each other, and
    // issuing one would make this benchmark depend on pathfinding as well as recon.

    // Warm-up (fog carving, first contacts), unmeasured.
    // 250 ticks, not 50. The benchmark comms profile delays a report by up to 160
    // ticks per hop, so a 50-tick warm-up ends before the first report has landed:
    // measurement then starts on an empty perceived world and the budgets are
    // trivially met. Warm-up must outlast the longest delivery path, or the gate is
    // timing the wrong thing.
    // Warm-up must outlast the benchmark comms profile's 160-tick per-hop delay,
    // or measurement begins before the first report has landed and the budgets are
    // met by an idle pipeline.
    RunTicks(World, 250);

    // Measured window: 200 ticks (10 s of sim time) with per-tick sampling.
    constexpr int32_t kMeasuredTicks = 200;
    std::vector<int64_t> ReconMicrosPerTick;  // ingestion + tracks: all phases
    std::vector<int64_t> DecayMicrosPerTick;  // TrackUpdate alone
    std::vector<int64_t> ChecksumDeltaMicros; // recon share of checksum ticks

    const Recon::PhaseStats& Stats = World.GetRecon().GetStats();
    for (int32_t T = 0; T < kMeasuredTicks; ++T)
    {
        World.Tick(nullptr);
        World.ClearEvents();

        int64_t TickTotal = 0;
        for (int32_t Ph = 0; Ph < Recon::kPhaseCount; ++Ph)
        {
            TickTotal += Stats.LastTickMicroseconds[Ph];
        }
        ReconMicrosPerTick.push_back(TickTotal);
        DecayMicrosPerTick.push_back(
            Stats.LastTickMicroseconds[int32_t(Recon::Phase::TrackUpdate)]);

        if ((World.GetTick() % kChecksumIntervalTicks) == 0)
        {
            // Recon share of the checksum: hash the recon layer alone.
            const auto C0 = std::chrono::steady_clock::now();
            Hash64 H;
            World.GetRecon().FeedChecksum(H);
            const auto C1 = std::chrono::steady_clock::now();
            (void)H;
            ChecksumDeltaMicros.push_back(
                std::chrono::duration_cast<std::chrono::microseconds>(C1 - C0).count());
        }
    }

    const auto Median = [](std::vector<int64_t> V) -> int64_t
    {
        std::sort(V.begin(), V.end());
        return V.empty() ? 0 : V[V.size() / 2];
    };
    const auto Peak = [](const std::vector<int64_t>& V) -> int64_t
    {
        int64_t M = 0;
        for (int64_t X : V)
        {
            M = X > M ? X : M;
        }
        return M;
    };

    const int64_t ReconMedian = Median(ReconMicrosPerTick);
    const int64_t ReconPeak = Peak(ReconMicrosPerTick);
    const int64_t DecayMedian = Median(DecayMicrosPerTick);
    const int64_t ChecksumMedian = Median(ChecksumDeltaMicros);

    // The numbers PERFORMANCE_BUDGETS.md section 4.1 must record (P-7):
    std::printf("[P-7] recon per-tick: median %lld us, peak %lld us (hard max 1500 us)\n",
                static_cast<long long>(ReconMedian), static_cast<long long>(ReconPeak));
    std::printf("[P-7] decay (TrackUpdate) per-tick: median %lld us (hard max 500 us)\n",
                static_cast<long long>(DecayMedian));
    std::printf("[P-7] recon checksum share: median %lld us per checksum tick (hard max 600 us)\n",
                static_cast<long long>(ChecksumMedian));

    // Gates: hard maxima from section 4.1, 2x noise tolerance on shared hardware.
    RA4_EXPECT(ReconMedian <= 2 * 1500);
    RA4_EXPECT(DecayMedian <= 2 * 500);
    RA4_EXPECT(ChecksumMedian <= 2 * 600);

    // 4.4 combined ceiling (2.5 ms target / 4.0 ms hard max) currently has only
    // the recon contributor implemented; gate what exists.
    RA4_EXPECT(ReconMedian <= 2 * 4000);

    // A performance gate that measures an idle system reports PASS forever and hides
    // the regression it exists to catch. Assert the workload was real BEFORE
    // trusting any timing above.
    {
        uint32_t TotalAliveTracks = 0;
        for (PlayerId P = 0; P < 4; ++P)
        {
            TotalAliveTracks += World.GetRecon().GetPerceivedWorld(P).GetAliveTrackCount();
        }
        std::printf("[P-7] alive tracks across 4 players: %u\n", TotalAliveTracks);
        // The exact count is scenario-dependent and deliberately not pinned; zero is
        // the only value that proves the measurement is meaningless.
        RA4_EXPECT(TotalAliveTracks > 0);
    }

    // Memory: PerceivedWorld per player (budget: <= 4 MB target, 8 MB hard).
    // Track slots dominate; measure the real allocation via capacity.
    const uint32_t TrackCap = World.GetRecon().GetPerceivedWorld(0).GetTrackCapacity();
    const size_t PerPlayerBytes =
        size_t(TrackCap) * sizeof(Recon::PerceivedTrack) + size_t(64 * 64) * sizeof(TickIndex);
    // GetTrackCapacity() is Tracks.size(), i.e. slots ever allocated -- it grows with
    // use and reads 0 on an idle world, so it measures live usage, not the ceiling.
    // The 8 MB budget is about the worst case, so it is checked at the configured
    // cap: otherwise the budget could be satisfied by simply not working.
    const size_t WorstCaseBytes = size_t(Settings.Tracks.MaxTracksPerPlayer) *
                                      sizeof(Recon::PerceivedTrack) +
                                  size_t(64 * 64) * sizeof(TickIndex);
    std::printf("[P-7] PerceivedWorld per player: ~%zu KB live (%u slots), ~%zu KB at cap (%d slots)\n",
                PerPlayerBytes / 1024, TrackCap, WorstCaseBytes / 1024,
                Settings.Tracks.MaxTracksPerPlayer);
    RA4_EXPECT(WorstCaseBytes <= size_t(8) * 1024 * 1024);

    // Determinism sanity: the measured world still hashes.
    RA4_EXPECT(World.ComputeStateChecksum() != 0);
}

RA4_TEST(ProvingGround, ReconBudgetStressInformational5000)
{
    // Section 4.4: 5,000 entities is a STRESS METRIC, never a gate (product
    // owner decision 2026-08-05). Prints numbers, asserts nothing but survival.
    ContentDatabase Db;
    BuildDefaultContent(Db);
    Recon::ReconSettings Settings = MakeBenchmarkReconSettings();

    SimWorld World;
    World.Initialize(&Db, MakeFourPlayerSetup(50000806), &Settings);
    for (PlayerId P = 0; P < 4; ++P)
    {
        for (int i = 0; i < 1250; ++i)
        {
            const Vec2 Pos =
                Vec2::FromInts(600 + int32_t(P) * 1400 + (i % 35) * 6, 600 + (i / 35) * 6);
            World.SpawnUnit(P % 2 == 0 ? Ids::SovConscript : Ids::AllRifleman, P, Pos);
        }
    }
    RunTicks(World, 100);

    const Recon::PhaseStats& Stats = World.GetRecon().GetStats();
    int64_t Total = 0;
    for (int32_t Ph = 0; Ph < Recon::kPhaseCount; ++Ph)
    {
        Total += Stats.TotalMicroseconds[Ph];
    }
    const int64_t AvgPerTick = Stats.TicksMeasured > 0 ? Total / Stats.TicksMeasured : 0;
    std::printf("[P-7 stress 5000] recon avg %lld us/tick over %u ticks (informational only)\n",
                static_cast<long long>(AvgPerTick), Stats.TicksMeasured);
    RA4_EXPECT(World.ComputeStateChecksum() != 0);
}

} // namespace RA4

RA4_TEST(ProvingGround, BenchmarkScenarioActuallyFormsBelief)
{
    // Diagnostic probe, kept as a permanent test because it answers the question
    // the budget benchmark cannot: does the benchmark's OWN scenario produce any
    // belief at all? ReconBudgetsAt2000Entities4Players reported "median 0 us,
    // track cap 0" and passed -- it was timing an idle pipeline. A performance
    // gate measuring nothing is worse than no gate, so the scenario is pinned
    // separately from the timings.
    ContentDatabase Db;
    BuildDefaultContent(Db);
    Recon::ReconSettings Settings = MakeBenchmarkReconSettings();

    SimWorld World;
    World.Initialize(&Db, MakeFourPlayerSetup(20260806), &Settings);

    // Two hostile units well inside each other's vision, the same shape as the
    // working recon tests. If belief does not form HERE, the benchmark's 2,000
    // converging units were never the problem.
    // A construction yard for the observing side: reports enter the chain only at a
    // receiving node, and nodes come from buildings. Without one every report is an
    // orphan. This is what the benchmark scenario was missing -- it spawned units
    // only.
    World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(14, 14), /*bInstantComplete*/ true);
    World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(3000, 3000));
    World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(3400, 3000));

    // 600 ticks, not 400: with no receiving node the report is an ORPHAN, delayed
    // by Chain.OrphanDelayTicks (200 by default) and then by the benchmark comms
    // profile's per-hop delays {160, 80, 30, 5}. 400 ticks can expire before the
    // first report ever lands, which is a slow pipeline, not a broken one --
    // exactly the distinction this probe exists to make.
    uint32_t Alive = 0;
    int32_t TicksToFirstTrack = -1;
    for (int32_t T = 0; T < 600; ++T)
    {
        World.Tick(nullptr);
        World.ClearEvents();
        Alive = World.GetRecon().GetPerceivedWorld(0).GetAliveTrackCount();
        if (Alive > 0)
        {
            TicksToFirstTrack = T;
            break;
        }
    }

    std::printf("[DIAG] benchmark settings: first track after %d ticks (alive %u)\n",
                TicksToFirstTrack, Alive);

    // The benchmark profile leaves every distortion stage enabled, unlike
    // MakeMinimalSettings which disables them. Belief must still form: distortion
    // is meant to corrupt reports, never to silence the pipeline entirely.
    RA4_EXPECT(Alive > 0);
}
