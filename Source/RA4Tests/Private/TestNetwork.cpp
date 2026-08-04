// Copyright (c) Red Alert 4 project. Lockstep networking tests.
//
// The integration tests at the bottom are the point of this file: two independent
// SimWorlds, driven only through LockstepSession, must stay bit-identical for the
// whole match, and must be *caught* when they do not. Everything above them tests
// one rule of the protocol in isolation so a failure names the rule it broke.
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Simulation/LockstepSession.h"

namespace RA4
{

using namespace RA4Test;
using RA4::Net::LockstepSession;
using RA4::Net::StallReason;

namespace
{

// A Move order carrying a caller-chosen tag in Param, so a test can assert on the
// exact ordering of commands after they have been through frame assembly.
Command MakeTaggedMove(PlayerId Issuer, int32_t Tag)
{
    Command C = MakeCommand(CommandType::Move, Issuer);
    C.Param = Tag;
    return C;
}

} // namespace

// --- Input delay -----------------------------------------------------------

RA4_TEST(Lockstep, CommandIsDeferredByInputDelay)
{
    LockstepSession Session;
    Session.Initialize(/*LocalPlayer*/ 0, /*NumPlayers*/ 2, /*bIsAuthority*/ false, /*InputDelay*/ 3);

    const TickIndex Scheduled = Session.SubmitLocalCommand(10, MakeTaggedMove(0, 7));
    RA4_EXPECT_EQ(Scheduled, TickIndex(13));

    // The tick the player was looking at when they clicked must not carry the order.
    RA4_EXPECT_EQ(Session.TakeOutgoingFrame(10).Commands.size(), size_t(0));

    const CommandFrame Frame = Session.TakeOutgoingFrame(13);
    RA4_EXPECT_EQ(Frame.Tick, TickIndex(13));
    RA4_REQUIRE(Frame.Commands.size() == 1);
    RA4_EXPECT_EQ(Frame.Commands[0].Param, 7);

    // Taken once and gone; a resend must not duplicate it into the stream.
    RA4_EXPECT_EQ(Session.TakeOutgoingFrame(13).Commands.size(), size_t(0));
}

RA4_TEST(Lockstep, ZeroInputDelayIsClampedToOne)
{
    // Delay 0 would require the packet to arrive before it was sent. Clamped rather
    // than rejected so a bad config is laggy, not silently order-dropping.
    LockstepSession Session;
    Session.Initialize(0, 2, false, /*InputDelay*/ 0);

    RA4_EXPECT_EQ(Session.GetInputDelay(), TickIndex(1));
    RA4_EXPECT_EQ(Session.SubmitLocalCommand(40, MakeTaggedMove(0, 1)), TickIndex(41));
}

// --- Deterministic assembly ------------------------------------------------

RA4_TEST(Lockstep, FrameAssemblyFollowsSlotOrderNotArrivalOrder)
{
    // The whole protocol rests on this. If assembly followed arrival order, two
    // servers handed the same packets over different routes would build different
    // command streams, and the peers would desync before the simulation ran a step.
    CommandFrame FromP0;
    FromP0.Tick = 5;
    FromP0.Commands.push_back(MakeTaggedMove(0, 100));
    FromP0.Commands.push_back(MakeTaggedMove(0, 101));

    CommandFrame FromP1;
    FromP1.Tick = 5;
    FromP1.Commands.push_back(MakeTaggedMove(1, 200));

    LockstepSession ServerA;
    ServerA.Initialize(0, 2, true);
    ServerA.ReceivePlayerFrame(0, FromP0);
    ServerA.ReceivePlayerFrame(1, FromP1);

    LockstepSession ServerB;
    ServerB.Initialize(0, 2, true);
    ServerB.ReceivePlayerFrame(1, FromP1);   // reversed on the wire
    ServerB.ReceivePlayerFrame(0, FromP0);

    CommandFrame OutA;
    CommandFrame OutB;
    RA4_REQUIRE(ServerA.AssembleAuthoritativeFrame(5, OutA));
    RA4_REQUIRE(ServerB.AssembleAuthoritativeFrame(5, OutB));

    RA4_REQUIRE(OutA.Commands.size() == 3);
    RA4_REQUIRE(OutB.Commands.size() == 3);
    for (size_t I = 0; I < OutA.Commands.size(); ++I)
    {
        RA4_EXPECT_EQ(OutA.Commands[I].Param, OutB.Commands[I].Param);
    }
    RA4_EXPECT_EQ(OutA.Commands[0].Param, 100);
    RA4_EXPECT_EQ(OutA.Commands[1].Param, 101);
    RA4_EXPECT_EQ(OutA.Commands[2].Param, 200);
}

RA4_TEST(Lockstep, FrameIsIncompleteUntilEveryPlayerReports)
{
    LockstepSession Server;
    Server.Initialize(0, 3, true);

    CommandFrame Empty;
    Empty.Tick = 12;

    RA4_EXPECT(!Server.IsFrameComplete(12));
    Server.ReceivePlayerFrame(0, Empty);
    RA4_EXPECT(!Server.IsFrameComplete(12));
    Server.ReceivePlayerFrame(2, Empty);
    RA4_EXPECT(!Server.IsFrameComplete(12));

    // A player with nothing to say still completes the tick — that is exactly why
    // an empty frame has to be transmitted rather than skipped.
    Server.ReceivePlayerFrame(1, Empty);
    RA4_EXPECT(Server.IsFrameComplete(12));

    CommandFrame Out;
    RA4_EXPECT(Server.AssembleAuthoritativeFrame(12, Out));
    RA4_EXPECT_EQ(Out.Commands.size(), size_t(0));
    RA4_EXPECT_EQ(Out.Tick, TickIndex(12));
}

RA4_TEST(Lockstep, DuplicatePlayerFrameIsIgnored)
{
    // A retransmit must not apply the same production order and credit spend twice
    // on the server while the clients saw it once.
    LockstepSession Server;
    Server.Initialize(0, 2, true);

    CommandFrame FromP0;
    FromP0.Tick = 3;
    FromP0.Commands.push_back(MakeTaggedMove(0, 42));

    CommandFrame FromP1;
    FromP1.Tick = 3;

    Server.ReceivePlayerFrame(0, FromP0);
    Server.ReceivePlayerFrame(0, FromP0);   // duplicate
    Server.ReceivePlayerFrame(1, FromP1);

    CommandFrame Out;
    RA4_REQUIRE(Server.AssembleAuthoritativeFrame(3, Out));
    RA4_EXPECT_EQ(Out.Commands.size(), size_t(1));
}

RA4_TEST(Lockstep, AssemblyIsRepeatableForRetransmission)
{
    LockstepSession Server;
    Server.Initialize(0, 1, true);

    CommandFrame FromP0;
    FromP0.Tick = 8;
    FromP0.Commands.push_back(MakeTaggedMove(0, 9));
    Server.ReceivePlayerFrame(0, FromP0);

    CommandFrame First;
    CommandFrame Second;
    RA4_REQUIRE(Server.AssembleAuthoritativeFrame(8, First));
    RA4_REQUIRE(Server.AssembleAuthoritativeFrame(8, Second));
    RA4_EXPECT_EQ(First.Commands.size(), Second.Commands.size());
    RA4_EXPECT_EQ(Second.Commands.size(), size_t(1));
}

// --- Stalling --------------------------------------------------------------

RA4_TEST(Lockstep, PeerStallsUntilAuthoritativeFrameArrives)
{
    LockstepSession Client;

    // Before Initialize there is no session to stall on; the caller must be able to
    // tell that apart from a slow server.
    RA4_EXPECT(int(Client.GetStallReason(0)) == int(StallReason::NotStarted));

    Client.Initialize(1, 2, false);
    RA4_EXPECT(!Client.CanAdvance(4));
    RA4_EXPECT(int(Client.GetStallReason(4)) == int(StallReason::AwaitingFrame));

    CommandFrame Out;
    RA4_EXPECT(!Client.TakeAuthoritativeFrame(4, Out));

    CommandFrame Authoritative;
    Authoritative.Tick = 4;
    Authoritative.Commands.push_back(MakeTaggedMove(0, 55));
    Client.ReceiveAuthoritativeFrame(Authoritative);

    RA4_EXPECT(Client.CanAdvance(4));
    RA4_REQUIRE(Client.TakeAuthoritativeFrame(4, Out));
    RA4_EXPECT_EQ(Out.Commands.size(), size_t(1));

    // Consumed exactly once, so a tick can never be executed twice.
    RA4_EXPECT(!Client.CanAdvance(4));
}

RA4_TEST(Lockstep, PruneReleasesRetiredTicks)
{
    LockstepSession Session;
    Session.Initialize(0, 1, true);

    for (TickIndex T = 0; T < 10u; ++T)
    {
        Session.SubmitLocalCommand(T, MakeTaggedMove(0, int32_t(T)));

        CommandFrame Frame;
        Frame.Tick = T;
        Session.ReceiveAuthoritativeFrame(Frame);
    }

    RA4_EXPECT_EQ(Session.GetPendingOutgoingCount(), size_t(10));
    RA4_EXPECT_EQ(Session.GetBufferedAuthoritativeCount(), size_t(10));

    Session.PruneUpToTick(5);
    RA4_EXPECT_EQ(Session.GetBufferedAuthoritativeCount(), size_t(4));   // ticks 6..9
    // Input delay 2 scheduled these onto ticks 2..11, so the prune drops 2..5.
    RA4_EXPECT_EQ(Session.GetPendingOutgoingCount(), size_t(6));         // ticks 6..11
}

// --- Checksum adjudication -------------------------------------------------

RA4_TEST(Lockstep, MatchingChecksumsDoNotReportDesync)
{
    LockstepSession Server;
    Server.Initialize(0, 2, true);

    for (TickIndex T = 0; T < 20u; ++T)
    {
        Server.SubmitChecksum(0, T, 0xABCDEF00ull + T);
        Server.SubmitChecksum(1, T, 0xABCDEF00ull + T);
    }
    RA4_EXPECT(!Server.HasDesynced());
}

RA4_TEST(Lockstep, DivergentChecksumIsReportedWithTickAndPlayer)
{
    LockstepSession Server;
    Server.Initialize(0, 2, true);

    Server.SubmitChecksum(0, 7, 0x1111ull);
    Server.SubmitChecksum(1, 7, 0x2222ull);

    RA4_REQUIRE(Server.HasDesynced());
    const Net::DesyncReport& Report = Server.GetDesync();
    RA4_EXPECT_EQ(Report.Tick, TickIndex(7));
    RA4_EXPECT_EQ(Report.Player, PlayerId(1));
    RA4_EXPECT_EQ(Report.Expected, uint64_t(0x1111ull));
    RA4_EXPECT_EQ(Report.Actual, uint64_t(0x2222ull));

    // The first divergence is the only informative one; after it every tick differs.
    Server.SubmitChecksum(1, 9, 0x3333ull);
    RA4_EXPECT_EQ(Server.GetDesync().Tick, TickIndex(7));
}

RA4_TEST(Lockstep, ClientDoesNotAdjudicateChecksums)
{
    LockstepSession Client;
    Client.Initialize(1, 2, /*bIsAuthority*/ false);

    Client.SubmitChecksum(0, 3, 0xAAAAull);
    Client.SubmitChecksum(1, 3, 0xBBBBull);

    // Only the authority declares a desync, or two clients would disagree about
    // which of them was wrong.
    RA4_EXPECT(!Client.HasDesynced());
}

// --- Integration: two peers running real simulations -----------------------

namespace
{

// Everything one peer needs: its own world, its own session, its own view of the
// match. Nothing is shared between two of these except the frames explicitly handed
// across in the harness loop below, which is what makes the test meaningful.
struct Peer
{
    SimWorld World;
    LockstepSession Session;
};

// Plays NumTicks of a two-player match with both peers driven exclusively through
// the lockstep protocol. If bCorruptPeer1AtTick is set, peer 1's world is mutated
// behind the protocol's back at that tick to prove the desync check actually fires.
// Returns the number of commands that reached the simulations.
size_t RunLockstepMatch(Peer& P0, Peer& P1, TickIndex NumTicks, TickIndex CorruptTick, bool bCorrupt)
{
    const TickIndex Delay = P0.Session.GetInputDelay();
    Peer* Peers[2] = { &P0, &P1 };

    // Both peers start with one unit each, spawned identically outside the command
    // stream, so there is something for the Move orders to act on.
    const EntityId Unit0 = P0.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(1000, 1000));
    P1.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(1000, 1000));
    const EntityId Unit1 = P0.World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(5000, 5000));
    P1.World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(5000, 5000));

    // Prime the pipeline: ticks 0..Delay-1 execute before anyone has had a chance to
    // schedule input for them, so every peer must still report an empty frame or the
    // server waits forever for a packet that was never going to be sent.
    for (TickIndex T = 0; T < Delay; ++T)
    {
        for (uint8_t P = 0; P < 2; ++P)
        {
            P0.Session.ReceivePlayerFrame(P, Peers[P]->Session.TakeOutgoingFrame(T));
        }
    }

    size_t DeliveredCommands = 0;

    for (TickIndex T = 0; T < NumTicks; ++T)
    {
        // 1. Local input. Each peer only ever issues orders for the player it owns.
        if (T % 5 == 0)
        {
            Command C = MakeCommand(CommandType::Move, 0);
            C.Primary = Unit0;
            C.Location = Vec2::FromInts(2000 + int32_t(T) * 10, 1500);
            P0.Session.SubmitLocalCommand(T, C);
        }
        if (T % 7 == 0)
        {
            Command C = MakeCommand(CommandType::Move, 1);
            C.Primary = Unit1;
            C.Location = Vec2::FromInts(4000, 3000 + int32_t(T) * 10);
            P1.Session.SubmitLocalCommand(T, C);
        }

        // 2. Every peer sends its slice for T+Delay, empty or not.
        for (uint8_t P = 0; P < 2; ++P)
        {
            P0.Session.ReceivePlayerFrame(P, Peers[P]->Session.TakeOutgoingFrame(T + Delay));
        }

        // 3. The authority assembles and broadcasts.
        CommandFrame Authoritative;
        if (!P0.Session.AssembleAuthoritativeFrame(T, Authoritative))
        {
            RA4Test::ReportFailure("server could not assemble frame for tick " + std::to_string(T),
                                   __FILE__, __LINE__);
            return DeliveredCommands;
        }
        DeliveredCommands += Authoritative.Commands.size();
        P0.Session.ReceiveAuthoritativeFrame(Authoritative);
        P1.Session.ReceiveAuthoritativeFrame(Authoritative);

        // 4. Each peer runs the tick from the frame it was given, never from its own
        //    local input. This is the step that makes the two worlds converge.
        for (uint8_t P = 0; P < 2; ++P)
        {
            CommandFrame ToRun;
            if (!Peers[P]->Session.TakeAuthoritativeFrame(T, ToRun))
            {
                RA4Test::ReportFailure("peer stalled at tick " + std::to_string(T), __FILE__, __LINE__);
                return DeliveredCommands;
            }
            Peers[P]->World.Tick(&ToRun);
        }

        if (bCorrupt && T == CorruptTick)
        {
            P1.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(3000, 3000));
        }

        // 5. Both peers report their post-tick state to the authority.
        P0.Session.SubmitChecksum(0, T, P0.World.ComputeStateChecksum());
        P0.Session.SubmitChecksum(1, T, P1.World.ComputeStateChecksum());

        P0.Session.PruneUpToTick(T);
    }

    return DeliveredCommands;
}

} // namespace

RA4_TEST(Lockstep, TwoPeersStayInSyncAcrossAFullMatch)
{
    ContentDatabase Db;
    BuildDefaultContent(Db);

    Peer P0;
    Peer P1;
    P0.World.Initialize(&Db, MakeTestSetup(2024));
    P1.World.Initialize(&Db, MakeTestSetup(2024));
    P0.Session.Initialize(0, 2, /*bIsAuthority*/ true);
    P1.Session.Initialize(1, 2, /*bIsAuthority*/ false);

    const size_t Delivered = RunLockstepMatch(P0, P1, 120, 0, /*bCorrupt*/ false);

    // Guards against a green test that proved nothing because no order ever moved.
    RA4_EXPECT(Delivered > 0);
    RA4_EXPECT(!P0.Session.HasDesynced());
    RA4_EXPECT_EQ(P0.World.ComputeStateChecksum(), P1.World.ComputeStateChecksum());
}

RA4_TEST(Lockstep, DesyncIsCaughtOnTheTickItHappens)
{
    ContentDatabase Db;
    BuildDefaultContent(Db);

    Peer P0;
    Peer P1;
    P0.World.Initialize(&Db, MakeTestSetup(2024));
    P1.World.Initialize(&Db, MakeTestSetup(2024));
    P0.Session.Initialize(0, 2, true);
    P1.Session.Initialize(1, 2, false);

    RunLockstepMatch(P0, P1, 120, /*CorruptTick*/ 40, /*bCorrupt*/ true);

    RA4_REQUIRE(P0.Session.HasDesynced());

    // Detected on the very tick the worlds diverged, not merely by the end of the
    // match: the value of the checksum stream is that it names the culprit tick.
    RA4_EXPECT_EQ(P0.Session.GetDesync().Tick, TickIndex(40));
    RA4_EXPECT_EQ(P0.Session.GetDesync().Player, PlayerId(1));
    RA4_EXPECT(P0.World.ComputeStateChecksum() != P1.World.ComputeStateChecksum());
}

} // namespace RA4
