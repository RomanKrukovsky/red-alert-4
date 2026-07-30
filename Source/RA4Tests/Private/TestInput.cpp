// Copyright (c) Red Alert 4 project. Tests for camera, selection and order resolution.
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Core/SimConfig.h"
#include <cmath>

#include "RA4Input/CameraController.h"
#include "RA4Input/ControlScheme.h"
#include "RA4Input/HitTest.h"
#include "RA4Input/OrderResolver.h"
#include "RA4Input/SelectionModel.h"

using namespace RA4;
using namespace RA4::Input;
using namespace RA4Test;

namespace
{

struct InputFixture
{
    ContentDatabase Content;
    SimWorld World;
    SelectionModel Selection;

    InputFixture()
    {
        BuildDefaultContent(Content);
        World.Initialize(&Content, MakeTestSetup(4242));
        Selection.SetLocalPlayer(0);
    }
};

CameraController MakeCamera()
{
    CameraController Cam;
    CameraConfig Config;
    Config.BorderMarginUnits = 0.0f;
    Cam.Configure(Config);
    Cam.SetViewportSize(1920.0f, 1080.0f);
    Cam.SetMapBounds(Vec2f(0.0f, 0.0f), Vec2f(12800.0f, 12800.0f));
    Cam.FocusOn(Vec2f(6400.0f, 6400.0f), true);
    return Cam;
}

void Advance(CameraController& Cam, float Seconds, float Step = 1.0f / 60.0f)
{
    for (float T = 0.0f; T < Seconds; T += Step)
    {
        Cam.Update(Step);
    }
}

OrderContext GroundClick(const Vec2& Where)
{
    OrderContext C;
    C.Issuer = 0;
    C.WorldLocation = Where;
    return C;
}

OrderContext EntityClick(const SimWorld& World, EntityId Target)
{
    OrderContext C;
    C.Issuer = 0;
    C.HoveredEntity = Target;
    const TransformComp* T = World.GetTransform(Target);
    C.WorldLocation = T != nullptr ? T->Position : Vec2::Zero();
    return C;
}

int32_t CountOfType(const std::vector<Command>& Commands, CommandType Type)
{
    int32_t Count = 0;
    for (const Command& C : Commands)
    {
        if (C.Type == Type)
        {
            ++Count;
        }
    }
    return Count;
}

} // namespace

// ---------------------------------------------------------------------------
// Camera
// ---------------------------------------------------------------------------

RA4_TEST(Camera, KeyboardPanMovesTheFocusAndSettles)
{
    CameraController Cam = MakeCamera();
    const Vec2f Start = Cam.GetFocus();

    Cam.SetKeyboardPan(1.0f, 0.0f);
    Advance(Cam, 1.0f);
    RA4_EXPECT(Cam.GetFocus().X > Start.X);

    // Releasing the key must stop the camera rather than let it coast forever.
    Cam.SetKeyboardPan(0.0f, 0.0f);
    Advance(Cam, 1.0f);
    const float Settled = Cam.GetFocus().X;
    Advance(Cam, 1.0f);
    RA4_EXPECT_NEAR(Cam.GetFocus().X, Settled, 1.0f);
}

RA4_TEST(Camera, DiagonalPanIsNotFasterThanCardinalPan)
{
    CameraController Cardinal = MakeCamera();
    Cardinal.SetKeyboardPan(1.0f, 0.0f);
    Advance(Cardinal, 1.0f);
    const float CardinalDistance = Cardinal.GetTargetFocus().X - 6400.0f;

    CameraController Diagonal = MakeCamera();
    Diagonal.SetKeyboardPan(1.0f, 1.0f);
    Advance(Diagonal, 1.0f);
    const float DX = Diagonal.GetTargetFocus().X - 6400.0f;
    const float DY = Diagonal.GetTargetFocus().Y - 6400.0f;
    const float DiagonalDistance = std::sqrt(DX * DX + DY * DY);

    // Without normalisation this would be 41% larger, which is the classic
    // "holding W+D is faster" bug.
    RA4_EXPECT_NEAR(DiagonalDistance, CardinalDistance, CardinalDistance * 0.02f);
}

RA4_TEST(Camera, FocusIsClampedToTheMap)
{
    CameraController Cam = MakeCamera();
    Cam.SetKeyboardPan(1.0f, 1.0f);
    Advance(Cam, 60.0f);
    RA4_EXPECT(Cam.GetTargetFocus().X <= 12800.0f + 0.01f);
    RA4_EXPECT(Cam.GetTargetFocus().Y <= 12800.0f + 0.01f);

    Cam.SetKeyboardPan(-1.0f, -1.0f);
    Advance(Cam, 60.0f);
    RA4_EXPECT(Cam.GetTargetFocus().X >= -0.01f);
    RA4_EXPECT(Cam.GetTargetFocus().Y >= -0.01f);
}

RA4_TEST(Camera, ZoomStaysWithinItsLimits)
{
    CameraController Cam = MakeCamera();
    const CameraConfig& Config = Cam.GetConfig();

    Cam.AddZoomNotches(1000.0f);
    RA4_EXPECT_NEAR(Cam.GetTargetHeight(), Config.MinHeight, 0.01f);
    Advance(Cam, 2.0f);
    RA4_EXPECT_NEAR(Cam.GetZoomAlpha(), 0.0f, 0.02f);

    Cam.AddZoomNotches(-1000.0f);
    RA4_EXPECT_NEAR(Cam.GetTargetHeight(), Config.MaxHeight, 0.01f);
    Advance(Cam, 2.0f);
    RA4_EXPECT_NEAR(Cam.GetZoomAlpha(), 1.0f, 0.02f);
}

RA4_TEST(Camera, EdgeScrollOnlyRunsWhenTheWindowIsFocused)
{
    CameraController Unfocused = MakeCamera();
    Unfocused.SetCursorPosition(2.0f, 500.0f, /*bWindowFocused*/ false);
    Advance(Unfocused, 1.0f);
    // Alt-tabbing away with the cursor parked at a screen edge must not slide the
    // camera off into the corner of the map.
    RA4_EXPECT_NEAR(Unfocused.GetTargetFocus().X, 6400.0f, 0.01f);

    CameraController Focused = MakeCamera();
    Focused.SetCursorPosition(2.0f, 500.0f, true);
    Advance(Focused, 1.0f);
    RA4_EXPECT(Focused.GetTargetFocus().X < 6400.0f);
}

RA4_TEST(Camera, EdgeScrollIsSuppressedWhileDragging)
{
    CameraController Cam = MakeCamera();
    Cam.SetCursorPosition(2.0f, 500.0f, true);
    Cam.BeginMiddleDrag(2.0f, 500.0f);
    Advance(Cam, 1.0f);
    RA4_EXPECT_NEAR(Cam.GetTargetFocus().X, 6400.0f, 0.01f);

    Cam.EndMiddleDrag();
    Advance(Cam, 1.0f);
    RA4_EXPECT(Cam.GetTargetFocus().X < 6400.0f);
}

RA4_TEST(Camera, PanIsFrameRateIndependent)
{
    CameraController Fast = MakeCamera();
    Fast.SetKeyboardPan(1.0f, 0.0f);
    for (int32_t I = 0; I < 240; ++I)
    {
        Fast.Update(1.0f / 240.0f);
    }

    CameraController Slow = MakeCamera();
    Slow.SetKeyboardPan(1.0f, 0.0f);
    for (int32_t I = 0; I < 30; ++I)
    {
        Slow.Update(1.0f / 30.0f);
    }

    // The commanded position is exact: panning integrates speed over elapsed time,
    // not per frame, so one second of input covers one second of distance at any
    // frame rate.
    RA4_EXPECT_NEAR(Fast.GetTargetFocus().X, Slow.GetTargetFocus().X, 0.5f);

    // The smoothed position trails it and cannot be identical -- an exponential
    // approach evaluated in 30 steps lands slightly ahead of one evaluated in 240 --
    // but the gap must stay far below anything a player could perceive.
    const float Travelled = Fast.GetFocus().X - 6400.0f;
    RA4_EXPECT(Travelled > 0.0f);
    RA4_EXPECT_NEAR(Fast.GetFocus().X, Slow.GetFocus().X, Travelled * 0.02f);
}

RA4_TEST(Camera, FocusOnJumpsInstantlyWhenAsked)
{
    CameraController Cam = MakeCamera();
    Cam.FocusOn(Vec2f(1000.0f, 2000.0f), true);
    RA4_EXPECT_NEAR(Cam.GetFocus().X, 1000.0f, 0.01f);
    RA4_EXPECT_NEAR(Cam.GetFocus().Y, 2000.0f, 0.01f);

    // A non-instant jump glides there instead of teleporting.
    Cam.FocusOn(Vec2f(9000.0f, 9000.0f), false);
    RA4_EXPECT(Cam.GetFocus().X < 2000.0f);
    Advance(Cam, 2.0f);
    RA4_EXPECT_NEAR(Cam.GetFocus().X, 9000.0f, 20.0f);
}

// ---------------------------------------------------------------------------
// Selection
// ---------------------------------------------------------------------------

RA4_TEST(Selection, ClickPrefersOwnUnitsOverEverythingElse)
{
    InputFixture F;
    const EntityId OwnBuilding = F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    const EntityId OwnUnit = F.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(2200, 2200));
    const EntityId EnemyUnit = F.World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(2200, 2200));
    RA4_REQUIRE(OwnBuilding.IsValid() && OwnUnit.IsValid() && EnemyUnit.IsValid());

    // All three overlap under the cursor.
    F.Selection.SelectAtCursor(F.World, {EnemyUnit, OwnBuilding, OwnUnit}, SelectionMode::Replace);
    RA4_EXPECT_EQ(F.Selection.Num(), 1);
    RA4_EXPECT(F.Selection.GetPrimary() == OwnUnit);

    F.Selection.SelectAtCursor(F.World, {EnemyUnit, OwnBuilding}, SelectionMode::Replace);
    RA4_EXPECT(F.Selection.GetPrimary() == OwnBuilding);

    F.Selection.SelectAtCursor(F.World, {EnemyUnit}, SelectionMode::Replace);
    RA4_EXPECT(F.Selection.GetPrimary() == EnemyUnit);
}

RA4_TEST(Selection, ClickingEmptyGroundClearsButShiftClickingDoesNot)
{
    InputFixture F;
    const EntityId Unit = F.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(2000, 2000));
    F.Selection.SelectAtCursor(F.World, {Unit}, SelectionMode::Replace);
    RA4_EXPECT_EQ(F.Selection.Num(), 1);

    F.Selection.SelectAtCursor(F.World, {}, SelectionMode::Add);
    RA4_EXPECT_EQ(F.Selection.Num(), 1);

    F.Selection.SelectAtCursor(F.World, {}, SelectionMode::Replace);
    RA4_EXPECT_EQ(F.Selection.Num(), 0);
}

RA4_TEST(Selection, MarqueeTakesOwnUnitsAndIgnoresEnemiesAndBuildings)
{
    InputFixture F;
    const EntityId Yard = F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    const EntityId A = F.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(2000, 2000));
    const EntityId B = F.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(2100, 2000));
    const EntityId Enemy = F.World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(2200, 2000));
    RA4_REQUIRE(Yard.IsValid() && A.IsValid() && B.IsValid() && Enemy.IsValid());

    F.Selection.SelectInMarquee(F.World, {Yard, A, Enemy, B}, SelectionMode::Replace);
    RA4_EXPECT_EQ(F.Selection.Num(), 2);
    RA4_EXPECT(F.Selection.IsSelected(A));
    RA4_EXPECT(F.Selection.IsSelected(B));
    RA4_EXPECT(!F.Selection.IsSelected(Enemy));
    RA4_EXPECT(!F.Selection.IsSelected(Yard));
}

RA4_TEST(Selection, MarqueeOverBaseWithNoUnitsPicksASingleBuilding)
{
    InputFixture F;
    const EntityId Yard = F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    const EntityId Power = F.World.SpawnBuilding(Ids::SovPower, 0, TileCoord(14, 10), true);
    RA4_REQUIRE(Yard.IsValid() && Power.IsValid());

    // Dragging a box across a base must not select twelve structures at once, or
    // "sell" becomes a catastrophe.
    F.Selection.SelectInMarquee(F.World, {Power, Yard}, SelectionMode::Replace);
    RA4_EXPECT_EQ(F.Selection.Num(), 1);
}

RA4_TEST(Selection, ShiftAddsAndCtrlToggles)
{
    InputFixture F;
    const EntityId A = F.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(2000, 2000));
    const EntityId B = F.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(2100, 2000));

    F.Selection.SelectAtCursor(F.World, {A}, SelectionMode::Replace);
    F.Selection.SelectAtCursor(F.World, {B}, SelectionMode::Add);
    RA4_EXPECT_EQ(F.Selection.Num(), 2);

    F.Selection.SelectAtCursor(F.World, {B}, SelectionMode::Toggle);
    RA4_EXPECT_EQ(F.Selection.Num(), 1);
    RA4_EXPECT(F.Selection.IsSelected(A));

    F.Selection.SelectAtCursor(F.World, {B}, SelectionMode::Toggle);
    RA4_EXPECT_EQ(F.Selection.Num(), 2);
}

RA4_TEST(Selection, DoubleClickSelectsOnlyTheSameUnitType)
{
    InputFixture F;
    const EntityId C1 = F.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(2000, 2000));
    const EntityId C2 = F.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(2100, 2000));
    const EntityId Tank = F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2200, 2000));
    const EntityId EnemyConscript = F.World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(2300, 2000));

    F.Selection.SelectSameType(F.World, C1, {C1, C2, Tank, EnemyConscript}, SelectionMode::Replace);
    RA4_EXPECT_EQ(F.Selection.Num(), 2);
    RA4_EXPECT(F.Selection.IsSelected(C1));
    RA4_EXPECT(F.Selection.IsSelected(C2));
    RA4_EXPECT(!F.Selection.IsSelected(Tank));
}

RA4_TEST(Selection, ControlGroupsAssignRecallAndForgetTheDead)
{
    InputFixture F;
    SpawnEnemyOutpost(F.World);
    const EntityId A = F.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(2000, 2000));
    const EntityId B = F.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(2100, 2000));

    F.Selection.SelectInMarquee(F.World, {A, B}, SelectionMode::Replace);
    RA4_EXPECT(F.Selection.AssignControlGroup(3));
    RA4_EXPECT_EQ(F.Selection.GetControlGroupSize(3, F.World), 2);

    F.Selection.Clear();
    RA4_EXPECT(F.Selection.RecallControlGroup(3, F.World));
    RA4_EXPECT_EQ(F.Selection.Num(), 2);

    // Kill one and the group must shrink instead of recalling a stale handle.
    const EntityId Killer = F.World.SpawnUnit(Ids::SovHeavyTank, 1, Vec2::FromInts(2050, 2000));
    RA4_REQUIRE(Killer.IsValid());
    RunUntil(F.World, SecondsToTicks(60), [&] { return !F.World.IsAlive(A) || !F.World.IsAlive(B); });

    F.Selection.RecallControlGroup(3, F.World);
    for (const EntityId& Id : F.Selection.Get())
    {
        RA4_EXPECT(F.World.IsAlive(Id));
    }

    RA4_EXPECT(!F.Selection.AssignControlGroup(-1));
    RA4_EXPECT(!F.Selection.AssignControlGroup(kControlGroupCount));
}

RA4_TEST(Selection, DeadUnitsLeaveTheSelection)
{
    InputFixture F;
    SpawnEnemyOutpost(F.World);
    const EntityId Victim = F.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(2000, 2000));
    const EntityId Killer = F.World.SpawnUnit(Ids::SovHeavyTank, 1, Vec2::FromInts(2300, 2000));
    RA4_REQUIRE(Victim.IsValid() && Killer.IsValid());

    F.Selection.SelectAtCursor(F.World, {Victim}, SelectionMode::Replace);
    RA4_EXPECT_EQ(F.Selection.Num(), 1);

    RunUntil(F.World, SecondsToTicks(60), [&] { return !F.World.IsAlive(Victim); });
    F.Selection.PruneDead(F.World);
    // A stale handle in the selection would make every following order bounce off
    // server validation with NoSuchEntity.
    RA4_EXPECT_EQ(F.Selection.Num(), 0);
}

RA4_TEST(Selection, SelectionIsCappedSoOrdersFitTheCommandBudget)
{
    InputFixture F;
    std::vector<EntityId> Many;
    for (int32_t I = 0; I < kMaxSelectedEntities + 40; ++I)
    {
        const EntityId Id = F.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(2000 + I * 5, 2000));
        if (Id.IsValid())
        {
            Many.push_back(Id);
        }
    }
    RA4_REQUIRE(int32_t(Many.size()) > kMaxSelectedEntities);

    F.Selection.SelectInMarquee(F.World, Many, SelectionMode::Replace);
    RA4_EXPECT_EQ(F.Selection.Num(), kMaxSelectedEntities);
}

// ---------------------------------------------------------------------------
// Order resolution
// ---------------------------------------------------------------------------

RA4_TEST(Orders, RightClickOnGroundIsAMoveForEverySelectedUnit)
{
    InputFixture F;
    const EntityId A = F.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(2000, 2000));
    const EntityId B = F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2100, 2000));
    F.Selection.SelectInMarquee(F.World, {A, B}, SelectionMode::Replace);

    const std::vector<Command> Orders = ResolveOrder(F.World, F.Selection, GroundClick(Vec2::FromInts(6000, 6000)));
    RA4_EXPECT_EQ(int32_t(Orders.size()), 2);
    RA4_EXPECT_EQ(CountOfType(Orders, CommandType::Move), 2);
    RA4_EXPECT(Orders[0].Mode == OrderMode::Replace);
    RA4_EXPECT(Orders[0].Location == Vec2::FromInts(6000, 6000));
}

RA4_TEST(Orders, RightClickOnAnEnemyAttacksWithArmedUnitsAndMovesTheRest)
{
    InputFixture F;
    const EntityId Tank = F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2000, 2000));
    const EntityId Harvester = F.World.SpawnUnit(Ids::SovHarvester, 0, Vec2::FromInts(2100, 2000));
    const EntityId Enemy = F.World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(9000, 9000));
    RA4_REQUIRE(Tank.IsValid() && Harvester.IsValid() && Enemy.IsValid());

    F.Selection.SelectInMarquee(F.World, {Tank, Harvester}, SelectionMode::Replace);
    const std::vector<Command> Orders = ResolveOrder(F.World, F.Selection, EntityClick(F.World, Enemy));

    RA4_EXPECT_EQ(int32_t(Orders.size()), 2);
    // The harvester cannot shoot, so ordering it to attack would be a command the
    // server rejects. It drives there instead.
    RA4_EXPECT_EQ(CountOfType(Orders, CommandType::Attack), 1);
    RA4_EXPECT_EQ(CountOfType(Orders, CommandType::Move), 1);
    for (const Command& C : Orders)
    {
        if (C.Type == CommandType::Attack)
        {
            RA4_EXPECT(C.Primary == Tank);
            RA4_EXPECT(C.Target == Enemy);
        }
    }
}

RA4_TEST(Orders, ShiftQueuesInsteadOfReplacing)
{
    InputFixture F;
    const EntityId Unit = F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2000, 2000));
    F.Selection.SelectAtCursor(F.World, {Unit}, SelectionMode::Replace);

    OrderContext Context = GroundClick(Vec2::FromInts(6000, 6000));
    Context.bQueueOrder = true;
    const std::vector<Command> Orders = ResolveOrder(F.World, F.Selection, Context);

    RA4_REQUIRE(Orders.size() == 1u);
    RA4_EXPECT(Orders[0].Mode == OrderMode::Queue);
}

RA4_TEST(Orders, ForceAttackTargetsAnAllyWhenCtrlIsHeld)
{
    InputFixture F;
    const EntityId Tank = F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2000, 2000));
    const EntityId Friend = F.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(2400, 2000));
    F.Selection.SelectAtCursor(F.World, {Tank}, SelectionMode::Replace);

    OrderContext Plain = EntityClick(F.World, Friend);
    const std::vector<Command> Moves = ResolveOrder(F.World, F.Selection, Plain);
    RA4_REQUIRE(Moves.size() == 1u);
    RA4_EXPECT(Moves[0].Type == CommandType::Move);

    OrderContext Forced = Plain;
    Forced.bForceAttack = true;
    const std::vector<Command> Attacks = ResolveOrder(F.World, F.Selection, Forced);
    RA4_REQUIRE(Attacks.size() == 1u);
    RA4_EXPECT(Attacks[0].Type == CommandType::Attack);
    RA4_EXPECT(Attacks[0].Target == Friend);
}

RA4_TEST(Orders, AttackMoveModeProducesAttackMove)
{
    InputFixture F;
    const EntityId Tank = F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2000, 2000));
    F.Selection.SelectAtCursor(F.World, {Tank}, SelectionMode::Replace);

    OrderContext Context = GroundClick(Vec2::FromInts(9000, 9000));
    Context.bAttackMoveMode = true;
    const std::vector<Command> Orders = ResolveOrder(F.World, F.Selection, Context);

    RA4_REQUIRE(Orders.size() == 1u);
    RA4_EXPECT(Orders[0].Type == CommandType::AttackMove);
}

RA4_TEST(Orders, HarvesterSentToOreGetsAHarvestOrder)
{
    InputFixture F;
    const EntityId Harvester = F.World.SpawnUnit(Ids::SovHarvester, 0, Vec2::FromInts(2000, 2000));
    const EntityId Tank = F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2100, 2000));
    const EntityId Ore = F.World.SpawnResourceNode(Ids::OreField, TileCoord(20, 20), 2000);
    RA4_REQUIRE(Harvester.IsValid() && Ore.IsValid());

    F.Selection.SelectInMarquee(F.World, {Harvester, Tank}, SelectionMode::Replace);
    const std::vector<Command> Orders = ResolveOrder(F.World, F.Selection, EntityClick(F.World, Ore));

    RA4_EXPECT_EQ(CountOfType(Orders, CommandType::Harvest), 1);
    // The tank has no business harvesting; it just drives there.
    RA4_EXPECT_EQ(CountOfType(Orders, CommandType::Move), 1);
}

RA4_TEST(Orders, PlacementModeEmitsPlaceBuilding)
{
    InputFixture F;
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);

    OrderContext Context;
    Context.Issuer = 0;
    Context.bPlacementMode = true;
    Context.PlacementContent = Ids::SovPower;
    Context.Tile = TileCoord(14, 10);

    const std::vector<Command> Orders = ResolveOrder(F.World, F.Selection, Context);
    RA4_REQUIRE(Orders.size() == 1u);
    RA4_EXPECT(Orders[0].Type == CommandType::PlaceBuilding);
    RA4_EXPECT(Orders[0].Tile == TileCoord(14, 10));
}

RA4_TEST(Orders, GroundClickWithOnlyBuildingsSelectedSetsTheRallyPoint)
{
    InputFixture F;
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    F.World.SpawnBuilding(Ids::SovPower, 0, TileCoord(14, 10), true);
    const EntityId Barracks = F.World.SpawnBuilding(Ids::SovBarracks, 0, TileCoord(10, 14), true);
    RA4_REQUIRE(Barracks.IsValid());

    F.Selection.SelectAtCursor(F.World, {Barracks}, SelectionMode::Replace);
    const std::vector<Command> Orders = ResolveOrder(F.World, F.Selection, GroundClick(Vec2::FromInts(5000, 5000)));

    RA4_REQUIRE(Orders.size() == 1u);
    RA4_EXPECT(Orders[0].Type == CommandType::SetRallyPoint);
    RA4_EXPECT(Orders[0].Primary == Barracks);
}

RA4_TEST(Orders, ResolvedOrdersAreAcceptedByTheServer)
{
    // The whole point of the resolver is to emit commands that pass validation.
    // Anything it produces for a legal gesture must not be rejected.
    InputFixture F;
    SpawnEnemyOutpost(F.World);
    const EntityId Tank = F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2000, 2000));
    const EntityId Enemy = F.World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(9000, 9000));
    F.Selection.SelectAtCursor(F.World, {Tank}, SelectionMode::Replace);

    for (const Command& C : ResolveOrder(F.World, F.Selection, GroundClick(Vec2::FromInts(6000, 6000))))
    {
        RA4_EXPECT(F.World.ApplyCommand(C).IsAccepted());
    }
    for (const Command& C : ResolveOrder(F.World, F.Selection, EntityClick(F.World, Enemy)))
    {
        RA4_EXPECT(F.World.ApplyCommand(C).IsAccepted());
    }
}

RA4_TEST(Orders, CursorHintMatchesWhatTheClickWillDo)
{
    InputFixture F;
    const EntityId Tank = F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2000, 2000));
    const EntityId Harvester = F.World.SpawnUnit(Ids::SovHarvester, 0, Vec2::FromInts(2100, 2000));
    const EntityId Enemy = F.World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(9000, 9000));
    const EntityId Ore = F.World.SpawnResourceNode(Ids::OreField, TileCoord(20, 20), 2000);

    F.Selection.SelectAtCursor(F.World, {Tank}, SelectionMode::Replace);
    RA4_EXPECT(ResolveCursorHint(F.World, F.Selection, GroundClick(Vec2::FromInts(5000, 5000))) == CursorHint::Move);
    RA4_EXPECT(ResolveCursorHint(F.World, F.Selection, EntityClick(F.World, Enemy)) == CursorHint::Attack);

    // A harvester alone cannot attack, so the cursor must not show a crosshair.
    F.Selection.SelectAtCursor(F.World, {Harvester}, SelectionMode::Replace);
    RA4_EXPECT(ResolveCursorHint(F.World, F.Selection, EntityClick(F.World, Enemy)) == CursorHint::Move);
    RA4_EXPECT(ResolveCursorHint(F.World, F.Selection, EntityClick(F.World, Ore)) == CursorHint::Harvest);

    F.Selection.Clear();
    RA4_EXPECT(ResolveCursorHint(F.World, F.Selection, GroundClick(Vec2::FromInts(5000, 5000))) == CursorHint::Select);
}

RA4_TEST(Orders, PlacementCursorRefusesIllegalGround)
{
    InputFixture F;
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);

    OrderContext Legal;
    Legal.Issuer = 0;
    Legal.bPlacementMode = true;
    Legal.PlacementContent = Ids::SovPower;
    Legal.Tile = TileCoord(14, 10);
    RA4_EXPECT(ResolveCursorHint(F.World, F.Selection, Legal) == CursorHint::Move);

    OrderContext OutsideBase = Legal;
    OutsideBase.Tile = TileCoord(50, 50);
    RA4_EXPECT(ResolveCursorHint(F.World, F.Selection, OutsideBase) == CursorHint::NoEntry);
}

// ---------------------------------------------------------------------------
// Picking geometry
// ---------------------------------------------------------------------------

namespace
{
std::vector<PickCandidate> MakeCandidates()
{
    // Three discs in a row, 400 units apart, radius 100.
    return {
        PickCandidate{EntityId(0, 0), Vec2::FromInts(1000, 1000), Fixed::FromInt(100)},
        PickCandidate{EntityId(1, 0), Vec2::FromInts(1400, 1000), Fixed::FromInt(100)},
        PickCandidate{EntityId(2, 0), Vec2::FromInts(1800, 1000), Fixed::FromInt(100)},
    };
}

void MakeAxisAlignedQuad(Vec2 Out[4], int64_t MinX, int64_t MinY, int64_t MaxX, int64_t MaxY)
{
    Out[0] = Vec2::FromInts(MinX, MinY);
    Out[1] = Vec2::FromInts(MaxX, MinY);
    Out[2] = Vec2::FromInts(MaxX, MaxY);
    Out[3] = Vec2::FromInts(MinX, MaxY);
}
} // namespace

RA4_TEST(HitTest, PointPickReturnsOnlyDiscsItTouches)
{
    const std::vector<PickCandidate> Candidates = MakeCandidates();

    const std::vector<EntityId> Hit = PickAtPoint(Candidates, Vec2::FromInts(1040, 1000));
    RA4_REQUIRE(Hit.size() == 1u);
    RA4_EXPECT_EQ(Hit[0].Index, 0u);

    // Exactly between two discs and outside both.
    RA4_EXPECT(PickAtPoint(Candidates, Vec2::FromInts(1200, 1000)).empty());

    // Tolerance widens every disc, which is how a forgiving click radius works.
    RA4_EXPECT_EQ(int32_t(PickAtPoint(Candidates, Vec2::FromInts(1200, 1000), Fixed::FromInt(150)).size()), 2);
}

RA4_TEST(HitTest, OverlappingDiscsResolveNearestFirstAndStably)
{
    std::vector<PickCandidate> Stacked = {
        PickCandidate{EntityId(7, 0), Vec2::FromInts(1000, 1000), Fixed::FromInt(300)},
        PickCandidate{EntityId(3, 0), Vec2::FromInts(1050, 1000), Fixed::FromInt(300)},
    };

    const std::vector<EntityId> Hit = PickAtPoint(Stacked, Vec2::FromInts(1060, 1000));
    RA4_REQUIRE(Hit.size() == 2u);
    // Slot 3 is centred closer to the click, so it ranks first despite being listed
    // second -- the caller's gather order must not decide what gets selected.
    RA4_EXPECT_EQ(Hit[0].Index, 3u);
    RA4_EXPECT_EQ(Hit[1].Index, 7u);
}

RA4_TEST(HitTest, MarqueeQuadSelectsWhatItCovers)
{
    const std::vector<PickCandidate> Candidates = MakeCandidates();

    Vec2 Quad[4];
    MakeAxisAlignedQuad(Quad, 900, 900, 1500, 1100);
    const std::vector<EntityId> Hit = PickInQuad(Candidates, Quad);
    RA4_REQUIRE(Hit.size() == 2u);
    RA4_EXPECT_EQ(Hit[0].Index, 0u);
    RA4_EXPECT_EQ(Hit[1].Index, 1u);

    MakeAxisAlignedQuad(Quad, 5000, 5000, 6000, 6000);
    RA4_EXPECT(PickInQuad(Candidates, Quad).empty());
}

RA4_TEST(HitTest, QuadWindingAndRotationDoNotMatter)
{
    Vec2 Clockwise[4];
    MakeAxisAlignedQuad(Clockwise, 0, 0, 1000, 1000);
    Vec2 CounterClockwise[4] = {Clockwise[0], Clockwise[3], Clockwise[2], Clockwise[1]};

    const Vec2 Inside = Vec2::FromInts(500, 500);
    const Vec2 Outside = Vec2::FromInts(1500, 500);
    RA4_EXPECT(IsPointInConvexQuad(Clockwise, Inside));
    RA4_EXPECT(IsPointInConvexQuad(CounterClockwise, Inside));
    RA4_EXPECT(!IsPointInConvexQuad(Clockwise, Outside));
    RA4_EXPECT(!IsPointInConvexQuad(CounterClockwise, Outside));

    // A rotated camera produces a diamond rather than an axis-aligned rectangle;
    // screen-space rectangle maths would get this wrong.
    Vec2 Diamond[4] = {Vec2::FromInts(500, 0), Vec2::FromInts(1000, 500), Vec2::FromInts(500, 1000),
                       Vec2::FromInts(0, 500)};
    RA4_EXPECT(IsPointInConvexQuad(Diamond, Vec2::FromInts(500, 500)));
    RA4_EXPECT(!IsPointInConvexQuad(Diamond, Vec2::FromInts(60, 60)));

    // A point on the boundary counts as inside rather than flickering.
    RA4_EXPECT(IsPointInConvexQuad(Clockwise, Vec2::FromInts(0, 500)));
}

RA4_TEST(HitTest, ShortDragCountsAsAClick)
{
    const Fixed Threshold = Fixed::FromInt(60);
    RA4_EXPECT(!IsDragSignificant(Vec2::FromInts(1000, 1000), Vec2::FromInts(1030, 1020), Threshold));
    RA4_EXPECT(IsDragSignificant(Vec2::FromInts(1000, 1000), Vec2::FromInts(1000, 1400), Threshold));
    // Direction is irrelevant: dragging up-left is as much a marquee as down-right.
    RA4_EXPECT(IsDragSignificant(Vec2::FromInts(1000, 1000), Vec2::FromInts(600, 900), Threshold));
}

// ---------------------------------------------------------------------------
// Control schemes
// ---------------------------------------------------------------------------

namespace
{
ClickFacts ClassicFacts(const InputFixture& F, EntityId Hovered)
{
    return MakeClickFacts(F.World, F.Selection, Hovered, /*bDragWasMarquee*/ false, /*bShift*/ false,
                          /*bCtrl*/ false, /*bAlt*/ false, /*bAttackMoveArmed*/ false,
                          /*bPlacementArmed*/ false);
}
} // namespace

RA4_TEST(ClassicScheme, LeftClickOnGroundIssuesTheOrder)
{
    InputFixture F;
    const EntityId Unit = F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2000, 2000));
    F.Selection.SelectAtCursor(F.World, {Unit}, SelectionMode::Replace);

    const ClickFacts Facts = ClassicFacts(F, EntityId::Invalid());
    RA4_EXPECT(RouteLeftClick(ControlScheme::ClassicRA, Facts) == ClickIntent::IssueOrder);
    // The same gesture under the modern scheme is a selection attempt.
    RA4_EXPECT(RouteLeftClick(ControlScheme::Modern, Facts) == ClickIntent::SelectAtPoint);
}

RA4_TEST(ClassicScheme, LeftClickOnOwnUnitSelectsRatherThanOrderingIntoIt)
{
    InputFixture F;
    const EntityId Selected = F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2000, 2000));
    const EntityId Other = F.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(3000, 3000));
    F.Selection.SelectAtCursor(F.World, {Selected}, SelectionMode::Replace);

    RA4_EXPECT(RouteLeftClick(ControlScheme::ClassicRA, ClassicFacts(F, Other)) == ClickIntent::SelectAtPoint);
}

RA4_TEST(ClassicScheme, LeftClickOnAnEnemyIssuesTheOrder)
{
    InputFixture F;
    const EntityId Tank = F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2000, 2000));
    const EntityId Enemy = F.World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(3000, 3000));
    F.Selection.SelectAtCursor(F.World, {Tank}, SelectionMode::Replace);

    RA4_EXPECT(RouteLeftClick(ControlScheme::ClassicRA, ClassicFacts(F, Enemy)) == ClickIntent::IssueOrder);
}

RA4_TEST(ClassicScheme, DraggingIsAlwaysAMarqueeEvenWithUnitsSelected)
{
    InputFixture F;
    const EntityId Tank = F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2000, 2000));
    F.Selection.SelectAtCursor(F.World, {Tank}, SelectionMode::Replace);

    ClickFacts Facts = ClassicFacts(F, EntityId::Invalid());
    Facts.bDragWasMarquee = true;
    // Without this the army would charge off to wherever the drag happened to end.
    RA4_EXPECT(RouteLeftClick(ControlScheme::ClassicRA, Facts) == ClickIntent::SelectInMarquee);
}

RA4_TEST(ClassicScheme, EmptySelectionMakesEveryLeftClickASelection)
{
    InputFixture F;
    const EntityId Enemy = F.World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(3000, 3000));
    RA4_EXPECT(RouteLeftClick(ControlScheme::ClassicRA, ClassicFacts(F, Enemy)) == ClickIntent::SelectAtPoint);
    RA4_EXPECT(RouteLeftClick(ControlScheme::ClassicRA, ClassicFacts(F, EntityId::Invalid())) ==
               ClickIntent::SelectAtPoint);
}

RA4_TEST(ClassicScheme, SelectingSomeoneElsesUnitsNeverArmsAnOrder)
{
    InputFixture F;
    const EntityId Enemy = F.World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(3000, 3000));
    F.Selection.SelectAtCursor(F.World, {Enemy}, SelectionMode::Replace);
    RA4_EXPECT(!F.Selection.IsEmpty());

    // Watching an enemy unit must not turn the next ground click into an order.
    RA4_EXPECT(RouteLeftClick(ControlScheme::ClassicRA, ClassicFacts(F, EntityId::Invalid())) ==
               ClickIntent::SelectAtPoint);
}

RA4_TEST(ClassicScheme, CtrlIsForceFireAndAltIsForceMove)
{
    InputFixture F;
    const EntityId Tank = F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2000, 2000));
    const EntityId Friend = F.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(3000, 3000));
    F.Selection.SelectAtCursor(F.World, {Tank}, SelectionMode::Replace);

    ClickFacts Ctrl = ClassicFacts(F, Friend);
    Ctrl.bCtrl = true;
    // Ctrl-clicking an ally is force fire, so it must beat the "own unit selects" rule.
    RA4_EXPECT(RouteLeftClick(ControlScheme::ClassicRA, Ctrl) == ClickIntent::IssueOrder);

    ClickFacts Alt = ClassicFacts(F, Friend);
    Alt.bAlt = true;
    RA4_EXPECT(RouteLeftClick(ControlScheme::ClassicRA, Alt) == ClickIntent::IssueOrder);
}

RA4_TEST(ClassicScheme, ShiftClickingOwnUnitsGrowsTheSelection)
{
    InputFixture F;
    const EntityId A = F.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(2000, 2000));
    const EntityId B = F.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(2100, 2000));
    F.Selection.SelectAtCursor(F.World, {A}, SelectionMode::Replace);

    ClickFacts Facts = ClassicFacts(F, B);
    Facts.bShift = true;
    RA4_EXPECT(RouteLeftClick(ControlScheme::ClassicRA, Facts) == ClickIntent::SelectAtPoint);
    RA4_EXPECT(ResolveSelectionMode(ControlScheme::ClassicRA, /*bShift*/ true, /*bCtrl*/ false) ==
               SelectionMode::Add);

    F.Selection.SelectAtCursor(F.World, {B},
                               ResolveSelectionMode(ControlScheme::ClassicRA, true, false));
    RA4_EXPECT(F.Selection.Num() == 2);
}

RA4_TEST(ClassicScheme, CtrlNeverTogglesSelectionTheWayTheModernSchemeDoes)
{
    // Ctrl is spoken for by force fire, so it must not also mean toggle-select --
    // one key doing both is how a force-fire order turns into a lost unit.
    RA4_EXPECT(ResolveSelectionMode(ControlScheme::ClassicRA, false, true) == SelectionMode::Replace);
    RA4_EXPECT(ResolveSelectionMode(ControlScheme::Modern, false, true) == SelectionMode::Toggle);
}

RA4_TEST(ClassicScheme, RightClickClearsTheSelectionInsteadOfOrdering)
{
    InputFixture F;
    const EntityId Tank = F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2000, 2000));
    F.Selection.SelectAtCursor(F.World, {Tank}, SelectionMode::Replace);
    const ClickFacts Facts = ClassicFacts(F, EntityId::Invalid());

    RA4_EXPECT(RouteRightClick(ControlScheme::ClassicRA, Facts, /*bSelectionEmpty*/ false) ==
               ClickIntent::ClearSelection);
    RA4_EXPECT(RouteRightClick(ControlScheme::Modern, Facts, false) == ClickIntent::IssueOrder);
    // Nothing selected: the button does nothing rather than firing a stray order.
    RA4_EXPECT(RouteRightClick(ControlScheme::ClassicRA, Facts, true) == ClickIntent::None);
}

RA4_TEST(ClassicScheme, ArmedModesOwnTheClickInBothSchemes)
{
    InputFixture F;
    const EntityId Tank = F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2000, 2000));
    F.Selection.SelectAtCursor(F.World, {Tank}, SelectionMode::Replace);

    ClickFacts Placement = ClassicFacts(F, EntityId::Invalid());
    Placement.bPlacementArmed = true;
    Placement.bDragWasMarquee = true;   // even a sloppy drag still places the building
    RA4_EXPECT(RouteLeftClick(ControlScheme::ClassicRA, Placement) == ClickIntent::IssueOrder);
    RA4_EXPECT(RouteRightClick(ControlScheme::ClassicRA, Placement, false) == ClickIntent::CancelArmedMode);

    ClickFacts AttackMove = ClassicFacts(F, EntityId::Invalid());
    AttackMove.bAttackMoveArmed = true;
    RA4_EXPECT(RouteLeftClick(ControlScheme::Modern, AttackMove) == ClickIntent::IssueOrder);
    RA4_EXPECT(RouteRightClick(ControlScheme::Modern, AttackMove, false) == ClickIntent::CancelArmedMode);
}

RA4_TEST(Orders, AltForcesAPlainMoveOntoAnOccupiedSpot)
{
    InputFixture F;
    const EntityId Tank = F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2000, 2000));
    const EntityId Enemy = F.World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(9000, 9000));
    F.Selection.SelectAtCursor(F.World, {Tank}, SelectionMode::Replace);

    OrderContext Context = EntityClick(F.World, Enemy);
    RA4_EXPECT(CountOfType(ResolveOrder(F.World, F.Selection, Context), CommandType::Attack) == 1);

    Context.bForceMove = true;
    const std::vector<Command> Forced = ResolveOrder(F.World, F.Selection, Context);
    RA4_EXPECT(CountOfType(Forced, CommandType::Move) == 1);
    RA4_EXPECT(CountOfType(Forced, CommandType::Attack) == 0);
    // The cursor has to promise the same thing the click will do.
    RA4_EXPECT(ResolveCursorHint(F.World, F.Selection, Context) == CursorHint::Move);
}

RA4_TEST(Orders, ForceMoveOutranksForceAttackAndAttackMove)
{
    InputFixture F;
    const EntityId Tank = F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2000, 2000));
    const EntityId Enemy = F.World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(9000, 9000));
    F.Selection.SelectAtCursor(F.World, {Tank}, SelectionMode::Replace);

    OrderContext Context = EntityClick(F.World, Enemy);
    Context.bForceMove = true;
    Context.bForceAttack = true;
    Context.bAttackMoveMode = true;

    const std::vector<Command> Commands = ResolveOrder(F.World, F.Selection, Context);
    RA4_EXPECT(CountOfType(Commands, CommandType::Move) == 1);
    RA4_EXPECT(CountOfType(Commands, CommandType::AttackMove) == 0);
    RA4_EXPECT(CountOfType(Commands, CommandType::Attack) == 0);
}

RA4_TEST(Selection, SelectIdleUnitsSelectsOnlyIdleUnits)
{
    InputFixture F;
    const EntityId IdleTank = F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2000, 2000));
    const EntityId BusyTank = F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(3000, 3000));
    
    // Direct push into OrderQueue for test setup
    const OrderQueue* OrdersConst = F.World.GetOrders(BusyTank);
    if (OrdersConst != nullptr)
    {
        OrderQueue* Orders = const_cast<OrderQueue*>(OrdersConst);
        Order O;
        O.Type = OrderType::Move;
        Orders->Push(O);
    }

    F.Selection.SelectIdleUnits(F.World, SelectionMode::Replace);
    RA4_EXPECT(F.Selection.Num() == 1);
    RA4_EXPECT(F.Selection.IsSelected(IdleTank));
    RA4_EXPECT(!F.Selection.IsSelected(BusyTank));
}



RA4_TEST(Selection, SelectWoundedUnitsSelectsDamagedUnits)
{
    InputFixture F;
    const EntityId HealthyTank = F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2000, 2000));
    const EntityId WoundedTank = F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(3000, 3000));

    F.World.DebugDamage(WoundedTank, 300); // Damage WoundedTank

    F.Selection.SelectWoundedUnits(F.World, 80, SelectionMode::Replace);
    RA4_EXPECT(F.Selection.Num() == 1);
    RA4_EXPECT(F.Selection.IsSelected(WoundedTank));
    RA4_EXPECT(!F.Selection.IsSelected(HealthyTank));
}



RA4_TEST(Camera, PanFollowsTheCameraWhenItRotates)
{
    // Screen-relative panning: "forward" must move the focus in whatever direction
    // the camera is currently facing. Without this the moment the view is rotated the
    // keys feel swapped, which is the exact complaint that motivated the test.
    CameraController Cam = MakeCamera();
    const Vec2f Start = Cam.GetFocus();

    // Yaw 0: the camera looks down +Y, so pushing forward increases Y and leaves X.
    Cam.SetKeyboardPan(0.0f, 1.0f);
    Advance(Cam, 0.5f);
    const Vec2f Unrotated = Cam.GetFocus();
    RA4_EXPECT(Unrotated.Y > Start.Y + 1.0f);
    RA4_EXPECT(std::fabs(Unrotated.X - Start.X) < 1.0f);

    // Turn a quarter turn and push forward again: the motion must now be along the
    // other world axis, not still along +Y.
    CameraController Rotated = MakeCamera();
    Rotated.AddYawDegrees(90.0f);
    const Vec2f RotatedStart = Rotated.GetFocus();
    Rotated.SetKeyboardPan(0.0f, 1.0f);
    Advance(Rotated, 0.5f);
    const Vec2f RotatedEnd = Rotated.GetFocus();

    RA4_EXPECT(std::fabs(RotatedEnd.Y - RotatedStart.Y) < 1.0f);
    RA4_EXPECT(std::fabs(RotatedEnd.X - RotatedStart.X) > 1.0f);
}

RA4_TEST(Camera, YawWrapsAndStaysWithinOneTurn)
{
    CameraController Cam = MakeCamera();
    Cam.AddYawDegrees(370.0f);
    RA4_EXPECT(Cam.GetYawDegrees() >= 0.0f && Cam.GetYawDegrees() < 360.0f);
    Cam.AddYawDegrees(-720.0f);
    RA4_EXPECT(Cam.GetYawDegrees() >= 0.0f && Cam.GetYawDegrees() < 360.0f);
}
