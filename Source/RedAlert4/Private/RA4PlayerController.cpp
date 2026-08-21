// Copyright (c) Red Alert 4 project.
#include "RA4PlayerController.h"

#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerInput.h"
#include "RA4CameraPawn.h"
#include "RA4SimCoords.h"
#include "RA4SimWorldSubsystem.h"
#include "RA4HUDWidget.h"
#include "RA4HUD.h"
#include "RA4MatchResultOverlayWidget.h"
#include "RA4PauseMenuWidget.h"
#include "RA4AudioSubsystem.h"
#include "RA4HoverTooltipWidget.h"
#include "RA4SidebarWidget.h"
#include "RA4CheatConsoleWidget.h"
#include "RA4DirectControlSubsystem.h"
#include "RA4UIDataProviderSubsystem.h"
#include "RA4DirectControlHUDViewModel.h"
#include "RA4DirectControlProfile.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Layout/WidgetPath.h"
#include "Misc/PackageName.h"
#include "UnrealClient.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "EngineUtils.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"

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
        // Build the complete viewport slots before adding the widgets. In UE 5.6 the
        // UUserWidget convenience setters each rewrite part of the slot, and the size
        // helper also resets its anchors to top-left. Supplying one complete slot is
        // both simpler and immune to setter ordering.
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 8
        UGameViewportSubsystem* ViewportSubsystem = UGameViewportSubsystem::Get();
#else
        UGameViewportSubsystem* ViewportSubsystem = UGameViewportSubsystem::Get(GetWorld());
#endif

        ResourceBar = CreateWidget<URA4ResourceBarWidget>(this, URA4ResourceBarWidget::StaticClass());
        if (ResourceBar != nullptr)
        {
            FGameViewportWidgetSlot Slot;
            Slot.Anchors = FAnchors(0.0f, 0.0f);
            Slot.Offsets = FMargin(16.0f, 16.0f, 1400.0f, 46.0f);
            Slot.Alignment = FVector2D::ZeroVector;
            Slot.ZOrder = 10;
            const bool bAdded = ViewportSubsystem != nullptr &&
                                ViewportSubsystem->AddWidget(ResourceBar, Slot);
            if (bAdded)
            {
                UE_LOG(LogTemp, Display, TEXT("RA4 HUD: resource bar added to viewport"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("RA4 HUD: resource bar viewport add failed"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("RA4 HUD: failed to create the resource bar"));
        }

        Sidebar = CreateWidget<URA4SidebarWidget>(this, URA4SidebarWidget::StaticClass());
        if (Sidebar != nullptr)
        {
            Sidebar->OnBuildCardClicked.AddUObject(this, &ARA4PlayerController::HandleBuildCardClicked);
            Sidebar->OnRadarClicked.AddUObject(this, &ARA4PlayerController::HandleRadarClicked);
            Sidebar->OnRadarOrdered.AddUObject(this, &ARA4PlayerController::HandleRadarOrdered);
            // The reserved strip and the widget's own width come from the same helper. If
            // they ever disagree the world is drawn under the column, or a band of
            // background shows beside it.
            const float ReservedWidth = URA4SidebarWidget::ComputeSidebarWidth(this);
            AppliedSidebarReservedWidth = ReservedWidth;
            FGameViewportWidgetSlot Slot;
            Slot.Anchors = FAnchors(1.0f, 0.0f, 1.0f, 1.0f);
            Slot.Offsets = FMargin(0.0f, 0.0f, ReservedWidth, 0.0f);
            Slot.Alignment = FVector2D(1.0f, 0.0f);
            Slot.ZOrder = 10;
            const bool bAdded = ViewportSubsystem != nullptr &&
                                ViewportSubsystem->AddWidget(Sidebar, Slot);
            UE_LOG(LogTemp, Display,
                   TEXT("RA4 HUD: production sidebar viewport add=%d width=%.0f"),
                   bAdded ? 1 : 0, ReservedWidth);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("RA4 HUD: failed to create the sidebar"));
        }

        NotificationFeed = CreateWidget<URA4NotificationFeedWidget>(
            this, URA4NotificationFeedWidget::StaticClass());
        if (NotificationFeed != nullptr)
        {
            // SC-20 places the EVA feed top-left under the resource bar, clear of
            // the sidebar column on the right.
            FGameViewportWidgetSlot Slot;
            Slot.Anchors = FAnchors(0.0f, 0.0f);
            Slot.Offsets = FMargin(16.0f, 78.0f, 420.0f, 220.0f);
            Slot.Alignment = FVector2D::ZeroVector;
            Slot.ZOrder = 11;
            const bool bAdded = ViewportSubsystem != nullptr &&
                                ViewportSubsystem->AddWidget(NotificationFeed, Slot);
            UE_LOG(LogTemp, Display, TEXT("RA4 HUD: notification feed viewport add=%d"), bAdded ? 1 : 0);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("RA4 HUD: failed to create the notification feed"));
        }

        HoverTooltip = CreateWidget<URA4HoverTooltipWidget>(this, URA4HoverTooltipWidget::StaticClass());
        if (HoverTooltip != nullptr)
        {
            HoverTooltip->AddToViewport(/*ZOrder*/ 20);   // above the HUD panels
            HoverTooltip->SetAlignmentInViewport(FVector2D(0.0f, 0.0f));
            HoverTooltip->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
    
    FInputModeGameAndUI InputMode;
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    InputMode.SetHideCursorDuringCapture(false);
    SetInputMode(InputMode);

    if (IsLocalController() && FParse::Param(FCommandLine::Get(), TEXT("RA4CaptureUI")))
    {
        // Long enough for the match to spawn its starting base and the HUD to fill
        // with real data; the delay mirrors the showcase capture flow.
        GetWorldTimerManager().SetTimer(
            HudCaptureTimer, this, &ARA4PlayerController::CaptureHudForQA, 8.0f, false);
    }

    BindMatchResultEvents();

    // Only in an actual match. The menu world has no simulation subsystem, and
    // starting battle music over the main menu is not what the track is for.
    if (IsLocalController() && GetSimSubsystem() != nullptr)
    {
        if (UWorld* UnrealWorld = GetWorld())
        {
            if (URA4AudioSubsystem* Audio = UnrealWorld->GetSubsystem<URA4AudioSubsystem>())
            {
                Audio->StartMusic();
            }
        }
    }

    TryInitializeCamera();
}

void ARA4PlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (URA4UIDataProviderSubsystem* Provider = GetWorld() != nullptr
                                                    ? GetWorld()->GetSubsystem<URA4UIDataProviderSubsystem>()
                                                    : nullptr)
    {
        Provider->OnMatchEnded.Remove(MatchEndedHandle);
    }
    MatchEndedHandle.Reset();
    MatchResultOverlay = nullptr;

    Super::EndPlay(EndPlayReason);
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


#if !UE_BUILD_SHIPPING
void ARA4PlayerController::DebugForceVictory()
{
    Command Surrender;
    Surrender.Type = CommandType::Surrender;
    Surrender.Issuer = 1;
    SubmitOrders({Surrender});
}

void ARA4PlayerController::DebugForceDefeat()
{
    Command Surrender;
    Surrender.Type = CommandType::Surrender;
    Surrender.Issuer = Selection.GetLocalPlayer();
    SubmitOrders({Surrender});
}
#endif

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

    // Mouse buttons stay literal. Which button selects and which orders is
    // ControlScheme's decision - the pair has to swap together or the scheme becomes
    // incoherent - so they are deliberately absent from the rebindable action table.
    InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ARA4PlayerController::OnPrimaryPressed);
    InputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &ARA4PlayerController::OnPrimaryReleased);
    InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &ARA4PlayerController::OnSecondaryPressed);
    InputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &ARA4PlayerController::OnSecondaryReleased);
    InputComponent->BindKey(EKeys::MiddleMouseButton, IE_Pressed, this, &ARA4PlayerController::OnMiddlePressed);
    InputComponent->BindKey(EKeys::MiddleMouseButton, IE_Released, this, &ARA4PlayerController::OnMiddleReleased);

    // Everything else comes from the binding table, so no keyboard key is named here.
    // RA4Input was already linked and unit tested but never actually consulted: the
    // control scheme was a wall of literal BindKey calls, which made every key a
    // recompile and remapping impossible. This is that connection.
    KeyBindings.LoadDefaults(Scheme);

    // A collision means two actions answer the same chord, and which one is the
    // mistake is the player's call, not ours - so report it and carry on rather than
    // silently dropping a binding the player asked for.
    for (const RA4::Input::GameAction Conflicted : KeyBindings.FindConflicts())
    {
        UE_LOG(LogTemp, Warning, TEXT("RA4 input: binding conflict on action '%s'"),
               *FString(RA4::Input::ToString(Conflicted)));
    }

    // Bind exactly the physical keys the table names, and nothing else.
    for (const std::string& KeyName : KeyBindings.DistinctKeys())
    {
        const FKey Key(*FString(KeyName.c_str()));
        if (!Key.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("RA4 input: binding names unknown key '%s'"),
                   *FString(KeyName.c_str()));
            continue;
        }
        InputComponent->BindKey(Key, IE_Pressed, this, &ARA4PlayerController::OnBoundKeyPressed);

        // Held actions are polled in UpdateCameraInput, but PlayerInput only tracks a
        // key it has a binding for, so the release edge has to be claimed too or
        // IsInputKeyDown never reports the key at all.
        if (RA4::Input::IsHeldAction(
                KeyBindings.Resolve(KeyName, /*bCtrl*/ false, /*bShift*/ false, /*bAlt*/ false)))
        {
            InputComponent->BindKey(Key, IE_Released, this, &ARA4PlayerController::OnDummyPanKey);
        }
    }

    // Build card hotkeys, in the same grid order as the badges the sidebar draws.
    for (const FKey& Key : GetBuildCardHotkeys())
    {
        InputComponent->BindKey(Key, IE_Pressed, this, &ARA4PlayerController::OnBuildCardKeyByKey);
    }

    // Direct tactical hotkeys:
    // 'H' -> Snap/focus camera on Home Base / MCV / Construction Yard
    InputComponent->BindKey(EKeys::H, IE_Pressed, this, &ARA4PlayerController::JumpToHomeBase);
    // 'J' -> Toggle First-Person / 3rd-Person Direct Control on selected unit
    InputComponent->BindKey(EKeys::J, IE_Pressed, this, &ARA4PlayerController::ToggleDirectControl);
    // 'D' -> Deploy selected MCV (or player's MCV) into Construction Yard
    InputComponent->BindKey(EKeys::D, IE_Pressed, this, &ARA4PlayerController::DeploySelectedMcv);
}

void ARA4PlayerController::OnBoundKeyPressed(const FKey Key)
{
    // Modifiers are read here rather than baked into separate enumerators: Resolve
    // prefers an exact modifier match, so Ctrl+1 and 1 can both reach ControlGroup1
    // and the assign-versus-recall decision stays where the group logic already is.
    const bool bCtrl = IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl);
    const bool bShift = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);
    const bool bAlt = IsInputKeyDown(EKeys::LeftAlt) || IsInputKeyDown(EKeys::RightAlt);

    const FString KeyName = Key.ToString();
    const RA4::Input::GameAction Action =
        KeyBindings.Resolve(TCHAR_TO_UTF8(*KeyName), bCtrl, bShift, bAlt);

    using RA4::Input::GameAction;
    switch (Action)
    {
    case GameAction::AttackMove:
        OnAttackMovePressed();
        return;
    case GameAction::Stop:
        OnStopPressed();
        return;
    case GameAction::Guard:
    case GameAction::HoldPosition:
        OnGuardPressed();
        return;
    case GameAction::CancelAction:
        CancelPendingAction();
        return;
    case GameAction::ToggleDirectControl:
        OnDirectControlTogglePressed();
        return;
    case GameAction::ToggleCheatConsole:
        ToggleCheatConsole();
        return;
    case GameAction::CameraZoomIn:
        OnZoomIn();
        return;
    case GameAction::CameraZoomOut:
        OnZoomOut();
        return;

    case GameAction::ControlGroup1:
    case GameAction::ControlGroup2:
    case GameAction::ControlGroup3:
    case GameAction::ControlGroup4:
    case GameAction::ControlGroup5:
    case GameAction::ControlGroup6:
    case GameAction::ControlGroup7:
    case GameAction::ControlGroup8:
    case GameAction::ControlGroup9:
        OnControlGroupKey(static_cast<int32>(Action) - static_cast<int32>(GameAction::ControlGroup1));
        return;
    case GameAction::ControlGroup0:
        // Group 0 sits after 9 on the keyboard but is the tenth slot, matching the
        // on-screen numbering rather than the enumerator order.
        OnControlGroupKey(9);
        return;

    default:
        // Held actions (panning, rotate, fast-pan) are polled in UpdateCameraInput,
        // and an unbound key resolves to None. Both are no-ops on the press edge.
        return;
    }
}

const TArray<FKey>& ARA4PlayerController::GetBuildCardHotkeys()
{
    // Deliberately not the digits: those are control groups above, and the ordinary RTS
    // reflex of pressing a number to recall a squad has to keep working. Order matches
    // URA4SidebarWidget's badge table, and the assert catches a key added to one table
    // and not the other rather than leaving a badge that does nothing.
    //
    // H is absent because RA4::Input::KeyBindingTable binds it to HoldPosition; while
    // both tables claimed it, one press both held position and queued a structure.
    static const TArray<FKey> Keys = {
        EKeys::Q, EKeys::E, EKeys::R, EKeys::T,
        EKeys::Y, EKeys::U, EKeys::I, EKeys::O,
        EKeys::P, EKeys::L, EKeys::J, EKeys::K,
    };
    checkf(Keys.Num() == URA4SidebarWidget::GetCardHotkeyCount(),
           TEXT("Build card hotkey table and sidebar badge table disagree (%d vs %d)"),
           Keys.Num(), URA4SidebarWidget::GetCardHotkeyCount());
    return Keys;
}

void ARA4PlayerController::OnBuildCardKeyByKey(const FKey Key)
{
    const TArray<FKey>& Keys = GetBuildCardHotkeys();
    for (int32 Index = 0; Index < Keys.Num(); ++Index)
    {
        if (Keys[Index] == Key)
        {
            OnBuildCardHotkey(Index);
            return;
        }
    }
}

void ARA4PlayerController::OnBuildCardHotkey(int32 CardIndex)
{
    // The console is eating letter keys while it is open, and a possessed unit is being
    // driven with them. Neither should be queueing structures.
    if (bCheatConsoleOpen || IsDirectControlActive() || Sidebar == nullptr)
    {
        return;
    }

    // Delegated rather than resolved here: which content a grid position maps to, and
    // whether that card is buildable at all, is the sidebar's business. This only turns a
    // key into an index, which is what an adapter is for.
    Sidebar->ActivateCardByIndex(CardIndex);
}

void ARA4PlayerController::OnDummyPanKey()
{
}

// ---------------------------------------------------------------------------
// Frame update
// ---------------------------------------------------------------------------

void ARA4PlayerController::SyncSidebarReservedWidth()
{
    if (Sidebar == nullptr)
    {
        return;
    }

    // The width the sidebar will actually draw itself at. Both sides deliberately go
    // through the same helper; the problem this function solves is not disagreement about
    // the formula but disagreement in *time*. BeginPlay runs before the viewport reports
    // a size, so the first evaluation returns the documented 1.0 fallback. The widget
    // corrects itself on its own tick, and without this the reserved strip would stay at
    // the fallback width forever -- leaving a band of world visible beside the column at
    // any viewport height other than the 1080p reference.
    const float DesiredWidth = URA4SidebarWidget::ComputeSidebarWidth(this);

    // Half a unit of slack: the width is a float derived from a viewport ratio, and
    // rewriting the slot every frame over rounding noise would invalidate Slate's layout
    // for no visible change.
    if (FMath::IsNearlyEqual(DesiredWidth, AppliedSidebarReservedWidth, 0.5f))
    {
        return;
    }

#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 8
    UGameViewportSubsystem* ViewportSubsystem = UGameViewportSubsystem::Get();
#else
    UGameViewportSubsystem* ViewportSubsystem = UGameViewportSubsystem::Get(GetWorld());
#endif
    if (ViewportSubsystem == nullptr)
    {
        return;
    }

    // Read-modify-write rather than rebuilding the slot: anchors, alignment and ZOrder
    // were set once in BeginPlay and are none of this function's business.
    FGameViewportWidgetSlot Slot = ViewportSubsystem->GetWidgetSlot(Sidebar);
    Slot.Offsets = FMargin(0.0f, 0.0f, DesiredWidth, 0.0f);
    ViewportSubsystem->SetWidgetSlot(Sidebar, Slot);

    AppliedSidebarReservedWidth = DesiredWidth;

    // Logged because the width that matters is not the one BeginPlay computed. That
    // first value is necessarily the pre-viewport fallback, so a log line showing only
    // it cannot tell anyone whether this correction happened at all.
    UE_LOG(LogTemp, Display, TEXT("RA4 HUD: sidebar reserved width synced to %.0f"), DesiredWidth);
}

void ARA4PlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);

    if (!bMatchResultEventsBound)
    {
        BindMatchResultEvents();
    }

    if (!bCameraInitialized)
    {
        TryInitializeCamera();
    }

    SyncSidebarReservedWidth();

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

    URA4SimWorldSubsystem* Subsystem = GetSimSubsystem();
    const SimWorld* World = Subsystem != nullptr ? Subsystem->GetSimWorld() : nullptr;
    if (World == nullptr)
    {
        return;
    }

    // A unit that died this tick has to leave the selection immediately, or the
    // next order references a stale handle and the server rejects it.
    Selection.PruneDead(*World);

    // The HUD reads the selection out of the snapshot, not out of this controller,
    // so it has to be handed over every frame. Doing it here rather than at each
    // call site covers every way the selection can change -- click, marquee,
    // control group, and the prune immediately above -- and costs one vector copy
    // of a list that is bounded by what a player can select.
    Subsystem->SetSelectedEntitiesForHUD(Selection.Get());

    // Cheap, and it has to run every frame: the outline follows the camera, which moves
    // continuously. SetRadarCameraView drops an identical frame without invalidating Slate.
    UpdateRadarCameraView();

    // Direct control is presentation-driven: the subsystem owns the phase
    // machine and the camera. We branch here so RTS camera/selection input
    // is completely suppressed while the player is inside a vehicle.
    if (URA4DirectControlSubsystem* Dc = GetDirectControlSubsystem())
    {
        if (Dc->IsInDirectControl() ||
            Dc->GetClientPhase() == ERA4DirectControlClientPhase::Entering ||
            Dc->GetClientPhase() == ERA4DirectControlClientPhase::Exiting)
        {
            UpdateDirectControl(DeltaTime);
            return;
        }
    }

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

        UpdateHoverTooltip(*World);
        UpdateGhostPlacement();
    }
}

void ARA4PlayerController::UpdateHoverTooltip(const SimWorld& World)
{
    if (!IsLocalController() || HoverTooltip == nullptr)
    {
        return;
    }

    float MouseX = 0.0f;
    float MouseY = 0.0f;
    Vec2 Ground;
    const bool bHaveCursor = GetMousePosition(MouseX, MouseY) && GetCursorGroundPosition(Ground);

    const EntityId Under = bHaveCursor ? FindHoveredEntity(Ground) : EntityId::Invalid();
    const double Now = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.0;

    // Moving to a different thing restarts the dwell timer, so sweeping the cursor
    // across a crowded base does not flash a card per unit passed over.
    if (Under != HoveredEntity)
    {
        HoveredEntity = Under;
        HoverStartedSeconds = Now;
        if (bTooltipVisible)
        {
            HoverTooltip->SetVisibility(ESlateVisibility::Collapsed);
            bTooltipVisible = false;
        }
    }

    const EntityCore* Core = Under.IsValid() ? World.GetCore(Under) : nullptr;
    if (Core == nullptr || !Core->bAlive || World.GetContent() == nullptr ||
        Now - HoverStartedSeconds < double(TooltipDelaySeconds))
    {
        return;
    }

    const EntityDef* Def = World.GetContent()->FindEntity(Core->Def);
    if (Def == nullptr)
    {
        return;
    }

    // DisplayNameKey is a localisation key; falling back to the stable content name
    // keeps something readable on screen for entries that have not been localised.
    FText Title = FText::FromString(FString(Def->Name.c_str()));
    if (!Def->DisplayNameKey.empty())
    {
        const FString Key(Def->DisplayNameKey.c_str());
        if (const URA4UIDataProviderSubsystem* Provider =
                GetWorld()->GetSubsystem<URA4UIDataProviderSubsystem>())
        {
            Title = Provider->GetDisplayNameForKey(Key);
        }
    }

    // Second line: health for anything damaged, plus whose it is, because "what is
    // that and is it mine" is the actual question a hover answers.
    FText Subtitle = FText::GetEmpty();
    const HealthComp* Health = World.GetHealth(Under);
    const bool bMine = Core->Owner == Selection.GetLocalPlayer();
    const FText Ownership = bMine ? NSLOCTEXT("RA4", "Tooltip_Own", "Allied")
                                  : NSLOCTEXT("RA4", "Tooltip_Enemy", "Enemy");
    if (Health != nullptr && Health->Max > 0)
    {
        Subtitle = FText::Format(NSLOCTEXT("RA4", "Tooltip_HealthLine", "{0}  —  {1} / {2}"), Ownership,
                                 FText::AsNumber(Health->Current), FText::AsNumber(Health->Max));
    }
    else
    {
        Subtitle = Ownership;
    }

    HoverTooltip->SetContent(Title, Subtitle);
    const FVector2D NewPos(MouseX + 18.0f, MouseY + 18.0f);
    if (!bTooltipVisible || !HoverTooltipPosition.Equals(NewPos, 0.5f))
    {
        HoverTooltipPosition = NewPos;
        HoverTooltip->SetPositionInViewport(NewPos, /*bRemoveDPIScale*/ false);
    }
    if (!bTooltipVisible)
    {
        HoverTooltip->SetVisibility(ESlateVisibility::HitTestInvisible);
        bTooltipVisible = true;
    }
}

void ARA4PlayerController::BindMatchResultEvents()
{
    if (!IsLocalController() || bMatchResultEventsBound || GetWorld() == nullptr)
    {
        return;
    }

    if (URA4UIDataProviderSubsystem* Provider = GetWorld()->GetSubsystem<URA4UIDataProviderSubsystem>())
    {
        MatchEndedHandle = Provider->OnMatchEnded.AddUObject(this, &ARA4PlayerController::HandleMatchEnded);
        bMatchResultEventsBound = true;

        if (Provider->GetMatchPhase() == ERA4MatchPhase::Finished)
        {
            constexpr int32 kLocalPlayer = 0;
            const bool bWon = Provider->GetWinningPlayer() == kLocalPlayer && !Provider->IsLocalPlayerDefeated();
            HandleMatchEnded(bWon, Provider->GetWinningPlayer());
        }
    }
}

void ARA4PlayerController::HandleMatchEnded(bool bLocalPlayerWon, int32 WinningPlayer)
{
    if (!IsLocalController() || bMatchResultVisible)
    {
        return;
    }

    if (MatchResultOverlay == nullptr)
    {
        MatchResultOverlay = CreateWidget<URA4MatchResultOverlayWidget>(
            this, URA4MatchResultOverlayWidget::StaticClass());
        if (MatchResultOverlay == nullptr)
        {
            UE_LOG(LogTemp, Error, TEXT("RA4 HUD: failed to create the match result overlay"));
            return;
        }

        MatchResultOverlay->OnRetryRequested.AddUObject(this, &ARA4PlayerController::HandleRetryRequested);
        MatchResultOverlay->OnExitRequested.AddUObject(this, &ARA4PlayerController::HandleExitRequested);
    }

    MatchResultOverlay->Configure(bLocalPlayerWon, HasMainMenuMap());
    MatchResultOverlay->AddToViewport(/*ZOrder*/ 100);
    bMatchResultVisible = true;

    FInputModeUIOnly InputMode;
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(InputMode);
    bShowMouseCursor = true;

    UE_LOG(LogTemp, Display, TEXT("RA4 HUD: match result overlay shown (won=%s, winner=%d)"),
           bLocalPlayerWon ? TEXT("true") : TEXT("false"), WinningPlayer);
}

void ARA4PlayerController::HandleRetryRequested()
{
    if (UWorld* World = GetWorld())
    {
        const FString MapPackageName = World->GetOutermost()->GetName();
        UE_LOG(LogTemp, Display, TEXT("RA4 HUD: restarting level %s"), *MapPackageName);
        UGameplayStatics::OpenLevel(this, FName(*MapPackageName));
    }
}

void ARA4PlayerController::HandleExitRequested()
{
    static const FString MainMenuMap = TEXT("/Game/Maps/RA4_MainMenu");
    UE_LOG(LogTemp, Display, TEXT("RA4 HUD: leaving match for %s"), *MainMenuMap);
    if (FPackageName::DoesPackageExist(MainMenuMap))
    {
        UGameplayStatics::OpenLevel(
            this,
            FName(*MainMenuMap),
            true,
            TEXT("game=/Script/RedAlert4.RA4UIShowcaseGameMode"));
    }
    else
    {
        UGameplayStatics::OpenLevel(
            this,
            TEXT("/Game/Maps/Showcase_UI"),
            true,
            TEXT("game=/Script/RedAlert4.RA4UIShowcaseGameMode"));
    }
}

bool ARA4PlayerController::HasMainMenuMap() const
{
    static const FString MainMenuMap = TEXT("/Game/Maps/RA4_MainMenu");
    static const FString FallbackMap = TEXT("/Game/Maps/Showcase_UI");
    return FPackageName::DoesPackageExist(MainMenuMap) || FPackageName::DoesPackageExist(FallbackMap);
}

void ARA4PlayerController::UpdateCameraInput(float DeltaTime)
{
    ARA4CameraPawn* CameraPawn = GetCameraPawn();
    if (CameraPawn == nullptr)
    {
        return;
    }
    CameraController& Camera = CameraPawn->GetCameraController();

    // WASD + Arrow keys:
    // W = Forward on screen (+Up)
    // S = Backward on screen (-Up)
    // A = Left on screen (-Right)
    // D = Right on screen (+Right)
    const float Right = (IsInputKeyDown(EKeys::D) || IsInputKeyDown(EKeys::Right) ? 1.0f : 0.0f) -
                        (IsInputKeyDown(EKeys::A) || IsInputKeyDown(EKeys::Left) ? 1.0f : 0.0f);
    const float Up = (IsInputKeyDown(EKeys::W) || IsInputKeyDown(EKeys::Up) ? 1.0f : 0.0f) -
                     (IsInputKeyDown(EKeys::S) || IsInputKeyDown(EKeys::Down) ? 1.0f : 0.0f);

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

    // Keyboard rotation controls:
    // Insert / Delete, Comma / Period, Alt + A / D or Alt + Left / Right
    float KeyYawDelta = 0.0f;
    float KeyPitchDelta = 0.0f;
    const bool bAltHeld = IsInputKeyDown(EKeys::LeftAlt) || IsInputKeyDown(EKeys::RightAlt);
    const bool bCtrlHeld = IsInputKeyDown(EKeys::LeftControl) || IsInputKeyDown(EKeys::RightControl);
    const float DtVal = DeltaTime > 0.0f ? DeltaTime : 0.016f;

    if (IsInputKeyDown(EKeys::Insert) || IsInputKeyDown(EKeys::Comma) || (bAltHeld && (IsInputKeyDown(EKeys::A) || IsInputKeyDown(EKeys::Left))))
    {
        KeyYawDelta -= 120.0f * DtVal;
    }
    if (IsInputKeyDown(EKeys::Delete) || IsInputKeyDown(EKeys::Period) || (bAltHeld && (IsInputKeyDown(EKeys::D) || IsInputKeyDown(EKeys::Right))))
    {
        KeyYawDelta += 120.0f * DtVal;
    }
    if (IsInputKeyDown(EKeys::PageUp) || (bAltHeld && (IsInputKeyDown(EKeys::W) || IsInputKeyDown(EKeys::Up))))
    {
        KeyPitchDelta += 60.0f * DtVal;
    }
    if (IsInputKeyDown(EKeys::PageDown) || (bAltHeld && (IsInputKeyDown(EKeys::S) || IsInputKeyDown(EKeys::Down))))
    {
        KeyPitchDelta -= 60.0f * DtVal;
    }
    if (IsInputKeyDown(EKeys::Home) || IsInputKeyDown(EKeys::BackSpace))
    {
        Camera.ResetRotation();
    }

    if (KeyYawDelta != 0.0f)
    {
        Camera.AddYawDegrees(KeyYawDelta);
    }
    if (KeyPitchDelta != 0.0f)
    {
        Camera.AddPitchDegrees(KeyPitchDelta);
    }

    // Mouse orbit / rotation gestures:
    // - Space + mouse movement
    // - Alt + RMB / LMB / MMB drag
    // - Ctrl + MMB drag
    const bool bRightMouseDown = IsInputKeyDown(EKeys::RightMouseButton);
    const bool bLeftMouseDown = IsInputKeyDown(EKeys::LeftMouseButton);
    const bool bMiddleMouseDown = IsInputKeyDown(EKeys::MiddleMouseButton);
    const bool bSpaceDown = IsInputKeyDown(EKeys::SpaceBar);

    const bool bRotateGestureActive = bSpaceDown ||
                                      (bAltHeld && (bRightMouseDown || bLeftMouseDown || bMiddleMouseDown)) ||
                                      (bCtrlHeld && bMiddleMouseDown);

    if (bRotateGestureActive)
    {
        if (!bRotatingCamera)
        {
            bRotatingCamera = true;
            RotateAnchorScreen = FVector2D(MouseX, MouseY);
        }
        else if (bHasCursor)
        {
            const float DeltaX = float(MouseX - RotateAnchorScreen.X);
            const float DeltaY = float(MouseY - RotateAnchorScreen.Y);
            RotateAnchorScreen = FVector2D(MouseX, MouseY);
            Camera.AddYawDegrees(DeltaX * 0.4f);
            Camera.AddPitchDegrees(-DeltaY * 0.3f);
            Camera.SetCursorPosition(MouseX, MouseY, /*bWindowFocused*/ false);
        }
        Camera.SetKeyboardPan(0.0f, 0.0f);
    }
    else
    {
        bRotatingCamera = false;
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

    FVector HitPos = FVector::ZeroVector;
    bool bHitFound = false;

    if (const UWorld* World = GetWorld())
    {
        FHitResult TraceHit;
        FCollisionQueryParams QueryParams(TEXT("RA4ClickTrace"), false);
        QueryParams.AddIgnoredActor(this);
        if (World->LineTraceSingleByChannel(TraceHit, RayOrigin, RayOrigin + RayDirection * 100000.0, ECC_WorldStatic, QueryParams))
        {
            if (TraceHit.bBlockingHit)
            {
                HitPos = TraceHit.ImpactPoint;
                bHitFound = true;
            }
        }
    }

    if (!bHitFound)
    {
        if (!RA4Coords::IntersectGroundPlane(RayOrigin, RayDirection, HitPos))
        {
            return false;
        }
    }

    OutPosition = RA4Coords::FromUnreal(HitPos);

    // Apply strict map boundary constraint (Invisible Barrier)
    const URA4SimWorldSubsystem* Subsystem = GetSimSubsystem();
    if (Subsystem != nullptr && Subsystem->GetSimWorld() != nullptr)
    {
        const RA4::MapDescription& Map = Subsystem->GetSimWorld()->GetMap();
        if (Map.Width > 0 && Map.Height > 0)
        {
            const RA4::Fixed MinMargin = RA4::Fixed::FromInt(50);
            const RA4::Fixed MaxX = RA4::Fixed::FromInt(int64(Map.Width) * RA4::MapDescription::kTileSizeUnitsLocal) - MinMargin;
            const RA4::Fixed MaxY = RA4::Fixed::FromInt(int64(Map.Height) * RA4::MapDescription::kTileSizeUnitsLocal) - MinMargin;
            OutPosition.X = RA4::Fixed::FromRaw(FMath::Clamp(OutPosition.X.Raw, MinMargin.Raw, MaxX.Raw));
            OutPosition.Y = RA4::Fixed::FromRaw(FMath::Clamp(OutPosition.Y.Raw, MinMargin.Raw, MaxY.Raw));
        }
    }

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

    const RA4::PlayerId LocalPlayer = Selection.GetLocalPlayer();

    for (uint32 Index = 0; Index < uint32(Cores.size()); ++Index)
    {
        const EntityCore& Core = Cores[Index];
        if (!Core.bAlive || Core.Kind == EntityKind::Projectile)
        {
            continue;
        }

        // V-B (VISIBILITY_CALLSITE_INVENTORY): the cursor must not find what the
        // player cannot see. Without this gate, hovering fog produced tooltips
        // and cursor changes over hidden enemies -- an intel leak through the
        // mouse. Own units are always pickable; IsEntityVisibleTo also handles
        // matches configured without fog (everything visible).
        if (!World->IsEntityVisibleTo(LocalPlayer, Index))
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

bool ARA4PlayerController::IsPointerOverUI() const
{
    float MouseX = 0.0f;
    float MouseY = 0.0f;
    if (GetMousePosition(MouseX, MouseY))
    {
        if (FSlateApplication::IsInitialized())
        {
            const FVector2D ScreenPos(MouseX, MouseY);
            FWidgetPath WidgetPath = FSlateApplication::Get().LocateWindowUnderMouse(
                ScreenPos, FSlateApplication::Get().GetInteractiveTopLevelWindows());
            if (WidgetPath.IsValid())
            {
                const TSharedRef<SWidget> LastWidget = WidgetPath.GetLastWidget();
                const FName WidgetType = LastWidget->GetType();
                if (WidgetType != FName(TEXT("SViewport")) && WidgetType != FName(TEXT("SGameLayerManager")))
                {
                    return true;
                }
            }
        }
    }

    return (Sidebar != nullptr && Sidebar->IsHovered()) ||
           (ResourceBar != nullptr && ResourceBar->IsHovered()) ||
           (MatchResultOverlay != nullptr && MatchResultOverlay->IsHovered()) ||
           (PauseMenuOverlay != nullptr && PauseMenuOverlay->IsHovered());
}

// ---------------------------------------------------------------------------
// Mouse handling
// ---------------------------------------------------------------------------

void ARA4PlayerController::OnPrimaryPressed()
{
    if (bDirectControlActive) return;
    bPrimaryConsumedByUI = IsPointerOverUI();
    if (bPrimaryConsumedByUI)
    {
        bMarqueeActive = false;
        return;
    }

    float MouseX = 0.0f;
    float MouseY = 0.0f;
    if (!GetMousePosition(MouseX, MouseY))
    {
        return;
    }

    if (IsInputKeyDown(EKeys::SpaceBar))
    {
        bRotatingCamera = true;
        RotateAnchorScreen = FVector2D(MouseX, MouseY);
        bMarqueeActive = false;
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
    if (bDirectControlActive) return;
    if (bRotatingCamera)
    {
        bRotatingCamera = false;
        bMarqueeActive = false;
        return;
    }

    if (bPrimaryConsumedByUI)
    {
        bPrimaryConsumedByUI = false;
        bMarqueeActive = false;
        return;
    }

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
    {
        const OrderContext Context = MakeOrderContext(EndGround);
        const std::vector<Command> Orders = ResolveOrder(*World, Selection, Context);
        SubmitOrders(Orders);
        // Voiced from the resolved hint rather than the button pressed, so the unit
        // acknowledges what it is actually about to do.
        if (!Orders.empty() && !bPlacementArmed)
        {
            const CursorHint Hint = ResolveCursorHint(*World, Selection, Context);
            PlayOrderVoice(*World, Hint);

            // Ping the destination once, at the moment the order is given. A marker
            // that tracked the cursor would just be clutter the rest of the time.
            if (Hint == CursorHint::Move || Hint == CursorHint::SetRallyPoint)
            {
                MoveOrderPingLocation = EndGround;
                MoveOrderPingSeconds = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.0;
                bHasMoveOrderPing = true;
            }
        }
        // An armed mode is spent by the click that used it, successful or not: the
        // alternative is a player who keeps placing buildings they thought they had
        // already placed.
        bAttackMoveArmed = false;
        bPlacementArmed = false;
        PlacementContent = ContentId();
        break;
    }

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

    PlaySelectionVoice(*World);
}

void ARA4PlayerController::PlayOrderVoice(const SimWorld& World, RA4::Input::CursorHint Hint)
{
    ERA4VoiceEvent Event = ERA4VoiceEvent::Move;
    switch (Hint)
    {
    case CursorHint::Attack:
    case CursorHint::ForceAttack:
        Event = ERA4VoiceEvent::Attack;
        break;
    case CursorHint::Harvest:
    case CursorHint::Deliver:
    case CursorHint::Repair:
    case CursorHint::Capture:
        // These are all "going to do a job somewhere", which the Ability line covers
        // better than a plain movement acknowledgement.
        Event = ERA4VoiceEvent::Ability;
        break;
    default:
        Event = ERA4VoiceEvent::Move;
        break;
    }
    PlayVoiceForPrimary(World, Event);
}

void ARA4PlayerController::PlaySelectionVoice(const SimWorld& World)
{
    PlayVoiceForPrimary(World, ERA4VoiceEvent::Selected);
}

void ARA4PlayerController::PlayVoiceForPrimary(const SimWorld& World, ERA4VoiceEvent Event)
{
    // The primary is what the HUD shows a portrait for, so it is the one that speaks.
    const EntityId Primary = Selection.GetPrimary();
    if (!Primary.IsValid())
    {
        return;
    }

    const EntityCore* Core = World.GetCore(Primary);
    if (Core == nullptr || Core->Owner != Selection.GetLocalPlayer() || World.GetContent() == nullptr)
    {
        return;   // enemy units do not answer to this player
    }

    // The voice pack is keyed by the unit's Stable ID, which the content database
    // stores alongside the unit rather than on the entity definition itself.
    const VoiceSetDef* VoiceSet = World.GetContent()->FindVoiceSet(Core->Def);
    if (VoiceSet == nullptr || VoiceSet->VoiceId.empty())
    {
        return;   // no recorded voice for this unit yet
    }

    if (UWorld* UnrealWorld = GetWorld())
    {
        if (URA4AudioSubsystem* Audio = UnrealWorld->GetSubsystem<URA4AudioSubsystem>())
        {
            Audio->PlayUnitVoice(FString(VoiceSet->VoiceId.c_str()), Event);
        }
    }
}

void ARA4PlayerController::OnSecondaryPressed()
{
    if (bDirectControlActive) return;
    if (IsPointerOverUI())
    {
        return;
    }

    float MouseX = 0.0f;
    float MouseY = 0.0f;
    if (!GetMousePosition(MouseX, MouseY))
    {
        return;
    }

    // Space held turns the right button into a camera-rotate drag instead of an
    // order/deselect. Checked on press so the button's normal meaning is untouched
    // whenever the modifier is not down.
    if (IsInputKeyDown(EKeys::SpaceBar))
    {
        bRotatingCamera = true;
        RotateAnchorScreen = FVector2D(MouseX, MouseY);
        return;
    }

    // A right click is never a drag: under the classic scheme it deselects, and there
    // is nothing to rubber-band.
    HandleClick(/*bLeftButton*/ false, FVector2D(MouseX, MouseY), /*bWasDrag*/ false);
}

void ARA4PlayerController::OnSecondaryReleased()
{
    if (bDirectControlActive) return;
    // Releasing ends the gesture without issuing the order the button would normally
    // carry -- the drag was the whole intent.
    bRotatingCamera = false;
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
    if (bDirectControlActive) return;
    if (ARA4CameraPawn* CameraPawn = GetCameraPawn())
    {
        CameraPawn->GetCameraController().AddZoomNotches(1.0f);
    }
}

void ARA4PlayerController::OnZoomOut()
{
    if (bDirectControlActive) return;
    if (ARA4CameraPawn* CameraPawn = GetCameraPawn())
    {
        CameraPawn->GetCameraController().AddZoomNotches(-1.0f);
    }
}

void ARA4PlayerController::OnStopPressed()
{
    if (bDirectControlActive) return;
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
    if (bDirectControlActive) return;
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

void ARA4PlayerController::OnAttackMovePressed()
{
    if (bDirectControlActive) return;
    const URA4SimWorldSubsystem* Subsystem = GetSimSubsystem();
    const SimWorld* World = Subsystem != nullptr ? Subsystem->GetSimWorld() : nullptr;
    if (World == nullptr)
    {
        return;
    }

    // Arming with nothing to order would leave the cursor stuck in attack-move until
    // the player found Escape: the click that follows resolves against an empty
    // selection, issues no command, and so never clears the mode. Only units the
    // player owns can carry the order, which is the same filter stop and guard use.
    for (const EntityId& Id : Selection.Get())
    {
        const EntityCore* Core = World->GetCore(Id);
        if (Core != nullptr && Core->Owner == Selection.GetLocalPlayer() && Core->Kind == EntityKind::Unit)
        {
            ArmAttackMove();
            return;
        }
    }
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

// The cursor is the only feedback the player gets before committing a click, so it
// has to be driven by the same hint the order resolver produces -- never by a guess
// about what is under the pointer.
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

void ARA4PlayerController::UpdateGhostPlacement()
{
    if (!bPlacementArmed || !PlacementContent.IsValid())
    {
        if (GhostPlacementActor != nullptr)
        {
            GhostPlacementActor->Destroy();
            GhostPlacementActor = nullptr;
        }
        return;
    }

    const URA4SimWorldSubsystem* Subsystem = GetSimSubsystem();
    const SimWorld* World = Subsystem != nullptr ? Subsystem->GetSimWorld() : nullptr;
    if (World == nullptr || World->GetContent() == nullptr)
    {
        return;
    }

    const EntityDef* Def = World->GetContent()->FindEntity(PlacementContent);
    if (Def == nullptr || Def->Kind != EntityKind::Building)
    {
        return;
    }

    Vec2 Ground;
    if (!GetCursorGroundPosition(Ground))
    {
        return;
    }

    const TileCoord OriginTile = World->GetMap().WorldToTile(Ground);
    const PlayerId LocalPlayer = Selection.GetLocalPlayer();
    const bool bValid = World->IsPlacementValid(PlacementContent, LocalPlayer, OriginTile);

    const Vec2 TileCenter = World->GetMap().TileCenterToWorld(OriginTile);
    const int64 HalfFootprintXUnits = int64((Def->Building.FootprintX - 1) * int32(kTileSizeUnits) / 2);
    const int64 HalfFootprintYUnits = int64((Def->Building.FootprintY - 1) * int32(kTileSizeUnits) / 2);

    FVector GhostLoc = RA4Coords::ToUnreal(Vec2(TileCenter.X + Fixed::FromInt(HalfFootprintXUnits),
                                                TileCenter.Y + Fixed::FromInt(HalfFootprintYUnits)));
    GhostLoc.Z = RA4Coords::GroundZ + 5.0f;

    UWorld* CurrentWorld = GetWorld();
    if (GhostPlacementActor == nullptr && CurrentWorld != nullptr)
    {
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        GhostPlacementActor = CurrentWorld->SpawnActor<ARA4EntityActor>(ARA4EntityActor::StaticClass(), GhostLoc, FRotator::ZeroRotator, SpawnParams);
        if (GhostPlacementActor != nullptr)
        {
            GhostPlacementActor->SetEntityId(UTF8_TO_TCHAR(Def->Name.c_str()));
            const float ScaleVal = FMath::Max(float(Def->Building.FootprintX), float(Def->Building.FootprintY)) * 0.95f;
            GhostPlacementActor->SetVisualScale(FVector(ScaleVal, ScaleVal, ScaleVal));
        }
    }

    if (GhostPlacementActor != nullptr)
    {
        GhostPlacementActor->SetActorLocation(GhostLoc);
        GhostPlacementActor->SetTeamColor(bValid ? FLinearColor(0.1f, 1.0f, 0.35f, 0.65f) : FLinearColor(1.0f, 0.15f, 0.15f, 0.65f));
    }
}

void ARA4PlayerController::HandleBuildCardClicked(int64 ContentIdValue)
{
    const URA4SimWorldSubsystem* Subsystem = GetSimSubsystem();
    const SimWorld* World = Subsystem != nullptr ? Subsystem->GetSimWorld() : nullptr;
    if (World == nullptr || World->GetContent() == nullptr)
    {
        return;
    }

    const ContentId Content = ContentId(uint32(ContentIdValue));
    if (!Content.IsValid())
    {
        return;
    }

    // A structure that has finished building is waiting for a spot, so the same card
    // that queued it now arms placement instead of queueing a second one -- which is
    // exactly how the sidebar behaved in the originals.
    if (const URA4UIDataProviderSubsystem* Provider = GetWorld()->GetSubsystem<URA4UIDataProviderSubsystem>())
    {
        for (const FRA4ProductionEntry& Entry : Provider->GetProductionQueue())
        {
            if (Entry.bAwaitingPlacement && Entry.ContentId == ContentIdValue)
            {
                BeginPlacement(ContentIdValue);
                return;
            }
        }
    }

    const EntityDef* Def = World->GetContent()->FindEntity(Content);
    const bool bShiftHeld = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);
    const int32 CountToQueue = (bShiftHeld && Def != nullptr && Def->Kind == EntityKind::Unit) ? 10 : 1;

    std::vector<Command> Orders;
    Orders.reserve(CountToQueue);
    for (int32 I = 0; I < CountToQueue; ++I)
    {
        Command C;
        C.Type = CommandType::StartProduction;
        C.Issuer = Selection.GetLocalPlayer();
        C.Content = Content;
        Orders.push_back(C);
    }
    SubmitOrders(Orders);
}

void ARA4PlayerController::CycleSelectedPowerPriority()
{
    const EntityId Primary = Selection.GetPrimary();
    if (!Primary.IsValid())
    {
        return;
    }

    // Read the current band from the simulation rather than tracking it here: the UI is
    // not a source of truth, and a stale local copy would send the wrong next value.
    // GetWorld() is null on a PlayerController during teardown and on the CDO, and a
    // bound input action can fire during a level transition.
    const UWorld* OwningWorld = GetWorld();
    const URA4SimWorldSubsystem* Sim =
        OwningWorld != nullptr ? OwningWorld->GetSubsystem<URA4SimWorldSubsystem>() : nullptr;
    const RA4::SimWorld* World = Sim != nullptr ? Sim->GetSimWorld() : nullptr;
    const RA4::BuildingComp* Building = World != nullptr ? World->GetBuilding(Primary) : nullptr;
    if (Building == nullptr)
    {
        return;   // not a building; the sidebar should not have offered the control
    }

    // Wrap round, so one control walks the whole table without needing four buttons.
    const int32 Next = (int32(Building->Priority) + 1) %
                       (int32(RA4::PowerPriority::Auxiliary) + 1);

    Command C;
    C.Type = CommandType::SetPowerPriority;
    C.Issuer = Selection.GetLocalPlayer();
    C.Primary = Primary;
    C.Param = Next;
    SubmitOrders({C});
}

void ARA4PlayerController::ToggleSelectedRepair()
{
    const EntityId Primary = Selection.GetPrimary();
    if (!Primary.IsValid())
    {
        return;
    }

    // The command itself is a toggle, and the simulation validates ownership and kind,
    // so there is nothing to decide here beyond who is asking.
    Command C;
    C.Type = CommandType::RepairBuilding;
    C.Issuer = Selection.GetLocalPlayer();
    C.Primary = Primary;
    SubmitOrders({C});
}

void ARA4PlayerController::CancelPendingAction()
{
    if (bCheatConsoleOpen)
    {
        ToggleCheatConsole();
        return;
    }

    if (bAttackMoveArmed || bPlacementArmed)
    {
        bAttackMoveArmed = false;
        bPlacementArmed = false;
        PlacementContent = ContentId();
        return;
    }

    TogglePauseMenu();
}

void ARA4PlayerController::ToggleCheatConsole()
{
    if (bCheatConsoleOpen)
    {
        if (CheatConsoleOverlay)
        {
            CheatConsoleOverlay->RemoveFromParent();
        }
        bCheatConsoleOpen = false;
        SetInputMode(FInputModeGameAndUI());
        bShowMouseCursor = true;
    }
    else
    {
        if (CheatConsoleOverlay == nullptr)
        {
            CheatConsoleOverlay = CreateWidget<URA4CheatConsoleWidget>(this, URA4CheatConsoleWidget::StaticClass());
        }
        if (CheatConsoleOverlay)
        {
            CheatConsoleOverlay->AddToViewport(150);
            bCheatConsoleOpen = true;
            SetInputMode(FInputModeGameAndUI());
            bShowMouseCursor = true;
        }
    }
}

void ARA4PlayerController::TogglePauseMenu()
{
    if (PauseMenuOverlay != nullptr && PauseMenuOverlay->IsVisible())
    {
        PauseMenuOverlay->SetVisibility(ESlateVisibility::Collapsed);
        FInputModeGameAndUI InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        InputMode.SetHideCursorDuringCapture(false);
        SetInputMode(InputMode);
        bShowMouseCursor = true;
        UGameplayStatics::SetGamePaused(this, false);
    }
    else
    {
        if (PauseMenuOverlay == nullptr)
        {
            if (URA4PauseMenuWidget* MenuWidget = CreateWidget<URA4PauseMenuWidget>(
                this, URA4PauseMenuWidget::StaticClass()))
            {
                MenuWidget->OnResumeRequested.AddUObject(this, &ARA4PlayerController::HandlePauseMenuResume);
                MenuWidget->OnRestartRequested.AddUObject(this, &ARA4PlayerController::HandlePauseMenuRestart);
                MenuWidget->OnSettingsRequested.AddUObject(this, &ARA4PlayerController::HandlePauseMenuSettings);
                MenuWidget->OnQuitToMenuRequested.AddUObject(this, &ARA4PlayerController::HandlePauseMenuQuitToMenu);
                MenuWidget->OnQuitToDesktopRequested.AddUObject(this, &ARA4PlayerController::HandlePauseMenuQuitToDesktop);
                MenuWidget->OnNextTrackRequested.AddUObject(this, &ARA4PlayerController::HandlePauseMenuNextTrack);
                MenuWidget->OnPrevTrackRequested.AddUObject(this, &ARA4PlayerController::HandlePauseMenuPrevTrack);
                MenuWidget->OnToggleMusicPauseRequested.AddUObject(this, &ARA4PlayerController::HandlePauseMenuToggleMusic);
                MenuWidget->OnTrackSelected.AddUObject(this, &ARA4PlayerController::HandlePauseMenuTrackSelected);
                MenuWidget->OnVolumeChanged.AddUObject(this, &ARA4PlayerController::HandlePauseMenuVolumeChanged);

                MenuWidget->OnQualityPresetChanged.AddUObject(this, &ARA4PlayerController::HandlePauseMenuQualityPresetChanged);
                MenuWidget->OnFpsCapChanged.AddUObject(this, &ARA4PlayerController::HandlePauseMenuFpsCapChanged);
                MenuWidget->OnAntiAliasingChanged.AddUObject(this, &ARA4PlayerController::HandlePauseMenuAntiAliasingChanged);
                MenuWidget->OnScreenShakeChanged.AddUObject(this, &ARA4PlayerController::HandlePauseMenuScreenShakeChanged);
                MenuWidget->OnMasterVolumeChanged.AddUObject(this, &ARA4PlayerController::HandlePauseMenuMasterVolumeChanged);
                MenuWidget->OnSfxVolumeChanged.AddUObject(this, &ARA4PlayerController::HandlePauseMenuSfxVolumeChanged);
                MenuWidget->OnEvaVolumeChanged.AddUObject(this, &ARA4PlayerController::HandlePauseMenuEvaVolumeChanged);
                MenuWidget->OnUnitVoicesChanged.AddUObject(this, &ARA4PlayerController::HandlePauseMenuUnitVoicesChanged);
                MenuWidget->OnControlSchemeChanged.AddUObject(this, &ARA4PlayerController::HandlePauseMenuControlSchemeChanged);
                MenuWidget->OnCameraSpeedChanged.AddUObject(this, &ARA4PlayerController::HandlePauseMenuCameraSpeedChanged);
                MenuWidget->OnEdgeScrollChanged.AddUObject(this, &ARA4PlayerController::HandlePauseMenuEdgeScrollChanged);
                MenuWidget->OnHealthBarModeChanged.AddUObject(this, &ARA4PlayerController::HandlePauseMenuHealthBarModeChanged);
                MenuWidget->OnDirectControlFovChanged.AddUObject(this, &ARA4PlayerController::HandlePauseMenuDirectControlFovChanged);

                PauseMenuOverlay = MenuWidget;
                PauseMenuOverlay->AddToViewport(100);
            }
        }
        if (PauseMenuOverlay != nullptr)
        {
            if (URA4PauseMenuWidget* MenuWidget = Cast<URA4PauseMenuWidget>(PauseMenuOverlay))
            {
                if (URA4AudioSubsystem* Audio = GetWorld()->GetSubsystem<URA4AudioSubsystem>())
                {
                    MenuWidget->SetTrackList(Audio->GetTrackTitles(), Audio->GetCurrentTrackIndex());
                    MenuWidget->SetCurrentTrack(Audio->GetCurrentTrackIndex(), Audio->GetCurrentTrackTitle(), Audio->IsMusicPlaying());
                    MenuWidget->SetMusicVolume(Audio->GetMusicVolume());
                }
            }

            PauseMenuOverlay->SetVisibility(ESlateVisibility::Visible);
            FInputModeGameAndUI InputMode;
            InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            InputMode.SetHideCursorDuringCapture(false);
            SetInputMode(InputMode);
            bShowMouseCursor = true;
            UGameplayStatics::SetGamePaused(this, true);
        }
    }
}

void ARA4PlayerController::HandlePauseMenuResume()
{
    TogglePauseMenu();
}

void ARA4PlayerController::HandlePauseMenuRestart()
{
    UGameplayStatics::SetGamePaused(this, false);
    const FString CurrentLevel = GetWorld()->GetMapName();
    UGameplayStatics::OpenLevel(this, FName(*CurrentLevel));
}

void ARA4PlayerController::HandlePauseMenuSettings()
{
    UE_LOG(LogTemp, Display, TEXT("RA4 Pause Menu: Settings clicked"));
}

void ARA4PlayerController::HandlePauseMenuQuitToMenu()
{
    UGameplayStatics::SetGamePaused(this, false);
    static const FString MainMenuMap = TEXT("/Game/Maps/RA4_MainMenu");
    if (FPackageName::DoesPackageExist(MainMenuMap))
    {
        UGameplayStatics::OpenLevel(
            this,
            FName(*MainMenuMap),
            true,
            TEXT("game=/Script/RedAlert4.RA4UIShowcaseGameMode"));
    }
    else
    {
        UGameplayStatics::OpenLevel(
            this,
            TEXT("/Game/Maps/Showcase_UI"),
            true,
            TEXT("game=/Script/RedAlert4.RA4UIShowcaseGameMode"));
    }
}

void ARA4PlayerController::HandlePauseMenuQuitToDesktop()
{
    UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void ARA4PlayerController::HandlePauseMenuNextTrack()
{
    if (URA4AudioSubsystem* Audio = GetWorld()->GetSubsystem<URA4AudioSubsystem>())
    {
        Audio->NextTrack();
        if (URA4PauseMenuWidget* MenuWidget = Cast<URA4PauseMenuWidget>(PauseMenuOverlay))
        {
            MenuWidget->SetCurrentTrack(Audio->GetCurrentTrackIndex(), Audio->GetCurrentTrackTitle(), Audio->IsMusicPlaying());
        }
    }
}

void ARA4PlayerController::HandlePauseMenuPrevTrack()
{
    if (URA4AudioSubsystem* Audio = GetWorld()->GetSubsystem<URA4AudioSubsystem>())
    {
        Audio->PreviousTrack();
        if (URA4PauseMenuWidget* MenuWidget = Cast<URA4PauseMenuWidget>(PauseMenuOverlay))
        {
            MenuWidget->SetCurrentTrack(Audio->GetCurrentTrackIndex(), Audio->GetCurrentTrackTitle(), Audio->IsMusicPlaying());
        }
    }
}

void ARA4PlayerController::HandlePauseMenuToggleMusic()
{
    if (URA4AudioSubsystem* Audio = GetWorld()->GetSubsystem<URA4AudioSubsystem>())
    {
        Audio->ToggleMusicPause();
        if (URA4PauseMenuWidget* MenuWidget = Cast<URA4PauseMenuWidget>(PauseMenuOverlay))
        {
            MenuWidget->SetCurrentTrack(Audio->GetCurrentTrackIndex(), Audio->GetCurrentTrackTitle(), Audio->IsMusicPlaying());
        }
    }
}

void ARA4PlayerController::HandlePauseMenuTrackSelected(int32 TrackIndex)
{
    if (URA4AudioSubsystem* Audio = GetWorld()->GetSubsystem<URA4AudioSubsystem>())
    {
        Audio->PlayTrackByIndex(TrackIndex);
        if (URA4PauseMenuWidget* MenuWidget = Cast<URA4PauseMenuWidget>(PauseMenuOverlay))
        {
            MenuWidget->SetCurrentTrack(Audio->GetCurrentTrackIndex(), Audio->GetCurrentTrackTitle(), Audio->IsMusicPlaying());
        }
    }
}

void ARA4PlayerController::HandlePauseMenuVolumeChanged(float DeltaVolume)
{
    if (URA4AudioSubsystem* Audio = GetWorld()->GetSubsystem<URA4AudioSubsystem>())
    {
        const float NewVol = FMath::Clamp(Audio->GetMusicVolume() + DeltaVolume, 0.0f, 1.0f);
        Audio->SetMusicVolume(NewVol);
        if (URA4PauseMenuWidget* MenuWidget = Cast<URA4PauseMenuWidget>(PauseMenuOverlay))
        {
            MenuWidget->SetMusicVolume(NewVol);
        }
    }
}

void ARA4PlayerController::HandlePauseMenuQualityPresetChanged(int32 Preset)
{
    const int32 Clamped = FMath::Clamp(Preset, 0, 3);
    ConsoleCommand(FString::Printf(TEXT("sg.ViewDistanceQuality %d"), Clamped));
    ConsoleCommand(FString::Printf(TEXT("sg.AntiAliasingQuality %d"), Clamped));
    ConsoleCommand(FString::Printf(TEXT("sg.ShadowQuality %d"), Clamped));
    ConsoleCommand(FString::Printf(TEXT("sg.PostProcessQuality %d"), Clamped));
    ConsoleCommand(FString::Printf(TEXT("sg.TextureQuality %d"), Clamped));
    ConsoleCommand(FString::Printf(TEXT("sg.EffectsQuality %d"), Clamped));
    ConsoleCommand(FString::Printf(TEXT("sg.FoliageQuality %d"), Clamped));
    ConsoleCommand(FString::Printf(TEXT("sg.ShadingQuality %d"), Clamped));
    UE_LOG(LogTemp, Display, TEXT("RA4 Graphics Quality set to preset %d"), Clamped);
}

void ARA4PlayerController::HandlePauseMenuFpsCapChanged(int32 FpsCap)
{
    ConsoleCommand(FString::Printf(TEXT("t.MaxFPS %d"), FpsCap));
    UE_LOG(LogTemp, Display, TEXT("RA4 FPS Cap set to %d"), FpsCap);
}

void ARA4PlayerController::HandlePauseMenuAntiAliasingChanged(int32 AaIndex)
{
    int32 UEMethod = 2; // Default TAA
    if (AaIndex == 0) UEMethod = 1; // FXAA
    else if (AaIndex == 1) UEMethod = 2; // TAA
    else if (AaIndex == 2) UEMethod = 4; // TSR
    ConsoleCommand(FString::Printf(TEXT("r.AntiAliasingMethod %d"), UEMethod));
    UE_LOG(LogTemp, Display, TEXT("RA4 Anti-Aliasing set to method %d"), UEMethod);
}

void ARA4PlayerController::HandlePauseMenuScreenShakeChanged(bool bEnabled)
{
    UE_LOG(LogTemp, Display, TEXT("RA4 Screen Shake set to %s"), bEnabled ? TEXT("Enabled") : TEXT("Disabled"));
}

void ARA4PlayerController::HandlePauseMenuMasterVolumeChanged(float Volume)
{
    ConsoleCommand(FString::Printf(TEXT("au.SetMasterVolume %f"), Volume));
    if (URA4AudioSubsystem* Audio = GetWorld()->GetSubsystem<URA4AudioSubsystem>())
    {
        Audio->SetMasterVolume(Volume);
    }
    UE_LOG(LogTemp, Display, TEXT("RA4 Master volume set to %f"), Volume);
}

void ARA4PlayerController::HandlePauseMenuSfxVolumeChanged(float Volume)
{
    if (URA4AudioSubsystem* Audio = GetWorld()->GetSubsystem<URA4AudioSubsystem>())
    {
        Audio->SetSfxVolume(Volume);
    }
    UE_LOG(LogTemp, Display, TEXT("RA4 SFX volume set to %f"), Volume);
}

void ARA4PlayerController::HandlePauseMenuEvaVolumeChanged(float Volume)
{
    if (URA4AudioSubsystem* Audio = GetWorld()->GetSubsystem<URA4AudioSubsystem>())
    {
        Audio->SetEvaVolume(Volume);
    }
    UE_LOG(LogTemp, Display, TEXT("RA4 EVA volume set to %f"), Volume);
}

void ARA4PlayerController::HandlePauseMenuUnitVoicesChanged(bool bEnabled)
{
    if (URA4AudioSubsystem* Audio = GetWorld()->GetSubsystem<URA4AudioSubsystem>())
    {
        Audio->SetUnitVoicesEnabled(bEnabled);
    }
    UE_LOG(LogTemp, Display, TEXT("RA4 Unit chatter set to %s"), bEnabled ? TEXT("Enabled") : TEXT("Disabled"));
}

void ARA4PlayerController::HandlePauseMenuControlSchemeChanged(int32 SchemeIndex)
{
    Scheme = SchemeIndex == 0 ? RA4::Input::ControlScheme::ClassicRA : RA4::Input::ControlScheme::Modern;
    UE_LOG(LogTemp, Display, TEXT("RA4 Control Scheme changed to %s"), SchemeIndex == 0 ? TEXT("Classic C&C (LMB)") : TEXT("Modern RTS (RMB)"));
}

void ARA4PlayerController::HandlePauseMenuCameraSpeedChanged(float SpeedMultiplier)
{
    if (ARA4CameraPawn* CameraPawn = GetCameraPawn())
    {
        RA4::Input::CameraConfig Config = CameraPawn->GetCameraController().GetConfig();
        Config.PanSpeedAtMinHeight = 1800.0f * SpeedMultiplier;
        CameraPawn->GetCameraController().Configure(Config);
    }
    UE_LOG(LogTemp, Display, TEXT("RA4 Camera Pan Speed multiplier set to %f"), SpeedMultiplier);
}

void ARA4PlayerController::HandlePauseMenuEdgeScrollChanged(bool bEnabled)
{
    if (ARA4CameraPawn* CameraPawn = GetCameraPawn())
    {
        RA4::Input::CameraConfig Config = CameraPawn->GetCameraController().GetConfig();
        Config.bEdgeScrollEnabled = bEnabled;
        CameraPawn->GetCameraController().Configure(Config);
    }
    UE_LOG(LogTemp, Display, TEXT("RA4 Edge Scroll set to %s"), bEnabled ? TEXT("Enabled") : TEXT("Disabled"));
}

void ARA4PlayerController::HandlePauseMenuHealthBarModeChanged(int32 ModeIndex)
{
    UE_LOG(LogTemp, Display, TEXT("RA4 Health Bar Mode set to %d"), ModeIndex);
}

void ARA4PlayerController::HandlePauseMenuDirectControlFovChanged(float FOV)
{
    if (URA4DirectControlSubsystem* Dc = GetDirectControlSubsystem())
    {
        if (Dc->ActiveProfile != nullptr)
        {
            Dc->ActiveProfile->Camera.WideFOV = FOV;
        }
    }
    UE_LOG(LogTemp, Display, TEXT("RA4 Direct Control FOV set to %f"), FOV);
}

void ARA4PlayerController::HandleRadarClicked(FVector2D WorldPosition)
{
    if (ARA4CameraPawn* CameraPawn = GetCameraPawn())
    {
        CameraPawn->GetCameraController().FocusOn(
            Vec2f(float(WorldPosition.X), float(WorldPosition.Y)), true);
    }
}

void ARA4PlayerController::HandleRadarOrdered(FVector2D WorldPosition)
{
    URA4SimWorldSubsystem* Subsystem = GetSimSubsystem();
    const SimWorld* World = Subsystem != nullptr ? Subsystem->GetSimWorld() : nullptr;
    if (World == nullptr || Selection.IsEmpty())
    {
        return;   // nothing selected: a right-click on the map is not an order
    }

    // Routed through the ordinary order resolver and the ordinary command queue, so a
    // minimap order is indistinguishable from a right-click in the world -- same validation,
    // same replay, same server authority. A separate path here would be a second way to
    // move an army, and the two would drift.
    // Same rounding as RA4Coords::FromUnreal, so a minimap order and a world right-click on
    // the identical spot resolve to the identical fixed-point tile.
    const Vec2 Ground = Vec2(
        RA4::Fixed::FromRaw(FMath::RoundToInt64(WorldPosition.X * double(RA4::kFixedOne))),
        RA4::Fixed::FromRaw(FMath::RoundToInt64(WorldPosition.Y * double(RA4::kFixedOne))));
    OrderContext Context = MakeOrderContext(Ground);
    // The pointer is over the sidebar, not the terrain, so whatever entity happens to sit at
    // those world coordinates was not hovered in any meaningful sense. Clearing it keeps a
    // minimap click a move order rather than an attack on something the player cannot see.
    Context.HoveredEntity = EntityId::Invalid();
    SubmitOrders(ResolveOrder(*World, Selection, Context));
}

void ARA4PlayerController::UpdateRadarCameraView()
{
    if (Sidebar == nullptr)
    {
        return;
    }

    int32 ViewportX = 0;
    int32 ViewportY = 0;
    GetViewportSize(ViewportX, ViewportY);
    if (ViewportX <= 0 || ViewportY <= 0)
    {
        return;
    }

    // Deprojected from the actual viewport corners rather than derived from the camera
    // height, so the outline matches what is really on screen at any pitch or zoom. The
    // sidebar covers the right-hand strip, so the visible world stops short of the full
    // width -- using the whole viewport would draw a frame wider than the player can see.
    const float RightEdge = FMath::Max(1.0f, float(ViewportX) - AppliedSidebarReservedWidth);
    const FVector2D Corners[4] = {
        FVector2D(0.0f, 0.0f),
        FVector2D(RightEdge, 0.0f),
        FVector2D(0.0f, float(ViewportY)),
        FVector2D(RightEdge, float(ViewportY)),
    };

    bool bAny = false;
    double MinX = 0.0, MaxX = 0.0, MinY = 0.0, MaxY = 0.0;
    for (const FVector2D& Corner : Corners)
    {
        Vec2 Ground;
        if (!ScreenToGround(Corner, Ground))
        {
            // A near-horizon corner can miss the ground plane entirely. Reporting an unknown
            // frame is better than one built from the corners that happened to hit.
            Sidebar->SetRadarCameraView(FVector2D::ZeroVector, FVector2D::ZeroVector);
            return;
        }
        const double X = Ground.X.ToDoubleUnsafe();
        const double Y = Ground.Y.ToDoubleUnsafe();
        if (!bAny)
        {
            MinX = MaxX = X;
            MinY = MaxY = Y;
            bAny = true;
            continue;
        }
        MinX = FMath::Min(MinX, X);
        MaxX = FMath::Max(MaxX, X);
        MinY = FMath::Min(MinY, Y);
        MaxY = FMath::Max(MaxY, Y);
    }

    Sidebar->SetRadarCameraView(FVector2D((MinX + MaxX) * 0.5, (MinY + MaxY) * 0.5),
                               FVector2D(MaxX - MinX, MaxY - MinY));
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

    bool bHasMoveOrAttack = false;
    bool bHasBuild = false;

    for (const Command& C : Commands)
    {
        Subsystem->EnqueueCommand(C);
        if (C.Type == CommandType::Move || C.Type == CommandType::Attack || C.Type == CommandType::AttackMove)
        {
            bHasMoveOrAttack = true;
        }
        else if (C.Type == CommandType::PlaceBuilding || C.Type == CommandType::StartProduction)
        {
            bHasBuild = true;
        }
    }

    // Play unit voice response or sound feedback
    if (bHasMoveOrAttack)
    {
        static USoundBase* MoveSound = LoadObject<USoundBase>(nullptr, TEXT("/Engine/VREditor/Sounds/UI/VR_Click_01.VR_Click_01"));
        if (MoveSound)
        {
            UGameplayStatics::PlaySound2D(this, MoveSound, 0.7f, 1.2f);
        }
    }
    else if (bHasBuild)
    {
        static USoundBase* BuildSound = LoadObject<USoundBase>(nullptr, TEXT("/Engine/VREditor/Sounds/UI/VR_Teleport_Mode_01.VR_Teleport_Mode_01"));
        if (BuildSound)
        {
            UGameplayStatics::PlaySound2D(this, BuildSound, 0.8f, 1.0f);
        }
    }
}

URA4SimWorldSubsystem* ARA4PlayerController::GetSimSubsystem() const
{
    UWorld* World = GetWorld();
    return World != nullptr ? World->GetSubsystem<URA4SimWorldSubsystem>() : nullptr;
}

void ARA4PlayerController::JumpToHomeBase()
{
    const URA4SimWorldSubsystem* SimSub = GetSimSubsystem();
    if (SimSub == nullptr || SimSub->GetSimWorld() == nullptr)
    {
        return;
    }

    const RA4::SimWorld* Sim = SimSub->GetSimWorld();
    const RA4::PlayerId LocalPlayer = Selection.GetLocalPlayer();

    RA4::Vec2 TargetPos(RA4::Fixed::FromInt(2400), RA4::Fixed::FromInt(2400));
    bool bFound = false;

    const auto& Cores = Sim->GetAllCores();
    const auto& Transforms = Sim->GetAllTransforms();
    const auto* Content = Sim->GetContent();

    // 1. First priority: Construction Yard or MCV
    for (size_t I = 0; I < Cores.size() && I < Transforms.size(); ++I)
    {
        if (Cores[I].bAlive && Cores[I].Owner == LocalPlayer)
        {
            if (Content != nullptr)
            {
                const auto* Def = Content->FindEntity(Cores[I].Def);
                if (Def != nullptr)
                {
                    if (Def->Name.find("construction_yard") != std::string::npos ||
                        Def->Name.find("mcv") != std::string::npos ||
                        Def->Name.find("headquarters") != std::string::npos)
                    {
                        TargetPos = Transforms[I].Position;
                        bFound = true;
                        break;
                    }
                }
            }
        }
    }

    // 2. Second priority: Any owned unit or building
    if (!bFound)
    {
        for (size_t I = 0; I < Cores.size() && I < Transforms.size(); ++I)
        {
            if (Cores[I].bAlive && Cores[I].Owner == LocalPlayer)
            {
                TargetPos = Transforms[I].Position;
                bFound = true;
                break;
            }
        }
    }

    if (ARA4CameraPawn* Cam = Cast<ARA4CameraPawn>(GetPawn()))
    {
        const FVector UnrealPos = RA4Coords::ToUnreal(TargetPos);
        Cam->GetCameraController().FocusOn(RA4::Input::Vec2f(float(UnrealPos.X), float(UnrealPos.Y)), /*bInstant*/ false);
        UE_LOG(LogTemp, Display, TEXT("RA4: Jumped to Home Base at (%.0f, %.0f)"), UnrealPos.X, UnrealPos.Y);
    }
}

void ARA4PlayerController::DeploySelectedMcv()
{
    const URA4SimWorldSubsystem* SimSub = GetSimSubsystem();
    if (SimSub == nullptr || SimSub->GetSimWorld() == nullptr)
    {
        return;
    }

    const RA4::SimWorld* Sim = SimSub->GetSimWorld();
    const RA4::PlayerId LocalPlayer = Selection.GetLocalPlayer();
    const auto* Content = Sim->GetContent();
    const auto& Cores = Sim->GetAllCores();

    RA4::EntityId TargetEntity = RA4::EntityId::Invalid();

    // 1. Check selected entities first (MCV or Construction Yard)
    const auto& Sel = Selection.Get();
    for (const auto& Id : Sel)
    {
        if (Id.IsValid() && Id.Index < Cores.size() && Cores[Id.Index].bAlive && Cores[Id.Index].Owner == LocalPlayer)
        {
            const auto* Def = Content ? Content->FindEntity(Cores[Id.Index].Def) : nullptr;
            if (Def != nullptr)
            {
                // MCV builder unit
                if (Cores[Id.Index].Kind == RA4::EntityKind::Unit && Def->Unit.bIsBuilder && Def->Unit.DeploysInto.IsValid())
                {
                    TargetEntity = Id;
                    break;
                }
                // Construction Yard building (to pack/undeploy)
                if (Cores[Id.Index].Kind == RA4::EntityKind::Building && Def->Name.find("construction_yard") != std::string::npos)
                {
                    TargetEntity = Id;
                    break;
                }
            }
        }
    }

    // 2. If nothing selected and player has 0 buildings, allow D to deploy starting MCV
    if (!TargetEntity.IsValid() && Sel.empty())
    {
        bool bHasAnyBuilding = false;
        RA4::EntityId McvCandidate = RA4::EntityId::Invalid();
        for (size_t I = 0; I < Cores.size(); ++I)
        {
            if (Cores[I].bAlive && Cores[I].Owner == LocalPlayer)
            {
                if (Cores[I].Kind == RA4::EntityKind::Building)
                {
                    bHasAnyBuilding = true;
                    break;
                }
                if (Cores[I].Kind == RA4::EntityKind::Unit)
                {
                    const auto* Def = Content ? Content->FindEntity(Cores[I].Def) : nullptr;
                    if (Def != nullptr && Def->Unit.bIsBuilder && Def->Unit.DeploysInto.IsValid())
                    {
                        McvCandidate = Sim->MakeId(uint32_t(I));
                    }
                }
            }
        }
        if (!bHasAnyBuilding && McvCandidate.IsValid())
        {
            TargetEntity = McvCandidate;
        }
    }

    if (TargetEntity.IsValid())
    {
        const auto* Def = Content ? Content->FindEntity(Cores[TargetEntity.Index].Def) : nullptr;
        const bool bIsMcv = (Cores[TargetEntity.Index].Kind == RA4::EntityKind::Unit && Def != nullptr && Def->Unit.bIsBuilder && Def->Unit.DeploysInto.IsValid());

        // If it's an MCV and placement mode is not yet armed, arm placement mode to show the green/red ConYard ghost!
        if (bIsMcv && !bPlacementArmed)
        {
            BeginPlacement(Def->Unit.DeploysInto.Value);
            UE_LOG(LogTemp, Display, TEXT("RA4: Armed MCV ConYard placement ghost preview for Entity %u"), TargetEntity.Index);
            return;
        }

        // If placement is already armed or it's a ConYard folding back into MCV, execute deployment!
        RA4::Command Cmd;
        Cmd.Type = RA4::CommandType::Deploy;
        Cmd.Issuer = LocalPlayer;
        Cmd.Primary = TargetEntity;

        Vec2 CursorGround;
        if (bPlacementArmed && GetCursorGroundPosition(CursorGround))
        {
            Cmd.Tile = Sim->GetMap().WorldToTile(CursorGround);
        }

        bPlacementArmed = false;
        PlacementContent = ContentId();

        URA4SimWorldSubsystem* MutableSim = GetWorld() ? GetWorld()->GetSubsystem<URA4SimWorldSubsystem>() : nullptr;
        if (MutableSim != nullptr)
        {
            MutableSim->EnqueueCommand(Cmd);
            UE_LOG(LogTemp, Display, TEXT("RA4: Issued Deploy/Pack command on Entity (Index=%u)"), TargetEntity.Index);

            // Audio feedback
            static USoundBase* DeploySound = LoadObject<USoundBase>(nullptr, TEXT("/Engine/VREditor/Sounds/UI/VR_Teleport_Mode_01.VR_Teleport_Mode_01"));
            if (DeploySound)
            {
                UGameplayStatics::PlaySound2D(this, DeploySound, 1.0f, 0.9f);
            }
        }
    }
}

void ARA4PlayerController::OnDirectControlTogglePressed()
{
    ToggleDirectControl();
}

void ARA4PlayerController::ToggleDirectControl()
{
    URA4DirectControlSubsystem* Dc = GetDirectControlSubsystem();
    if (Dc == nullptr)
    {
        return;
    }
    if (Dc->IsInDirectControl())
    {
        Dc->RequestExit(this);
        return;
    }
    // Resolve the vehicle to enter: prefer the primary selection, then any
    // owned alive unit. The subsystem re-validates ownership against the
    // authoritative SimWorld before submitting the Enter command, so a stale
    // selection or a just-died unit is rejected cleanly.
    RA4::EntityId Target = Selection.GetPrimary();
    if (!Target.IsValid())
    {
        const auto& Sel = Selection.Get();
        if (!Sel.empty())
        {
            Target = Sel[0];
        }
    }
    if (!Target.IsValid())
    {
        if (const URA4SimWorldSubsystem* Sim = GetSimSubsystem())
        {
            if (const SimWorld* World = Sim->GetSimWorld())
            {
                const auto& Cores = World->GetAllCores();
                for (size_t I = 0; I < Cores.size(); ++I)
                {
                    if (Cores[I].bAlive && Cores[I].Owner == Selection.GetLocalPlayer() &&
                        Cores[I].Kind == EntityKind::Unit)
                    {
                        Target = World->MakeId(uint32_t(I));
                        break;
                    }
                }
            }
        }
    }
    if (Target.IsValid())
    {
        if (Dc->RequestEnter(this, Target))
        {
            bDirectControlActive = true;
            DirectControlEntityId = Target;
        }
    }
}

void ARA4PlayerController::EnterDirectControl(const EntityId TargetId)
{
    if (URA4DirectControlSubsystem* Dc = GetDirectControlSubsystem())
    {
        Dc->RequestEnter(this, TargetId);
    }
    UE_LOG(LogTemp, Display, TEXT("RA4 First Person Direct Control activated for entity %u"), TargetId.Index);
}

void ARA4PlayerController::ExitDirectControl()
{
    if (URA4DirectControlSubsystem* Dc = GetDirectControlSubsystem())
    {
        Dc->RequestExit(this);
    }
}

void ARA4PlayerController::UpdateDirectControl(float DeltaTime)
{
    URA4DirectControlSubsystem* Dc = GetDirectControlSubsystem();
    if (Dc == nullptr)
    {
        return;
    }
    Dc->TickInput(this, DeltaTime);
    Dc->TickPresentation(this, DeltaTime);

    if (WasInputKeyJustPressed(EKeys::Escape))
    {
        TogglePauseMenu();
        return;
    }

    // Lazily create the HUD ViewModel when we first enter direct control.
    if (DirectControlHUDViewModel == nullptr && Dc->IsInDirectControl())
    {
        DirectControlHUDViewModel = NewObject<URA4DirectControlHUDViewModel>(this);
        UE_LOG(LogTemp, Display, TEXT("RA4 DirectControl: HUD ViewModel created"));
    }

    // Refresh the ViewModel from the authoritative SimWorld every frame.
    // The ViewModel is presentation-only; the Blueprint widget binds to it
    // and never touches the simulation.
    if (DirectControlHUDViewModel != nullptr && Dc->IsInDirectControl())
    {
        if (const URA4SimWorldSubsystem* Sim = GetSimSubsystem())
        {
            if (const SimWorld* World = Sim->GetSimWorld())
            {
                const int32 VehIndex = Dc->GetControlledVehicleIndex();
                if (VehIndex >= 0)
                {
                    const EntityId Vehicle{uint32_t(VehIndex), 0u};
                    const HealthComp* Hp = World->GetHealth(Vehicle);
                    const TransformComp* Tf = World->GetTransform(Vehicle);
                    const MovementComp* Mv = World->GetMovement(Vehicle);
                    const DirectControlComp* DcComp = World->GetDirectControl(Vehicle);
                    const EntityCore* Core = World->GetCore(Vehicle);
                    const ContentDatabase* Db = World->GetContent();

                    int32 VehicleHealth = 0, VehicleMaxHealth = 1;
                    if (Hp != nullptr)
                    {
                        VehicleHealth = Hp->Current;
                        VehicleMaxHealth = Hp->Max > 0 ? Hp->Max : 1;
                    }
                    float SpeedKph = 0.0f;
                    if (Mv != nullptr)
                    {
                        SpeedKph = float(Mv->CurrentSpeed.ToDoubleUnsafe() / 100.0 * 3.6);
                    }
                    int32 TurretYaw = 0, HullYaw = 0;
                    if (Tf != nullptr)
                    {
                        TurretYaw = int32(Tf->TurretFacing / 100) % 360;
                        HullYaw = int32(Tf->Facing / 100) % 360;
                    }
                    bool bZoomed = false;
                    int32 PrimaryCdPct = 0;
                    bool bPrimaryReloading = false;
                    ContentId VehicleDefId;
                    if (Core != nullptr) { VehicleDefId = Core->Def; }
                    const EntityDef* Def = (Db != nullptr && VehicleDefId.IsValid()) ? Db->FindEntity(VehicleDefId) : nullptr;
                    if (DcComp != nullptr)
                    {
                        bZoomed = DcComp->bOpticsZoomed;
                        if (Def != nullptr && Def->Weapon.IsValid() && Db != nullptr)
                        {
                            if (const WeaponDef* Wpn = Db->FindWeapon(Def->Weapon))
                            {
                                if (Wpn->CooldownTicks > 0)
                                {
                                    PrimaryCdPct = FMath::Clamp((DcComp->CooldownTicksPrimary * 100) / Wpn->CooldownTicks, 0, 100);
                                    bPrimaryReloading = DcComp->CooldownTicksPrimary > 0;
                                }
                            }
                        }
                    }
                    FText PrimaryName = FText::GetEmpty();
                    FText SecondaryName = FText::GetEmpty();
                    if (Def != nullptr && Db != nullptr)
                    {
                        if (Def->Weapon.IsValid())
                        {
                            if (const WeaponDef* Wpn = Db->FindWeapon(Def->Weapon))
                            {
                                PrimaryName = FText::FromString(UTF8_TO_TCHAR(Wpn->Name.c_str()));
                            }
                        }
                        if (Def->SecondaryWeapon.IsValid())
                        {
                            if (const WeaponDef* Wpn = Db->FindWeapon(Def->SecondaryWeapon))
                            {
                                SecondaryName = FText::FromString(UTF8_TO_TCHAR(Wpn->Name.c_str()));
                            }
                        }
                    }
                    // Damage states inferred from HP percent (Stage 2 placeholder
                    // until the sim has a proper subsystem-damage model).
                    const int32 HpPct = (VehicleHealth * 100) / VehicleMaxHealth;
                    const bool bEngineDamaged = HpPct < 25;
                    const bool bTracksDamaged = HpPct < 50;
                    const bool bTurretDamaged = HpPct < 10;

                    DirectControlHUDViewModel->Refresh(
                        VehicleHealth, VehicleMaxHealth,
                        SpeedKph,
                        TurretYaw, HullYaw,
                        0,       // target range (Stage 2: raycast from reticle)
                        bZoomed,
                        0,       // detected target count (Stage 2: fog-of-war query)
                        PrimaryName,
                        PrimaryCdPct, bPrimaryReloading,
                        SecondaryName,
                        bEngineDamaged, bTracksDamaged, bTurretDamaged,
                        FText::GetEmpty(),  // current task (Stage 2)
                        FText::GetEmpty()); // EVA message (Stage 2)

                    if (ARA4HUD* Ra4Hud = Cast<ARA4HUD>(GetHUD()))
                    {
                        Ra4Hud->UpdateDirectControlDisplay(
                            true,
                            VehicleHealth, VehicleMaxHealth,
                            PrimaryName, SecondaryName,
                            float(PrimaryCdPct) / 100.0f,
                            0.0f,
                            SpeedKph,
                            bZoomed
                        );
                    }
                }
            }
        }
    }

    // If the authoritative phase is back to RTS (vehicle destroyed or exit
    // completed), restore the RTS view target and mouse cursor.
    if (Dc->GetClientPhase() == ERA4DirectControlClientPhase::RTS ||
        Dc->GetClientPhase() == ERA4DirectControlClientPhase::VehicleDestroyed)
    {
        if (ARA4CameraPawn* CameraPawn = GetCameraPawn())
        {
            SetViewTargetWithBlend(CameraPawn, 0.25f);
        }
        bShowMouseCursor = true;
        FInputModeGameAndUI InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        InputMode.SetHideCursorDuringCapture(false);
        SetInputMode(InputMode);
        bDirectControlActive = false;
        DirectControlEntityId = EntityId{};
        if (DirectControlHUDViewModel != nullptr)
        {
            DirectControlHUDViewModel = nullptr;
        }
        if (ARA4HUD* Ra4Hud = Cast<ARA4HUD>(GetHUD()))
        {
            Ra4Hud->UpdateDirectControlDisplay(false, 0, 0, FText::GetEmpty(), FText::GetEmpty(), 0.0f, 0.0f, 0.0f, false);
        }
        if (GEngine != nullptr)
        {
            GEngine->AddOnScreenDebugMessage(-1, 2.5f, FColor::Yellow,
                TEXT("[DIRECT CONTROL OFF] RTS view restored"));
        }
    }
}

ARA4CameraPawn* ARA4PlayerController::GetCameraPawn() const
{
    return Cast<ARA4CameraPawn>(GetPawn());
}

URA4DirectControlSubsystem* ARA4PlayerController::GetDirectControlSubsystem() const
{
    UWorld* World = GetWorld();
    return World != nullptr ? World->GetSubsystem<URA4DirectControlSubsystem>() : nullptr;
}

bool ARA4PlayerController::IsDirectControlActive() const
{
    if (URA4DirectControlSubsystem* Dc = GetDirectControlSubsystem())
    {
        return Dc->IsInDirectControl();
    }
    return false;
}

void ARA4PlayerController::CaptureHudForQA()
{
    const FString ScreenshotPath = FPaths::Combine(
        FPaths::ProjectSavedDir(), TEXT("Screenshots/MacEditor/RA4_UI_QA_MatchHUD.png"));
    FScreenshotRequest::RequestScreenshot(ScreenshotPath, true, false);
    UE_LOG(LogTemp, Display, TEXT("RA4 HUD QA screenshot requested: %s"), *ScreenshotPath);
}
