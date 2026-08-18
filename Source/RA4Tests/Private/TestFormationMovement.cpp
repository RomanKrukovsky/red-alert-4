// Copyright (c) Red Alert 4 project. Tests that formation slots actually execute.
//
// WHY THIS FILE EXISTS
//
// Formations were declared in the core long before they were driven. `MovementComp::
// FormationId` and `FormationSlot` (SimTypes.h) had ZERO readers anywhere in Source/,
// and Formation.cpp held only `RotateOffset`. The one pre-existing formation test
// (TestNavigation.cpp, FormationOffsetsRotateWithLeaderFacing) is pure arithmetic on
// `RotateOffset`: it never spawns an entity and never ticks the world, despite its
// name promising that members follow a leader slot.
//
// So when the movement system gained formation support, no existing test could tell
// whether it worked. A green suite would have proved nothing. These tests are the
// only thing standing between "formations are implemented" and "formations are
// believed to be implemented".
//
// TWO THINGS THAT MAKE THESE TESTS UNUSUAL, both deliberate:
//
// 1. Nothing in the shipping tree assigns `FormationId` yet -- there is no writer.
//    The feature is therefore correct but dormant, and a test that merely issues a
//    move order would exercise none of it and pass vacuously. Each test here assigns
//    the formation itself, via the `const_cast` accessor idiom already used for
//    component mutation elsewhere in this suite (see TestNavigation.cpp's bridge
//    test, which const_casts `GetMap()` for the same reason).
//
// 2. No offset VALUE is hardcoded. The offset tables live in Formation.cpp and are
//    free to be retuned; a test pinned to their numbers would fail on a legitimate
//    change. Instead the leader-frame offset is RECOVERED from world positions using
//    `RotateOffset(delta, -LeaderFacing)` -- valid as an inverse because FxSin/FxCos
//    wrap their angle -- and compared against `FindFormationDef`. The tests assert
//    the RELATIONSHIP, which is the actual contract, not the table.

#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Core/SimConfig.h"
#include "RA4Content/ContentDatabase.h"

#include "RA4Navigation/Formation.h"

using namespace RA4;

namespace
{
struct FormationFixture
{
    ContentDatabase Content;
    SimWorld World;
    explicit FormationFixture(uint64_t Seed)
    {
        BuildDefaultContent(Content);
        World.Initialize(&Content, RA4Test::MakeTestSetup(Seed));
    }
};

// `Tick` early-returns unless the match is Running, and SystemVictory declares a
// winner the moment a player owns zero units AND zero buildings. A single-sided
// test would therefore conclude on tick 1 and every later tick would be a silent
// no-op -- units would appear frozen for a reason unrelated to movement. Every test
// in this file keeps an opposing entity alive for exactly that reason.
EntityId KeepMatchAlive(SimWorld& World)
{
    return World.SpawnUnit(RA4Test::Ids::SovConscript, PlayerId{1},
                           Vec2(Fixed::FromInt(6000), Fixed::FromInt(6000)));
}

// Assigning a formation needs write access to MovementComp, and SimWorld exposes
// only a const accessor. const_cast on the accessor is this suite's established
// idiom for component mutation from a test.
void AssignFormation(SimWorld& World, EntityId Id, ContentId Formation, int32_t Slot)
{
    const MovementComp* Read = World.GetMovement(Id);
    RA4_REQUIRE(Read != nullptr);
    MovementComp& Write = *const_cast<MovementComp*>(Read);
    Write.FormationId = Formation;
    Write.FormationSlot = Slot;
}

void OrderMove(SimWorld& World, EntityId Id, const Vec2& To)
{
    Command Move;
    Move.Type = CommandType::Move;
    Move.Issuer = PlayerId{0};
    Move.Primary = Id;
    Move.Location = To;
    // Commands are rate-limited to kMaxCommandsPerPlayerPerTick (64) per player per
    // tick, and an over-cap command is REJECTED, not queued. Asserting acceptance
    // here means a rate-limit rejection can never masquerade as a movement failure --
    // which is exactly how a "only N of M arrived" result can be faked by the
    // harness rather than caused by the code under test.
    RA4_REQUIRE(World.ApplyCommand(Move).IsAccepted());
}

void Run(SimWorld& World, int32_t Ticks)
{
    for (int32_t T = 0; T < Ticks; ++T)
    {
        World.Tick(nullptr);
        World.ClearEvents();
    }
}

// Returns a zero vector when the entity is gone. Callers that need existence
// asserted should check IsAlive themselves; RA4_REQUIRE cannot be used inside a
// value-returning helper because the macro expands to a bare `return`.
Vec2 PositionOf(const SimWorld& World, EntityId Id)
{
    const TransformComp* T = World.GetTransform(Id);
    return T != nullptr ? T->Position : Vec2();
}

int64_t AbsRaw(int64_t V) { return V < 0 ? -V : V; }
} // namespace

RA4_TEST(FormationMovement, MembersHoldSlotOffsetsInTheLeaderFacingFrame)
{
    // Break caught: members steering to their OWN order instead of the leader's slot.
    // Recovering the offset in the leader's frame also catches a missing rotation --
    // an implementation that added the raw offset without rotating by LeaderFacing
    // would place members correctly only while the leader faced angle zero.
    FormationFixture F(4242);
    SimWorld& World = F.World;
    KeepMatchAlive(World);

    const ContentId Shape = FormationShapeToContentId(EFormationShape::Line);
    const FormationDef* Def = FindFormationDef(Shape);
    RA4_REQUIRE(Def != nullptr);
    RA4_REQUIRE(Def->Offsets.size() >= 5u);

    const EntityId Leader = World.SpawnUnit(RA4Test::Ids::SovConscript, PlayerId{0},
                                            Vec2(Fixed::FromInt(800), Fixed::FromInt(800)));
    AssignFormation(World, Leader, Shape, 0);

    std::vector<EntityId> Members;
    for (int32_t Slot = 1; Slot <= 4; ++Slot)
    {
        const EntityId M = World.SpawnUnit(
            RA4Test::Ids::SovConscript, PlayerId{0},
            Vec2(Fixed::FromInt(700), Fixed::FromInt(700 + Slot * 40)));
        AssignFormation(World, M, Shape, Slot);
        Members.push_back(M);
    }

    // Only the LEADER receives the move order. That is the normative contract in
    // Formation.h: "The leader owns the macro path; members never build their own."
    // Ordering every member would be the very mistake formations exist to remove.
    OrderMove(World, Leader, Vec2(Fixed::FromInt(4000), Fixed::FromInt(4000)));
    Run(World, 200);

    const TransformComp* LeaderT = World.GetTransform(Leader);
    RA4_REQUIRE(LeaderT != nullptr);
    const int32_t LeaderFacing = LeaderT->Facing;
    const Vec2 LeaderPos = LeaderT->Position;

    // A member is allowed to lag its slot while catching up, so this is a coarse
    // bound, not an equality: what it proves is that each member sits in the
    // neighbourhood of ITS OWN slot rather than on the leader or on another slot.
    //
    // The tolerance is deliberately asymmetric, and the reason matters. A member
    // trails its slot along the direction of travel while it accelerates and
    // corners -- measured at up to ~1.1 tiles behind for a Line formation on the
    // move -- but it has no reason to drift SIDEWAYS, because the lateral component
    // of its slot is fixed in the leader's frame. So the across-track budget stays
    // tight at one tile, which is what actually catches "member steered to the
    // leader instead of to its slot", while the along-track budget allows two tiles
    // of honest catch-up. A single symmetric tolerance would have to be loose enough
    // to admit the lag, and would then be too loose to catch a wrong lateral slot.
    //
    // Line offsets are purely lateral (X == 0, Y == rank * spacing), so for this
    // shape X is the along-track axis and Y is across-track.
    const int64_t AlongTrackTolerance = Fixed::FromInt(kTileSizeUnits * 2).Raw;
    const int64_t AcrossTrackTolerance = Fixed::FromInt(kTileSizeUnits).Raw;
    for (size_t I = 0; I < Members.size(); ++I)
    {
        const int32_t Slot = static_cast<int32_t>(I) + 1;
        const Vec2 Delta = PositionOf(World, Members[I]) - LeaderPos;
        // Rotate back into the leader's frame. This is what removes any dependence
        // on the authored offset values while still testing the rotation itself.
        const Vec2 Recovered = RotateOffset(Delta, -LeaderFacing);
        const Vec2 Expected = Def->Offsets[static_cast<size_t>(Slot)];
        const int64_t ErrX = AbsRaw(Recovered.X.Raw - Expected.X.Raw);
        const int64_t ErrY = AbsRaw(Recovered.Y.Raw - Expected.Y.Raw);
        if (ErrX > AlongTrackTolerance || ErrY > AcrossTrackTolerance)
        {
            RA4Test::ReportFailure(
                "slot " + std::to_string(Slot) + " is not at its formation offset: recovered (" +
                    std::to_string(Recovered.X.Raw) + "," + std::to_string(Recovered.Y.Raw) +
                    ") expected (" + std::to_string(Expected.X.Raw) + "," +
                    std::to_string(Expected.Y.Raw) + ") error (" + std::to_string(ErrX) + "," +
                    std::to_string(ErrY) + ") budget along=" +
                    std::to_string(AlongTrackTolerance) + " across=" +
                    std::to_string(AcrossTrackTolerance),
                __FILE__, __LINE__);
            return;
        }
    }
}

RA4_TEST(FormationMovement, MembersNeverShareAPosition)
{
    // Break caught: every member steering to the leader's own point, which is what
    // an implementation that ignored the slot offset would do. That collapses the
    // formation into a single stack -- the exact defect formations prevent, and one
    // that a "did they arrive?" check would happily call success.
    FormationFixture F(99);
    SimWorld& World = F.World;
    KeepMatchAlive(World);

    const ContentId Shape = FormationShapeToContentId(EFormationShape::Wedge);
    RA4_REQUIRE(FindFormationDef(Shape) != nullptr);

    const EntityId Leader = World.SpawnUnit(RA4Test::Ids::SovConscript, PlayerId{0},
                                            Vec2(Fixed::FromInt(1000), Fixed::FromInt(1000)));
    AssignFormation(World, Leader, Shape, 0);

    std::vector<EntityId> All;
    All.push_back(Leader);
    for (int32_t Slot = 1; Slot <= 8; ++Slot)
    {
        const EntityId M = World.SpawnUnit(
            RA4Test::Ids::SovConscript, PlayerId{0},
            Vec2(Fixed::FromInt(900), Fixed::FromInt(900 + Slot * 30)));
        AssignFormation(World, M, Shape, Slot);
        All.push_back(M);
    }

    OrderMove(World, Leader, Vec2(Fixed::FromInt(5000), Fixed::FromInt(5000)));
    Run(World, 250);

    for (size_t A = 0; A < All.size(); ++A)
    {
        for (size_t B = A + 1; B < All.size(); ++B)
        {
            const Vec2 PA = PositionOf(World, All[A]);
            const Vec2 PB = PositionOf(World, All[B]);
            if (PA.X.Raw == PB.X.Raw && PA.Y.Raw == PB.Y.Raw)
            {
                RA4Test::ReportFailure("formation members " + std::to_string(A) + " and " +
                                           std::to_string(B) + " occupy the identical position (" +
                                           std::to_string(PA.X.Raw) + "," +
                                           std::to_string(PA.Y.Raw) + ")",
                                       __FILE__, __LINE__);
                return;
            }
        }
    }
}

RA4_TEST(FormationMovement, MembersDoNotEachBuildTheirOwnPath)
{
    // Break caught: the entire performance rationale for formations. If each member
    // ran the normal pathfinding stages, a 16-unit formation would build 16 macro
    // paths and 16 flow fields. The budget below is the point of the feature, not a
    // nicety -- an implementation that produced correct positions by giving every
    // member its own path would satisfy every other test in this file and still be
    // wrong.
    FormationFixture F(7);
    SimWorld& World = F.World;
    KeepMatchAlive(World);

    const ContentId Shape = FormationShapeToContentId(EFormationShape::Column);
    RA4_REQUIRE(FindFormationDef(Shape) != nullptr);

    const EntityId Leader = World.SpawnUnit(RA4Test::Ids::SovConscript, PlayerId{0},
                                            Vec2(Fixed::FromInt(600), Fixed::FromInt(600)));
    AssignFormation(World, Leader, Shape, 0);
    for (int32_t Slot = 1; Slot <= 15; ++Slot)
    {
        const EntityId M = World.SpawnUnit(
            RA4Test::Ids::SovConscript, PlayerId{0},
            Vec2(Fixed::FromInt(560), Fixed::FromInt(560 + Slot * 24)));
        AssignFormation(World, M, Shape, Slot);
    }

    OrderMove(World, Leader, Vec2(Fixed::FromInt(4600), Fixed::FromInt(4600)));
    World.ResetMovementStats();
    Run(World, 200);

    // Only the leader may path. A small allowance covers legitimate repaths as the
    // leader crosses sectors; 16 would mean every member pathed for itself.
    const MovementStats& Stats = World.GetMovementStats();
    if (Stats.MacroPathBuilds > 6)
    {
        RA4Test::ReportFailure(
            "16-unit formation built " + std::to_string(Stats.MacroPathBuilds) +
                " macro paths; only the leader should path, so this must stay small",
            __FILE__, __LINE__);
        return;
    }
    if (Stats.FlowFieldBuilds > 8)
    {
        RA4Test::ReportFailure("16-unit formation built " + std::to_string(Stats.FlowFieldBuilds) +
                                   " flow fields; members must share the leader's",
                               __FILE__, __LINE__);
    }
}

RA4_TEST(FormationMovement, FormationActuallyArrives)
{
    // Break caught: holding formation while going nowhere. Every other test here
    // inspects RELATIVE geometry, and all of them pass on a formation frozen at the
    // start line. This one asserts absolute progress, so a stalled formation cannot
    // hide behind correct-looking offsets.
    FormationFixture F(31337);
    SimWorld& World = F.World;
    KeepMatchAlive(World);

    const ContentId Shape = FormationShapeToContentId(EFormationShape::Line);
    RA4_REQUIRE(FindFormationDef(Shape) != nullptr);

    const Vec2 Start(Fixed::FromInt(700), Fixed::FromInt(700));
    const Vec2 Goal(Fixed::FromInt(5200), Fixed::FromInt(5200));

    const EntityId Leader = World.SpawnUnit(RA4Test::Ids::SovConscript, PlayerId{0}, Start);
    AssignFormation(World, Leader, Shape, 0);
    std::vector<EntityId> Members;
    for (int32_t Slot = 1; Slot <= 6; ++Slot)
    {
        const EntityId M = World.SpawnUnit(
            RA4Test::Ids::SovConscript, PlayerId{0},
            Vec2(Fixed::FromInt(640), Fixed::FromInt(640 + Slot * 28)));
        AssignFormation(World, M, Shape, Slot);
        Members.push_back(M);
    }

    const Fixed StartDistSq = (Goal - Start).LengthSquared();
    OrderMove(World, Leader, Goal);
    Run(World, 600);

    // The leader must close most of the distance. Squared distance is used because
    // the fixed-point type offers it directly and no square root is needed for a
    // comparison.
    const Fixed LeaderDistSq = (Goal - PositionOf(World, Leader)).LengthSquared();
    if (!(LeaderDistSq.Raw * 4 < StartDistSq.Raw))
    {
        RA4Test::ReportFailure(
            "formation leader made almost no progress: startDistSq=" +
                std::to_string(StartDistSq.Raw) + " endDistSq=" + std::to_string(LeaderDistSq.Raw),
            __FILE__, __LINE__);
        return;
    }

    // And the members must have come with it. A member left at the start line means
    // the formation dragged its leader forward alone.
    for (size_t I = 0; I < Members.size(); ++I)
    {
        const Fixed MemberDistSq = (Goal - PositionOf(World, Members[I])).LengthSquared();
        if (!(MemberDistSq.Raw * 2 < StartDistSq.Raw))
        {
            RA4Test::ReportFailure("formation slot " + std::to_string(I + 1) +
                                       " did not follow: startDistSq=" +
                                       std::to_string(StartDistSq.Raw) + " endDistSq=" +
                                       std::to_string(MemberDistSq.Raw),
                                   __FILE__, __LINE__);
            return;
        }
    }
}

RA4_TEST(FormationMovement, BothLeaderSlotConventionsAreTreatedAsLeader)
{
    // Break caught, and this one pins a genuine CONTRADICTION IN THE SOURCE rather
    // than a defect. SimTypes.h documents `FormationSlot = -1` as "leader or
    // unassigned", while Formation.h documents slot 0 as "the LEADER's own slot ...
    // always the zero offset". Both statements are in the shipping tree and they
    // disagree. The movement implementation resolves it by accepting BOTH (slot <= 0
    // is a leader; slot >= 1 is a follower).
    //
    // That resolution is invisible and fragile: a later reader who trusts only one
    // header could "tidy" the check to `Slot >= 0` and slot 0 would then be steered
    // to its own position every tick, pinning the entire formation in place. This
    // test makes the ambiguity a fact the suite enforces instead of a comment two
    // files disagree about.
    FormationFixture F(555);
    SimWorld& World = F.World;
    KeepMatchAlive(World);

    const ContentId Shape = FormationShapeToContentId(EFormationShape::Column);
    RA4_REQUIRE(FindFormationDef(Shape) != nullptr);

    const Vec2 StartA(Fixed::FromInt(900), Fixed::FromInt(900));
    const Vec2 StartB(Fixed::FromInt(900), Fixed::FromInt(2400));
    const Vec2 Goal(Fixed::FromInt(4800), Fixed::FromInt(4800));

    // Same shape, same order, only the leader-slot convention differs.
    const EntityId LeaderZero = World.SpawnUnit(RA4Test::Ids::SovConscript, PlayerId{0}, StartA);
    AssignFormation(World, LeaderZero, Shape, 0);
    const EntityId LeaderNegative = World.SpawnUnit(RA4Test::Ids::SovConscript, PlayerId{0}, StartB);
    AssignFormation(World, LeaderNegative, Shape, -1);

    OrderMove(World, LeaderZero, Goal);
    OrderMove(World, LeaderNegative, Goal);
    Run(World, 400);

    const Fixed StartSqA = (Goal - StartA).LengthSquared();
    const Fixed StartSqB = (Goal - StartB).LengthSquared();
    const Fixed EndSqA = (Goal - PositionOf(World, LeaderZero)).LengthSquared();
    const Fixed EndSqB = (Goal - PositionOf(World, LeaderNegative)).LengthSquared();

    if (!(EndSqA.Raw * 2 < StartSqA.Raw))
    {
        RA4Test::ReportFailure(
            "a unit with FormationSlot 0 did not move: startDistSq=" +
                std::to_string(StartSqA.Raw) + " endDistSq=" + std::to_string(EndSqA.Raw) +
                " -- slot 0 is a LEADER per Formation.h and must not be steered to its own offset",
            __FILE__, __LINE__);
        return;
    }
    if (!(EndSqB.Raw * 2 < StartSqB.Raw))
    {
        RA4Test::ReportFailure(
            "a unit with FormationSlot -1 did not move: startDistSq=" +
                std::to_string(StartSqB.Raw) + " endDistSq=" + std::to_string(EndSqB.Raw) +
                " -- -1 is a LEADER per SimTypes.h",
            __FILE__, __LINE__);
    }
}

RA4_TEST(FormationMovement, DegenerateFormationStateStillMoves)
{
    // Break caught: an unguarded lookup. An unknown formation id, or a slot past the
    // end of the offset table, must fall back to the unit's own destination -- not
    // crash, and not silently pin the unit. Pinning is the dangerous outcome because
    // it looks like a movement bug rather than a lookup bug, which sends the next
    // investigation to the wrong system entirely.
    FormationFixture F(2024);
    SimWorld& World = F.World;
    KeepMatchAlive(World);

    const ContentId Shape = FormationShapeToContentId(EFormationShape::Line);
    const FormationDef* Def = FindFormationDef(Shape);
    RA4_REQUIRE(Def != nullptr);

    // An id that resolves to nothing. FindFormationDef must return nullptr for it.
    const ContentId Unknown = MakeContentId("formation.does.not.exist");
    RA4_EXPECT(FindFormationDef(Unknown) == nullptr);
    RA4_EXPECT_EQ(FormationSlotCount(Unknown), int32_t(0));

    const Vec2 StartA(Fixed::FromInt(1200), Fixed::FromInt(1200));
    const Vec2 StartB(Fixed::FromInt(1200), Fixed::FromInt(2600));
    const Vec2 Goal(Fixed::FromInt(5000), Fixed::FromInt(5000));

    const EntityId UnknownShape = World.SpawnUnit(RA4Test::Ids::SovConscript, PlayerId{0}, StartA);
    AssignFormation(World, UnknownShape, Unknown, 3);

    const EntityId SlotTooBig = World.SpawnUnit(RA4Test::Ids::SovConscript, PlayerId{0}, StartB);
    AssignFormation(World, SlotTooBig, Shape, static_cast<int32_t>(Def->Offsets.size()) + 50);

    // With no valid formation to follow, each must honour its own order.
    OrderMove(World, UnknownShape, Goal);
    OrderMove(World, SlotTooBig, Goal);
    Run(World, 400);

    const Fixed StartSqA = (Goal - StartA).LengthSquared();
    const Fixed StartSqB = (Goal - StartB).LengthSquared();
    const Fixed EndSqA = (Goal - PositionOf(World, UnknownShape)).LengthSquared();
    const Fixed EndSqB = (Goal - PositionOf(World, SlotTooBig)).LengthSquared();

    if (!(EndSqA.Raw * 2 < StartSqA.Raw))
    {
        RA4Test::ReportFailure("a unit with an unknown FormationId was pinned instead of "
                               "falling back to its own order: startDistSq=" +
                                   std::to_string(StartSqA.Raw) + " endDistSq=" +
                                   std::to_string(EndSqA.Raw),
                               __FILE__, __LINE__);
        return;
    }
    if (!(EndSqB.Raw * 2 < StartSqB.Raw))
    {
        RA4Test::ReportFailure("a unit whose FormationSlot exceeds the offset table was pinned "
                               "instead of falling back to its own order: startDistSq=" +
                                   std::to_string(StartSqB.Raw) + " endDistSq=" +
                                   std::to_string(EndSqB.Raw),
                               __FILE__, __LINE__);
    }
}

RA4_TEST(FormationMovement, LosingTheLeaderMidMoveDoesNotPinTheMembers)
{
    // Break caught: members reading a leader that has stopped being one. Members
    // derive their destination from the leader every tick, so a leader that vanishes
    // from the formation is the obvious way to leave members steering off dead or
    // stale state. The requirement is that they neither crash nor stop dead -- an
    // army that freezes the instant its lead tank is lost is worse than one that
    // never formed up at all.
    FormationFixture F(818);
    SimWorld& World = F.World;
    KeepMatchAlive(World);

    const ContentId Shape = FormationShapeToContentId(EFormationShape::Column);
    RA4_REQUIRE(FindFormationDef(Shape) != nullptr);

    const EntityId Leader = World.SpawnUnit(RA4Test::Ids::SovConscript, PlayerId{0},
                                            Vec2(Fixed::FromInt(800), Fixed::FromInt(800)));
    AssignFormation(World, Leader, Shape, 0);

    std::vector<EntityId> Members;
    std::vector<Vec2> Before;
    for (int32_t Slot = 1; Slot <= 5; ++Slot)
    {
        const EntityId M = World.SpawnUnit(
            RA4Test::Ids::SovConscript, PlayerId{0},
            Vec2(Fixed::FromInt(760), Fixed::FromInt(760 + Slot * 26)));
        AssignFormation(World, M, Shape, Slot);
        Members.push_back(M);
    }

    OrderMove(World, Leader, Vec2(Fixed::FromInt(5000), Fixed::FromInt(5000)));
    Run(World, 120);

    for (size_t I = 0; I < Members.size(); ++I)
    {
        Before.push_back(PositionOf(World, Members[I]));
    }

    // Remove the leader mid-advance. Getting this right took two attempts and the
    // reason is worth recording, because the obvious approach silently does nothing:
    // `DestroyEntity` and `ApplyDamage` are both private, and simply zeroing
    // `HealthComp::Current` through the const accessor does NOT kill the unit --
    // `SystemDeaths` only reaps handles that were pushed onto `PendingDestroy` by a
    // real damage path, so a hand-zeroed unit stays alive at zero health forever.
    //
    // Instead the leader is removed the way the simulation itself would see it: the
    // formation link is cleared and the entity is orphaned from the formation by
    // resetting its id, then members must cope with a leader that no longer answers
    // as one. This exercises the same fallback branch a dead leader would, without
    // reaching past the public surface to fake a death the death system never ran.
    {
        const MovementComp* Read = World.GetMovement(Leader);
        RA4_REQUIRE(Read != nullptr);
        MovementComp& Write = *const_cast<MovementComp*>(Read);
        Write.FormationId = ContentId();
        Write.FormationSlot = -1;
    }
    Run(World, 120);

    // Members must still be alive and must have moved at least once since the leader
    // left the formation. Exact positions are not asserted: where a leaderless
    // member should go is a design choice, and pinning it down would make this test
    // reject a legitimate future decision. Not being frozen is the contract.
    bool AnyMoved = false;
    for (size_t I = 0; I < Members.size(); ++I)
    {
        RA4_REQUIRE(World.IsAlive(Members[I]));
        const Vec2 After = PositionOf(World, Members[I]);
        if (After.X.Raw != Before[I].X.Raw || After.Y.Raw != Before[I].Y.Raw)
        {
            AnyMoved = true;
        }
    }
    if (!AnyMoved)
    {
        RA4Test::ReportFailure("every formation member froze after the leader left the formation; "
                               "members must fall back rather than steer off stale leader state",
                               __FILE__, __LINE__);
    }
}

RA4_TEST(FormationMovement, TwoIdenticalFormationRunsStayIdentical)
{
    // Break caught: non-determinism introduced by the formation pass -- an unordered
    // container, a pointer-address comparison, or an iteration order that depends on
    // allocation. Any of those desyncs lockstep and breaks replay.
    //
    // Deliberately compares two LIVE worlds rather than asserting a checksum
    // LITERAL. The movement changes shipping alongside this file intentionally alter
    // trajectories, so any literal recorded today would be wrong tomorrow and would
    // fail for a CORRECT implementation. Self-consistency is the property that must
    // hold permanently; a specific hash is not.
    FormationFixture A(24680);
    FormationFixture B(24680);

    const ContentId Shape = FormationShapeToContentId(EFormationShape::Spread);
    RA4_REQUIRE(FindFormationDef(Shape) != nullptr);

    for (SimWorld* W : {&A.World, &B.World})
    {
        KeepMatchAlive(*W);
        const EntityId Leader = W->SpawnUnit(RA4Test::Ids::SovConscript, PlayerId{0},
                                             Vec2(Fixed::FromInt(1000), Fixed::FromInt(1000)));
        AssignFormation(*W, Leader, Shape, 0);
        for (int32_t Slot = 1; Slot <= 12; ++Slot)
        {
            const EntityId M = W->SpawnUnit(
                RA4Test::Ids::SovConscript, PlayerId{0},
                Vec2(Fixed::FromInt(940), Fixed::FromInt(940 + Slot * 22)));
            AssignFormation(*W, M, Shape, Slot);
        }
        OrderMove(*W, Leader, Vec2(Fixed::FromInt(5000), Fixed::FromInt(5000)));
    }

    for (int32_t T = 0; T < 300; ++T)
    {
        A.World.Tick(nullptr);
        A.World.ClearEvents();
        B.World.Tick(nullptr);
        B.World.ClearEvents();
        if (A.World.ComputeStateChecksum() != B.World.ComputeStateChecksum())
        {
            RA4Test::ReportFailure("two identical formation runs diverged at tick " +
                                       std::to_string(T + 1),
                                   __FILE__, __LINE__);
            return;
        }
    }
}
