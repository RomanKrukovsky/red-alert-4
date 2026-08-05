// Copyright (c) Red Alert 4 project.
#include "RA4PlayerController.h"

#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerInput.h"
#include "RA4CameraPawn.h"
#include "RA4SimCoords.h"
#include "RA4SimWorldSubsystem.h"
#include "RA4HUDWidget.h"
#include "RA4MatchResultOverlayWidget.h"
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
#include "Layout/WidgetPath.h"
#include "Misc/PackageName.h"
#include "UnrealClient.h"
#include "EngineUtils.h"
#include "Camera/CameraComponent.h"

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
            // The reserved strip and the widget's own width come from the same helper. If
            // they ever disagree the world is drawn under the column, or a band of
            // background shows beside it.
            const float ReservedWidth = URA4SidebarWidget::ComputeSidebarWidth(this);
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

    // Bound directly to keys rather than through an InputMappingContext asset: the
    // whole control scheme stays in code and in version control, and the project
    // needs no editor-authored asset to be playable. Remapping moves to Enhanced
    // Input with the settings screen.
    InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ARA4PlayerController::OnPrimaryPressed);
    InputComponent->BindKey(EKeys::LeftMouseButton, IE_Released, this, &ARA4PlayerController::OnPrimaryReleased);
    InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &ARA4PlayerController::OnSecondaryPressed);
    InputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &ARA4PlayerController::OnSecondaryReleased);
    InputComponent->BindKey(EKeys::MiddleMouseButton, IE_Pressed, this, &ARA4PlayerController::OnMiddlePressed);
    InputComponent->BindKey(EKeys::MiddleMouseButton, IE_Released, this, &ARA4PlayerController::OnMiddleReleased);

    InputComponent->BindKey(EKeys::MouseScrollUp, IE_Pressed, this, &ARA4PlayerController::OnZoomIn);
    InputComponent->BindKey(EKeys::MouseScrollDown, IE_Pressed, this, &ARA4PlayerController::OnZoomOut);

    // Ensure WASD and arrow keys are tracked by PlayerInput for camera panning.
    // A is deliberately absent: it arms attack-move below, which is what the genre
    // and this class's own documentation expect of it. Panning left is left to the
    // arrow key, the screen edge and the middle-mouse drag.
    InputComponent->BindKey(EKeys::W, IE_Pressed, this, &ARA4PlayerController::OnDummyPanKey);
    InputComponent->BindKey(EKeys::S, IE_Pressed, this, &ARA4PlayerController::OnDummyPanKey);
    InputComponent->BindKey(EKeys::D, IE_Pressed, this, &ARA4PlayerController::OnDummyPanKey);
    InputComponent->BindKey(EKeys::Up, IE_Pressed, this, &ARA4PlayerController::OnDummyPanKey);
    InputComponent->BindKey(EKeys::Down, IE_Pressed, this, &ARA4PlayerController::OnDummyPanKey);
    InputComponent->BindKey(EKeys::Left, IE_Pressed, this, &ARA4PlayerController::OnDummyPanKey);
    // Right was missing from this list while its three neighbours were present.
    InputComponent->BindKey(EKeys::Right, IE_Pressed, this, &ARA4PlayerController::OnDummyPanKey);
    InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &ARA4PlayerController::OnDummyPanKey);
    InputComponent->BindKey(EKeys::SpaceBar, IE_Released, this, &ARA4PlayerController::OnDummyPanKey);

    InputComponent->BindKey(EKeys::F, IE_Pressed, this, &ARA4PlayerController::OnDirectControlTogglePressed);
    InputComponent->BindKey(EKeys::A, IE_Pressed, this, &ARA4PlayerController::OnAttackMovePressed);
    InputComponent->BindKey(EKeys::X, IE_Pressed, this, &ARA4PlayerController::OnStopPressed);
    InputComponent->BindKey(EKeys::G, IE_Pressed, this, &ARA4PlayerController::OnGuardPressed);
    InputComponent->BindKey(EKeys::Tilde, IE_Pressed, this, &ARA4PlayerController::ToggleCheatConsole);
    InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ARA4PlayerController::CancelPendingAction);

    // Control groups 1..9 then 0, matching the on-screen numbering.
    static const FKey GroupKeys[10] = {EKeys::One, EKeys::Two,   EKeys::Three, EKeys::Four, EKeys::Five,
                                       EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine, EKeys::Zero};
    for (int32 Index = 0; Index < 10; ++Index)
    {
        InputComponent->BindKey(GroupKeys[Index], IE_Pressed, this, &ARA4PlayerController::OnControlGroupKeyByKey);
    }

    // Build card hotkeys, in the same grid order as the badges the sidebar draws.
    for (const FKey& Key : GetBuildCardHotkeys())
    {
        InputComponent->BindKey(Key, IE_Pressed, this, &ARA4PlayerController::OnBuildCardKeyByKey);
    }
}

const TArray<FKey>& ARA4PlayerController::GetBuildCardHotkeys()
{
    // Deliberately not the digits: those are control groups above, and the ordinary RTS
    // reflex of pressing a number to recall a squad has to keep working. Order matches
    // URA4SidebarWidget's badge table, and the assert catches a key added to one table
    // and not the other rather than leaving a badge that does nothing.
    static const TArray<FKey> Keys = {
        EKeys::Q, EKeys::E, EKeys::R, EKeys::T,
        EKeys::Y, EKeys::U, EKeys::I, EKeys::O,
        EKeys::P, EKeys::H, EKeys::J, EKeys::K,
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
    // Offset so the card sits clear of the pointer instead of under it.
    HoverTooltip->SetPositionInViewport(FVector2D(MouseX + 18.0f, MouseY + 18.0f), /*bRemoveDPIScale*/ false);
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
    static const TCHAR* MainMenuMap = TEXT("/Engine/Maps/Entry");
    UE_LOG(LogTemp, Display, TEXT("RA4 HUD: leaving match for %s"), MainMenuMap);
    UGameplayStatics::OpenLevel(
        this,
        FName(MainMenuMap),
        true,
        TEXT("game=/Script/RedAlert4.RA4UIShowcaseGameMode"));
}

bool ARA4PlayerController::HasMainMenuMap() const
{
    static const FString MainMenuMap = TEXT("/Engine/Maps/Entry");
    return FPackageName::DoesPackageExist(MainMenuMap);
}

void ARA4PlayerController::UpdateCameraInput(float DeltaTime)
{
    ARA4CameraPawn* CameraPawn = GetCameraPawn();
    if (CameraPawn == nullptr)
    {
        return;
    }
    CameraController& Camera = CameraPawn->GetCameraController();

    // A is not read here: it arms attack-move. Left is panned with the arrow key,
    // the screen edge or the middle-mouse drag.
    const float Right = (IsInputKeyDown(EKeys::D) || IsInputKeyDown(EKeys::Right) ? 1.0f : 0.0f) -
                        (IsInputKeyDown(EKeys::Left) ? 1.0f : 0.0f);
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

    // Space + drag (RMB or LMB): horizontal mouse travel spins the view. Edge scrolling is
    // suppressed meanwhile, or dragging toward a border would slide the map as well
    // as turn it.
    const bool bRightMouseDown = IsInputKeyDown(EKeys::RightMouseButton);
    const bool bLeftMouseDown = IsInputKeyDown(EKeys::LeftMouseButton);
    const bool bSpaceDown = IsInputKeyDown(EKeys::SpaceBar);
    const bool bAnyMouseDown = bRightMouseDown || bLeftMouseDown || IsInputKeyDown(EKeys::MiddleMouseButton);

    // On macOS trackpads, clicking and dragging is cumbersome. 
    // Allow rotating the camera by just holding Space and moving the cursor.
    if (bSpaceDown || bRotatingCamera)
    {
        if (!bRotatingCamera)
        {
            bRotatingCamera = true;
            RotateAnchorScreen = FVector2D(MouseX, MouseY);
        }

        if (!bSpaceDown && !bAnyMouseDown)
        {
            bRotatingCamera = false;
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
        // The player is turning the camera, not driving it with the keyboard.
        Camera.SetKeyboardPan(0.0f, 0.0f);
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

void ARA4PlayerController::HandleBuildCardClicked(int64 ContentIdValue)
{
    const URA4SimWorldSubsystem* Subsystem = GetSimSubsystem();
    const SimWorld* World = Subsystem != nullptr ? Subsystem->GetSimWorld() : nullptr;
    if (World == nullptr)
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

    Command C;
    C.Type = CommandType::StartProduction;
    C.Issuer = Selection.GetLocalPlayer();
    C.Content = Content;
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
            if (URA4MatchResultOverlayWidget* MenuWidget = CreateWidget<URA4MatchResultOverlayWidget>(
                this, URA4MatchResultOverlayWidget::StaticClass()))
            {
                MenuWidget->OnRetryRequested.AddUObject(this, &ARA4PlayerController::TogglePauseMenu);
                MenuWidget->OnExitRequested.AddUObject(this, &ARA4PlayerController::HandleExitRequested);

                PauseMenuOverlay = MenuWidget;
                PauseMenuOverlay->AddToViewport(90);
            }
        }
        if (PauseMenuOverlay != nullptr)
        {
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

void ARA4PlayerController::HandleRadarClicked(FVector2D WorldPosition)
{
    if (ARA4CameraPawn* CameraPawn = GetCameraPawn())
    {
        CameraPawn->GetCameraController().FocusOn(
            Vec2f(float(WorldPosition.X), float(WorldPosition.Y)), true);
    }
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

void ARA4PlayerController::EnterDirectControl(int64 EntityIdValue)
{
    if (URA4DirectControlSubsystem* Dc = GetDirectControlSubsystem())
    {
        const uint32_t Index = static_cast<uint32_t>(EntityIdValue);
        RA4::EntityId Vehicle{Index, 0u};
        Dc->RequestEnter(this, Vehicle);
    }
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
