// Copyright (c) Red Alert 4 project.
#include "RA4PlayerController.h"

#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerInput.h"
#include "RA4CameraPawn.h"
#include "RA4SimCoords.h"
#include "RA4SimWorldSubsystem.h"
#include "RA4HUDWidget.h"
#include "Blueprint/UserWidget.h"
#include "UnrealClient.h"

#include "RA4Content/ContentDatabase.h"
#include "RA4Core/SimConfig.h"
#include "RA4Simulation/SimWorld.h"

using namespace RA4;
using namespace RA4::Input;

ARA4PlayerController::ARA4PlayerController()
{
    PrimaryActorTick.bCanEverTick = true;
    bShowMouseCursor = true;
    bEnableClickEvents = false;      // picking is done against the simulation, not actors
    bEnableMouseOverEvents = false;
    DefaultMouseCursor = EMouseCursor::Default;
}

void ARA4PlayerController::BeginPlay()
{
    Super::BeginPlay();

    // Single player for now; the lobby assigns this once networking lands.
    Selection.SetLocalPlayer(0);

    if (IsLocalController())
    {
        ResourceBar = CreateWidget<URA4ResourceBarWidget>(this, URA4ResourceBarWidget::StaticClass());
        if (ResourceBar != nullptr)
        {
            ResourceBar->AddToViewport(/*ZOrder*/ 10);
            UE_LOG(LogTemp, Display, TEXT("RA4 HUD: resource bar added to viewport"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("RA4 HUD: failed to create the resource bar"));
        }
    }
    TryInitializeCamera();
}

void ARA4PlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    bCameraInitialized = false;
    bInitialCursorPositionSet = false;
    EdgeScrollWarmupFrames = 0;
    TryInitializeCamera();
}

bool ARA4PlayerController::TryInitializeCamera()
{
    if (ARA4CameraPawn* CameraPawn = GetCameraPawn())
    {
        CameraController& Camera = CameraPawn->GetCameraController();

        if (const URA4SimWorldSubsystem* Subsystem = GetSimSubsystem())
        {
            if (const SimWorld* World = Subsystem->GetSimWorld())
            {
                const MapDescription& Map = World->GetMap();
                const float MaxX = float(Map.Width) * float(RA4::kTileSizeUnits);
                const float MaxY = float(Map.Height) * float(RA4::kTileSizeUnits);
                Vec2f OpeningFocus(MaxX * 0.25f, MaxY * 0.25f);
                const std::vector<EntityCore>& Cores = World->GetAllCores();
                const std::vector<TransformComp>& Transforms = World->GetAllTransforms();
                for (uint32 Index = 0; Index < uint32(Cores.size()); ++Index)
                {
                    if (Cores[Index].bAlive && Cores[Index].Owner == 0 &&
                        Cores[Index].Kind == EntityKind::Building)
                    {
                        OpeningFocus = Vec2f(
                            float(Transforms[Index].Position.X.ToDoubleUnsafe()),
                            float(Transforms[Index].Position.Y.ToDoubleUnsafe()));
                        break;
                    }
                }
                Camera.SetMapBounds(Vec2f(0.0f, 0.0f), Vec2f(MaxX, MaxY));
                Camera.FocusOn(OpeningFocus, true);

                bCameraInitialized = true;
                UE_LOG(LogTemp, Display, TEXT("RA4 camera initialized at %.0f, %.0f"),
                       OpeningFocus.X, OpeningFocus.Y);
                return true;
            }
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Input binding
// ---------------------------------------------------------------------------

void ARA4PlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    if (InputComponent == nullptr)
    {
        return;
    }

    // Bound directly to keys rather than through an InputMappingContext asset: the
    // whole control scheme stays in code and in version control, and the project
    // needs no editor-authored asset to be playable. Remapping moves to Enhanced
    // Input with the settings screen.
    InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ARA4PlayerController::OnPrimaryPressed);
    InputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &ARA4PlayerController::OnPrimaryReleased);
    InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &ARA4PlayerController::OnSecondaryPressed);
    InputComponent->BindKey(EKeys::MiddleMouseButton, IE_Pressed, this, &ARA4PlayerController::OnMiddlePressed);
    InputComponent->BindKey(EKeys::MiddleMouseButton, IE_Released, this, &ARA4PlayerController::OnMiddleReleased);

    InputComponent->BindKey(EKeys::MouseScrollUp, IE_Pressed, this, &ARA4PlayerController::OnZoomIn);
    InputComponent->BindKey(EKeys::MouseScrollDown, IE_Pressed, this, &ARA4PlayerController::OnZoomOut);

    // Letter keys are orders, not camera movement -- the originals scroll with the
    // arrows, the screen edge and the minimap, which leaves the whole keyboard free
    // for the command hotkeys players already have in their fingers.
    InputComponent->BindKey(EKeys::A, IE_Pressed, this, &ARA4PlayerController::ArmAttackMove);
    InputComponent->BindKey(EKeys::S, IE_Pressed, this, &ARA4PlayerController::OnStopPressed);
    InputComponent->BindKey(EKeys::G, IE_Pressed, this, &ARA4PlayerController::OnGuardPressed);
    InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ARA4PlayerController::CancelPendingAction);

    // Control groups 1..9 then 0, matching the on-screen numbering.
    static const FKey GroupKeys[10] = {EKeys::One, EKeys::Two,   EKeys::Three, EKeys::Four, EKeys::Five,
                                       EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine, EKeys::Zero};
    for (int32 Index = 0; Index < 10; ++Index)
    {
        InputComponent->BindKey(GroupKeys[Index], IE_Pressed, this, &ARA4PlayerController::OnControlGroupKeyByKey);
    }
}

// ---------------------------------------------------------------------------
// Frame update
// ---------------------------------------------------------------------------

void ARA4PlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);

    if (!bCameraInitialized)
    {
        TryInitializeCamera();
    }

    // The viewport is still 0x0 during possession in a standalone launch. Wait
    // until it has a real size, then place the inherited system cursor away from
    // the edge and give Slate two frames to report the new coordinates.
    if (!bInitialCursorPositionSet)
    {
        int32 ViewportWidth = 0;
        int32 ViewportHeight = 0;
        GetViewportSize(ViewportWidth, ViewportHeight);
        if (ViewportWidth > 0 && ViewportHeight > 0)
        {
            SetMouseLocation(ViewportWidth / 2, ViewportHeight / 2);
            bInitialCursorPositionSet = true;
            EdgeScrollWarmupFrames = 2;
        }
    }

    const URA4SimWorldSubsystem* Subsystem = GetSimSubsystem();
    const SimWorld* World = Subsystem != nullptr ? Subsystem->GetSimWorld() : nullptr;
    if (World == nullptr)
    {
        return;
    }

    // A unit that died this tick has to leave the selection immediately, or the
    // next order references a stale handle and the server rejects it.
    Selection.PruneDead(*World);

    UpdateCameraInput(DeltaTime);
    if (EdgeScrollWarmupFrames > 0)
    {
        --EdgeScrollWarmupFrames;
    }

    float MouseX = 0.0f;
    float MouseY = 0.0f;
    if (GetMousePosition(MouseX, MouseY))
    {
        MarqueeCurrentScreen = FVector2D(MouseX, MouseY);
    }

    Vec2 Ground;
    if (GetCursorGroundPosition(Ground))
    {
        // Under the classic scheme the cursor must show what the LEFT button will do,
        // and left-clicking our own units picks them rather than ordering the current
        // selection into them.
        const OrderContext Context = MakeOrderContext(Ground);
        const ClickFacts Facts = MakeClickFacts(*World, Selection, Context.HoveredEntity, /*bDragWasMarquee*/ false,
                                                Context.bQueueOrder, Context.bForceAttack, Context.bForceMove,
                                                bAttackMoveArmed, bPlacementArmed);
        // Whichever button carries orders in the active scheme is the one the cursor
        // previews.
        const ClickIntent Intent = Scheme == ControlScheme::ClassicRA
                                       ? RouteLeftClick(Scheme, Facts)
                                       : RouteRightClick(Scheme, Facts, Selection.IsEmpty());
        CurrentCursorHint =
            Intent == ClickIntent::IssueOrder ? ResolveCursorHint(*World, Selection, Context) : CursorHint::Select;
        ApplyCursorShape();
    }
}

void ARA4PlayerController::UpdateCameraInput(float DeltaTime)
{
    ARA4CameraPawn* CameraPawn = GetCameraPawn();
    if (CameraPawn == nullptr)
    {
        return;
    }
    CameraController& Camera = CameraPawn->GetCameraController();

    // Arrow keys only. The originals scroll with the screen edge, the arrows and the
    // minimap, and the letter keys are all order hotkeys -- binding WASD to panning
    // meant A both armed an attack-move and slid the camera sideways.
    const float Right = (IsInputKeyDown(EKeys::Right) ? 1.0f : 0.0f) -
                        (IsInputKeyDown(EKeys::Left) ? 1.0f : 0.0f);
    const float Up = (IsInputKeyDown(EKeys::Up) ? 1.0f : 0.0f) -
                     (IsInputKeyDown(EKeys::Down) ? 1.0f : 0.0f);

    Camera.SetKeyboardPan(Right, Up);
    Camera.SetFastPan(IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift));

    int32 SizeX = 0;
    int32 SizeY = 0;
    GetViewportSize(SizeX, SizeY);
    Camera.SetViewportSize(float(SizeX), float(SizeY));

    float MouseX = 0.0f;
    float MouseY = 0.0f;
    const bool bHasCursor = GetMousePosition(MouseX, MouseY);
    // Reports focus so edge scrolling stops when the player alt-tabs away with the
    // cursor parked against a screen border.
    const ULocalPlayer* LocalPlayer = GetLocalPlayer();
    const UGameViewportClient* ViewportClient = LocalPlayer != nullptr ? LocalPlayer->ViewportClient : nullptr;
    const bool bFocused = bInitialCursorPositionSet && EdgeScrollWarmupFrames == 0 && bHasCursor &&
                          ViewportClient != nullptr && ViewportClient->Viewport != nullptr &&
                          ViewportClient->Viewport->HasFocus();
    Camera.SetCursorPosition(MouseX, MouseY, bFocused);

    if (Camera.IsMiddleDragging() && bHasCursor)
    {
        Camera.UpdateMiddleDrag(MouseX, MouseY);
    }

    // The pawn ticks itself; nothing to advance here.
    (void)DeltaTime;
}

// ---------------------------------------------------------------------------
// Picking
// ---------------------------------------------------------------------------

bool ARA4PlayerController::ScreenToGround(const FVector2D& ScreenPosition, Vec2& OutPosition) const
{
    FVector RayOrigin;
    FVector RayDirection;
    if (!DeprojectScreenPositionToWorld(ScreenPosition.X, ScreenPosition.Y, RayOrigin, RayDirection))
    {
        return false;
    }
    FVector Hit;
    if (!RA4Coords::IntersectGroundPlane(RayOrigin, RayDirection, Hit))
    {
        return false;
    }
    OutPosition = RA4Coords::FromUnreal(Hit);
    return true;
}

bool ARA4PlayerController::GetCursorGroundPosition(Vec2& OutPosition) const
{
    float MouseX = 0.0f;
    float MouseY = 0.0f;
    if (!GetMousePosition(MouseX, MouseY))
    {
        return false;
    }
    return ScreenToGround(FVector2D(MouseX, MouseY), OutPosition);
}

void ARA4PlayerController::BuildPickCandidates(TArray<PickCandidate>& OutCandidates) const
{
    OutCandidates.Reset();

    const URA4SimWorldSubsystem* Subsystem = GetSimSubsystem();
    const SimWorld* World = Subsystem != nullptr ? Subsystem->GetSimWorld() : nullptr;
    if (World == nullptr || World->GetContent() == nullptr)
    {
        return;
    }

    const std::vector<EntityCore>& Cores = World->GetAllCores();
    const std::vector<TransformComp>& Transforms = World->GetAllTransforms();
    const RA4::Fixed Tolerance = RA4::Fixed::FromInt(int64(PickToleranceUnits));

    for (uint32 Index = 0; Index < uint32(Cores.size()); ++Index)
    {
        const EntityCore& Core = Cores[Index];
        if (!Core.bAlive || Core.Kind == EntityKind::Projectile)
        {
            continue;
        }

        const EntityDef* Def = World->GetContent()->FindEntity(Core.Def);
        RA4::Fixed Radius = RA4::Fixed::FromInt(60);
        if (Def != nullptr)
        {
            if (Def->Kind == EntityKind::Building)
            {
                // Half the footprint diagonal, so a 3x3 factory is as easy to click
                // as it looks.
                const int64 HalfSpanTiles = FMath::Max(Def->Building.FootprintX, Def->Building.FootprintY);
                Radius = RA4::Fixed::FromInt((HalfSpanTiles * RA4::kTileSizeUnits) / 2);
            }
            else if (Def->Kind == EntityKind::Unit)
            {
                Radius = FxMax(Def->Unit.CollisionRadius, RA4::Fixed::FromInt(45));
            }
        }

        OutCandidates.Add(PickCandidate{World->MakeId(Index), Transforms[Index].Position, Radius + Tolerance});
    }
}

EntityId ARA4PlayerController::FindHoveredEntity(const Vec2& GroundPosition) const
{
    TArray<PickCandidate> Candidates;
    BuildPickCandidates(Candidates);

    std::vector<PickCandidate> AsVector(Candidates.GetData(), Candidates.GetData() + Candidates.Num());
    const std::vector<EntityId> Hits = PickAtPoint(AsVector, GroundPosition);
    return Hits.empty() ? EntityId::Invalid() : Hits.front();
}

OrderContext ARA4PlayerController::MakeOrderContext(const Vec2& GroundPosition) const
{
    OrderContext Context;
    Context.Issuer = Selection.GetLocalPlayer();
    Context.WorldLocation = GroundPosition;
    Context.Tile = TileCoord(int32(GroundPosition.X.ToIntFloor() / RA4::kTileSizeUnits),
                             int32(GroundPosition.Y.ToIntFloor() / RA4::kTileSizeUnits));
    Context.bQueueOrder = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);
    Context.bForceAttack = IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl);
    Context.bForceMove = IsInputKeyDown(EKeys::LeftAlt) || IsInputKeyDown(EKeys::RightAlt);
    Context.bAttackMoveMode = bAttackMoveArmed;
    Context.bPlacementMode = bPlacementArmed;
    Context.PlacementContent = PlacementContent;
    Context.HoveredEntity = FindHoveredEntity(GroundPosition);
    return Context;
}

// ---------------------------------------------------------------------------
// Mouse handling
// ---------------------------------------------------------------------------

void ARA4PlayerController::OnPrimaryPressed()
{
    float MouseX = 0.0f;
    float MouseY = 0.0f;
    if (!GetMousePosition(MouseX, MouseY))
    {
        return;
    }
    MarqueeStartScreen = FVector2D(MouseX, MouseY);
    MarqueeCurrentScreen = MarqueeStartScreen;

    if (ScreenToGround(MarqueeStartScreen, MarqueeStartGround))
    {
        bMarqueeActive = true;
    }
}

void ARA4PlayerController::OnPrimaryReleased()
{
    const bool bWasPressed = bMarqueeActive;
    bMarqueeActive = false;
    if (!bWasPressed)
    {
        return;
    }

    float MouseX = 0.0f;
    float MouseY = 0.0f;
    if (!GetMousePosition(MouseX, MouseY))
    {
        return;
    }
    const FVector2D EndScreen(MouseX, MouseY);

    Vec2 EndGround;
    if (!ScreenToGround(EndScreen, EndGround))
    {
        return;
    }
    const bool bWasDrag =
        IsDragSignificant(MarqueeStartGround, EndGround, RA4::Fixed::FromInt(int64(MarqueeMinimumExtentUnits)));

    HandleClick(/*bLeftButton*/ true, EndScreen, bWasDrag);
}

// Both buttons land here; ControlScheme decides what the gesture means, so switching
// between the classic and modern layouts changes one enum and nothing else.
void ARA4PlayerController::HandleClick(bool bLeftButton, const FVector2D& EndScreen, bool bWasDrag)
{
    const URA4SimWorldSubsystem* Subsystem = GetSimSubsystem();
    const SimWorld* World = Subsystem != nullptr ? Subsystem->GetSimWorld() : nullptr;
    if (World == nullptr)
    {
        return;
    }

    Vec2 EndGround;
    if (!ScreenToGround(EndScreen, EndGround))
    {
        return;
    }

    const bool bShift = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);
    const bool bCtrl = IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl);
    const bool bAlt = IsInputKeyDown(EKeys::LeftAlt) || IsInputKeyDown(EKeys::RightAlt);

    const ClickFacts Facts = MakeClickFacts(*World, Selection, FindHoveredEntity(EndGround), bWasDrag, bShift,
                                            bCtrl, bAlt, bAttackMoveArmed, bPlacementArmed);
    const ClickIntent Intent = bLeftButton ? RouteLeftClick(Scheme, Facts)
                                           : RouteRightClick(Scheme, Facts, Selection.IsEmpty());

    switch (Intent)
    {
    case ClickIntent::IssueOrder:
        SubmitOrders(ResolveOrder(*World, Selection, MakeOrderContext(EndGround)));
        // An armed mode is spent by the click that used it, successful or not: the
        // alternative is a player who keeps placing buildings they thought they had
        // already placed.
        bAttackMoveArmed = false;
        bPlacementArmed = false;
        PlacementContent = ContentId();
        break;

    case ClickIntent::SelectAtPoint:
    case ClickIntent::SelectInMarquee:
        PerformSelection(EndScreen, EndGround, bWasDrag);
        break;

    case ClickIntent::ClearSelection:
        Selection.Clear();
        break;

    case ClickIntent::CancelArmedMode:
        CancelPendingAction();
        break;

    case ClickIntent::None:
        break;
    }
}

void ARA4PlayerController::PerformSelection(const FVector2D& EndScreen, const Vec2& EndGround, bool bWasDrag)
{
    const URA4SimWorldSubsystem* Subsystem = GetSimSubsystem();
    const SimWorld* World = Subsystem != nullptr ? Subsystem->GetSimWorld() : nullptr;
    if (World == nullptr)
    {
        return;
    }

    const SelectionMode Mode =
        ResolveSelectionMode(Scheme, IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift),
                             IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl));

    TArray<PickCandidate> Candidates;
    BuildPickCandidates(Candidates);
    const std::vector<PickCandidate> AsVector(Candidates.GetData(), Candidates.GetData() + Candidates.Num());

    if (bWasDrag)
    {
        // Deproject all four screen corners onto the ground plane rather than
        // building an axis-aligned world rectangle: with a tilted or rotated camera
        // the marquee is a trapezoid, and a rectangle would select the wrong units.
        const FVector2D Corners[4] = {MarqueeStartScreen,
                                      FVector2D(EndScreen.X, MarqueeStartScreen.Y),
                                      EndScreen,
                                      FVector2D(MarqueeStartScreen.X, EndScreen.Y)};
        Vec2 Quad[4];
        bool bAllProjected = true;
        for (int32 Index = 0; Index < 4; ++Index)
        {
            if (!ScreenToGround(Corners[Index], Quad[Index]))
            {
                bAllProjected = false;
                break;
            }
        }
        if (bAllProjected)
        {
            Selection.SelectInMarquee(*World, PickInQuad(AsVector, Quad), Mode);
            return;
        }
    }

    // Plain click. A second click on the same entity within the double-click window
    // widens the selection to every visible unit of that type.
    const std::vector<EntityId> Hits = PickAtPoint(AsVector, EndGround);
    const EntityId Clicked = Hits.empty() ? EntityId::Invalid() : Hits.front();
    const double Now = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.0;

    if (Clicked.IsValid() && Clicked == LastClickedEntity && (Now - LastPrimaryClickTime) <= DoubleClickSeconds)
    {
        std::vector<EntityId> Visible;
        Visible.reserve(AsVector.size());
        for (const PickCandidate& C : AsVector)
        {
            Visible.push_back(C.Id);
        }
        Selection.SelectSameType(*World, Clicked, Visible, Mode);
        LastPrimaryClickTime = -1.0;
    }
    else
    {
        Selection.SelectAtCursor(*World, Hits, Mode);
        LastPrimaryClickTime = Now;
    }
    LastClickedEntity = Clicked;
}

void ARA4PlayerController::OnSecondaryPressed()
{
    float MouseX = 0.0f;
    float MouseY = 0.0f;
    if (!GetMousePosition(MouseX, MouseY))
    {
        return;
    }
    // A right click is never a drag: under the classic scheme it deselects, and there
    // is nothing to rubber-band.
    HandleClick(/*bLeftButton*/ false, FVector2D(MouseX, MouseY), /*bWasDrag*/ false);
}

void ARA4PlayerController::OnMiddlePressed()
{
    float MouseX = 0.0f;
    float MouseY = 0.0f;
    if (ARA4CameraPawn* CameraPawn = GetCameraPawn())
    {
        if (GetMousePosition(MouseX, MouseY))
        {
            CameraPawn->GetCameraController().BeginMiddleDrag(MouseX, MouseY);
        }
    }
}

void ARA4PlayerController::OnMiddleReleased()
{
    if (ARA4CameraPawn* CameraPawn = GetCameraPawn())
    {
        CameraPawn->GetCameraController().EndMiddleDrag();
    }
}

void ARA4PlayerController::OnZoomIn()
{
    if (ARA4CameraPawn* CameraPawn = GetCameraPawn())
    {
        CameraPawn->GetCameraController().AddZoomNotches(1.0f);
    }
}

void ARA4PlayerController::OnZoomOut()
{
    if (ARA4CameraPawn* CameraPawn = GetCameraPawn())
    {
        CameraPawn->GetCameraController().AddZoomNotches(-1.0f);
    }
}

void ARA4PlayerController::OnStopPressed()
{
    const URA4SimWorldSubsystem* Subsystem = GetSimSubsystem();
    const SimWorld* World = Subsystem != nullptr ? Subsystem->GetSimWorld() : nullptr;
    if (World == nullptr)
    {
        return;
    }

    std::vector<Command> Commands;
    for (const EntityId& Id : Selection.Get())
    {
        const EntityCore* Core = World->GetCore(Id);
        if (Core == nullptr || Core->Owner != Selection.GetLocalPlayer() || Core->Kind != EntityKind::Unit)
        {
            continue;
        }
        Command C;
        C.Type = CommandType::Stop;
        C.Issuer = Selection.GetLocalPlayer();
        C.Primary = Id;
        Commands.push_back(C);
    }
    SubmitOrders(Commands);
}

void ARA4PlayerController::OnGuardPressed()
{
    const URA4SimWorldSubsystem* Subsystem = GetSimSubsystem();
    const SimWorld* World = Subsystem != nullptr ? Subsystem->GetSimWorld() : nullptr;
    if (World == nullptr)
    {
        return;
    }

    std::vector<Command> Commands;
    for (const EntityId& Id : Selection.Get())
    {
        const EntityCore* Core = World->GetCore(Id);
        if (Core == nullptr || Core->Owner != Selection.GetLocalPlayer() || Core->Kind != EntityKind::Unit)
        {
            continue;
        }
        Command C;
        C.Type = CommandType::Guard;
        C.Issuer = Selection.GetLocalPlayer();
        C.Primary = Id;
        Commands.push_back(C);
    }
    SubmitOrders(Commands);
}

// The cursor is the only feedback the player gets before committing a click, so it
// has to be driven by the same hint the order resolver produces -- never by a guess
// about what is under the pointer.
void ARA4PlayerController::ApplyCursorShape()
{
    EMouseCursor::Type Shape = EMouseCursor::Default;
    switch (CurrentCursorHint)
    {
    case CursorHint::Move:
        Shape = EMouseCursor::CardinalCross;
        break;
    case CursorHint::Attack:
    case CursorHint::ForceAttack:
        Shape = EMouseCursor::Crosshairs;
        break;
    case CursorHint::NoEntry:
        Shape = EMouseCursor::SlashedCircle;
        break;
    case CursorHint::Harvest:
    case CursorHint::Deliver:
        Shape = EMouseCursor::GrabHand;
        break;
    case CursorHint::Repair:
    case CursorHint::Capture:
    case CursorHint::SetRallyPoint:
        Shape = EMouseCursor::Hand;
        break;
    case CursorHint::Select:
    case CursorHint::None:
    default:
        Shape = EMouseCursor::Default;
        break;
    }
    CurrentMouseCursor = Shape;
}

void ARA4PlayerController::OnControlGroupKeyByKey(const FKey Key)
{
    static const FKey GroupKeys[10] = {EKeys::One, EKeys::Two,   EKeys::Three, EKeys::Four, EKeys::Five,
                                       EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine, EKeys::Zero};
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(GroupKeys); ++Index)
    {
        if (GroupKeys[Index] == Key)
        {
            OnControlGroupKey(Index);
            return;
        }
    }
}

void ARA4PlayerController::OnControlGroupKey(int32 GroupIndex)
{
    const URA4SimWorldSubsystem* Subsystem = GetSimSubsystem();
    const SimWorld* World = Subsystem != nullptr ? Subsystem->GetSimWorld() : nullptr;
    if (World == nullptr)
    {
        return;
    }

    const bool bCtrl = IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl);
    const bool bShift = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);

    if (bCtrl)
    {
        Selection.AssignControlGroup(GroupIndex);
        return;
    }
    if (bShift)
    {
        Selection.AddToControlGroup(GroupIndex);
        return;
    }

    if (Selection.RecallControlGroup(GroupIndex, *World))
    {
        // Snap the camera to the group so the key is also a "show me" gesture.
        const EntityId Primary = Selection.GetPrimary();
        if (const TransformComp* Transform = World->GetTransform(Primary))
        {
            if (ARA4CameraPawn* CameraPawn = GetCameraPawn())
            {
                CameraPawn->GetCameraController().FocusOn(
                    RA4::Input::Vec2f(float(Transform->Position.X.ToDoubleUnsafe()),
                                      float(Transform->Position.Y.ToDoubleUnsafe())),
                    false);
            }
        }
    }
}

void ARA4PlayerController::BeginPlacement(int64 ContentIdValue)
{
    PlacementContent = ContentId(uint32(ContentIdValue));
    bPlacementArmed = PlacementContent.IsValid();
    bAttackMoveArmed = false;
}

void ARA4PlayerController::CancelPendingAction()
{
    bAttackMoveArmed = false;
    bPlacementArmed = false;
    PlacementContent = ContentId();
}

// ---------------------------------------------------------------------------
// Plumbing
// ---------------------------------------------------------------------------

void ARA4PlayerController::SubmitOrders(const std::vector<Command>& Commands)
{
    URA4SimWorldSubsystem* Subsystem = GetSimSubsystem();
    if (Subsystem == nullptr)
    {
        return;
    }
    // Commands go into the subsystem's queue, not straight into the simulation.
    // That queue is where the network layer will later serialise and send them,
    // so single player and multiplayer take the identical path.
    for (const Command& C : Commands)
    {
        Subsystem->EnqueueCommand(C);
    }
}

URA4SimWorldSubsystem* ARA4PlayerController::GetSimSubsystem() const
{
    UWorld* World = GetWorld();
    return World != nullptr ? World->GetSubsystem<URA4SimWorldSubsystem>() : nullptr;
}

ARA4CameraPawn* ARA4PlayerController::GetCameraPawn() const
{
    return Cast<ARA4CameraPawn>(GetPawn());
}
