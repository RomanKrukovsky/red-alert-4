// Copyright (c) Red Alert 4 project. RTS player controller.
//
// Adapter, not logic. Its whole job is the part that genuinely needs the engine --
// reading keys, deprojecting the cursor onto the ground plane, projecting the
// marquee corners -- and handing the results to RA4Input, whose rules are covered
// by headless tests. Anything resembling a gameplay decision that appears in this
// file belongs in RA4Input instead.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "RA4Input/ControlScheme.h"
#include "RA4Input/HitTest.h"
#include "RA4Input/KeyBindings.h"
#include "RA4Input/OrderResolver.h"
#include "RA4Input/SelectionModel.h"

#include "RA4AudioSubsystem.h"

#include "RA4PlayerController.generated.h"

class ARA4CameraPawn;
class URA4SimWorldSubsystem;
class URA4MatchResultOverlayWidget;
class URA4DirectControlSubsystem;
class URA4DirectControlHUDViewModel;

UCLASS()
class REDALERT4_API ARA4PlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ARA4PlayerController();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void SetupInputComponent() override;
    virtual void PlayerTick(float DeltaTime) override;

    // --- read by the HUD -----------------------------------------------------
    bool IsMarqueeActive() const { return bMarqueeActive; }
    FVector2D GetMarqueeStartScreen() const { return MarqueeStartScreen; }
    FVector2D GetMarqueeCurrentScreen() const { return MarqueeCurrentScreen; }
    RA4::Input::CursorHint GetCursorHint() const { return CurrentCursorHint; }

    // Where the last move/rally order landed and when, so the HUD can ping it once
    // instead of drawing a marker that follows the cursor forever.
    bool GetMoveOrderPing(RA4::Vec2& OutLocation, double& OutIssuedSeconds) const
    {
        OutLocation = MoveOrderPingLocation;
        OutIssuedSeconds = MoveOrderPingSeconds;
        return bHasMoveOrderPing;
    }
    const RA4::Input::SelectionModel& GetSelection() const { return Selection; }

    // Armed by pressing A; the next click becomes an attack-move.
    UFUNCTION(BlueprintCallable, Category = "RA4|Input")
    void ArmAttackMove() { bAttackMoveArmed = true; }

    bool IsPlacementArmed() const { return bPlacementArmed; }
    RA4::ContentId GetPlacementContent() const { return PlacementContent; }
    bool GetCursorGroundPosition(RA4::Vec2& OutPosition) const;

    // Checks whether pointer sits over an interactive Slate UI element
    bool IsPointerOverUI() const;

    // Armed by the production sidebar once a structure has finished building.
    UFUNCTION(BlueprintCallable, Category = "RA4|Input")
    void BeginPlacement(int64 ContentIdValue);

    UFUNCTION(BlueprintCallable, Category = "RA4|Input")
    void CancelPendingAction();

    // --- ADR-0013 building controls -----------------------------------------
    // Both go out as ordinary validated commands, so they are replayed and
    // server-authoritative like any other decision. The UI never mutates the
    // simulation directly.

    /**
     * Cycles the selected building's power priority to the next band, wrapping round.
     * This is what makes a deficit a choice rather than a uniform slowdown -- the
     * player decides the radar goes dark so the factory keeps running.
     */
    UFUNCTION(BlueprintCallable, Category = "RA4|Input")
    void CycleSelectedPowerPriority();

    /** Toggles repair on the selected building. Repair spends credits while it runs. */
    UFUNCTION(BlueprintCallable, Category = "RA4|Input")
    void ToggleSelectedRepair();

    UFUNCTION(BlueprintCallable, Category = "RA4|UI")
    void TogglePauseMenu();

    // --- Direct Control / Possession Mode ------------------------------------
    UFUNCTION(BlueprintCallable, Category = "RA4|DirectControl")
    void ToggleDirectControl();

    UFUNCTION(BlueprintCallable, Category = "RA4|DirectControl")
    void EnterDirectControl(int64 EntityIdValue);

    UFUNCTION(BlueprintCallable, Category = "RA4|DirectControl")
    void ExitDirectControl();

    UFUNCTION(BlueprintCallable, Category = "RA4|DirectControl")
    bool IsDirectControlActive() const;

    // --- Cheat Console -------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "RA4|CheatConsole")
    void ToggleCheatConsole();

    bool IsCheatConsoleOpen() const { return bCheatConsoleOpen; }

protected:
    void OnDirectControlTogglePressed();
    void UpdateDirectControl(float DeltaTime);

    URA4DirectControlSubsystem* GetDirectControlSubsystem() const;
    // Drag shorter than this on the ground plane counts as a click. Measured in
    // world units rather than pixels so it means the same thing at every zoom.
    UPROPERTY(EditAnywhere, Category = "RA4|Input")
    float MarqueeMinimumExtentUnits = 55.0f;

    // Extra picking radius so small infantry remain clickable when zoomed out.
    UPROPERTY(EditAnywhere, Category = "RA4|Input")
    float PickToleranceUnits = 35.0f;

    UPROPERTY(EditAnywhere, Category = "RA4|Input")
    float DoubleClickSeconds = 0.30f;

    // Classic Red Alert by default: left button orders, right button deselects. The
    // settings screen flips this; the rules themselves live in RA4Input/ControlScheme.
    RA4::Input::ControlScheme Scheme = RA4::Input::ControlScheme::ClassicRA;

    // Degrees of yaw per pixel of horizontal drag while space + right button is held.
    UPROPERTY(EditAnywhere, Category = "RA4|Input")
    float CameraRotateDegreesPerPixel = 0.35f;

    // How long the cursor must rest on something before its name appears.
    UPROPERTY(EditAnywhere, Category = "RA4|HUD")
    float TooltipDelaySeconds = 0.45f;

private:
    // --- input handlers ------------------------------------------------------
    void OnPrimaryPressed();
    void OnPrimaryReleased();
    void OnSecondaryPressed();
    void OnSecondaryReleased();
    void OnMiddlePressed();
    void OnMiddleReleased();
    void OnZoomIn();
    void OnZoomOut();
    void OnStopPressed();
    void OnGuardPressed();
    void OnAttackMovePressed();
    void OnControlGroupKeyByKey(FKey Key);
    void OnControlGroupKey(int32 GroupIndex);

    /**
     * Dispatches one physical key press through the remappable binding table.
     *
     * Every non-mouse key routes here rather than to its own handler, so which key
     * does what is data in RA4::Input::KeyBindingTable and not a literal in
     * SetupInputComponent. Modifier state is read from the controller at dispatch
     * time so Ctrl+1 and 1 can reach the same action and be told apart there.
     */
    void OnBoundKeyPressed(FKey Key);

    /**
     * The player's binding table. Engine-free by design, so the rules that actually
     * bite - modifier precedence, conflicts, unbound actions - are unit tested
     * without an editor. Populated from the active control scheme on setup and, once
     * a settings screen exists, from the player's overrides.
     */
    RA4::Input::KeyBindingTable KeyBindings;

    /**
     * Keys that commit build cards, in the same grid order as the badges the sidebar
     * draws. One table so a badge can never promise a key that is not bound; the two
     * table lengths are asserted against each other the first time this is read.
     */
    static const TArray<FKey>& GetBuildCardHotkeys();
    void OnBuildCardKeyByKey(FKey Key);
    void OnBuildCardHotkey(int32 CardIndex);

    /**
     * Keeps the viewport strip reserved for the sidebar equal to the width the sidebar
     * actually draws. Called every tick because the first evaluation, in BeginPlay,
     * necessarily predates the viewport having a size.
     */
    void SyncSidebarReservedWidth();
#if !UE_BUILD_SHIPPING
    void DebugForceVictory();
    void DebugForceDefeat();
#endif

    // Shared by both buttons: the scheme decides which one gets here.
    void HandleClick(bool bLeftButton, const FVector2D& EndScreen, bool bWasDrag);
    void PerformSelection(const FVector2D& EndScreen, const RA4::Vec2& EndGround, bool bWasDrag);
    // Speaks the newly selected unit's line, if its faction has a recorded pack.
    void PlaySelectionVoice(const RA4::SimWorld& World);
    // Acknowledges an order, choosing the line from what the order will actually do.
    void PlayOrderVoice(const RA4::SimWorld& World, RA4::Input::CursorHint Hint);
    void PlayVoiceForPrimary(const RA4::SimWorld& World, ERA4VoiceEvent Event);

    // Name card shown once the cursor has rested on something for a moment.
    void UpdateHoverTooltip(const RA4::SimWorld& World);
    void ApplyCursorShape();

    // --- helpers -------------------------------------------------------------
    URA4SimWorldSubsystem* GetSimSubsystem() const;
    ARA4CameraPawn* GetCameraPawn() const;
    bool TryInitializeCamera();

    bool ScreenToGround(const FVector2D& ScreenPosition, RA4::Vec2& OutPosition) const;

    // Everything the local player is allowed to click on. The fog of war filter
    // belongs here once fog exists: an entity the player cannot see must never
    // enter this list, or picking becomes a maphack.
    void BuildPickCandidates(TArray<RA4::Input::PickCandidate>& OutCandidates) const;

    RA4::Input::OrderContext MakeOrderContext(const RA4::Vec2& GroundPosition) const;
    RA4::EntityId FindHoveredEntity(const RA4::Vec2& GroundPosition) const;
    void SubmitOrders(const std::vector<RA4::Command>& Commands);

    void UpdateCameraInput(float DeltaTime);
    void OnDummyPanKey();

    // The in-match HUD. Created here because the controller is what knows this is a
    // local player; the widget itself pulls everything it shows from the UI data
    // provider and never touches the simulation.
    UPROPERTY(Transient)
    TObjectPtr<class URA4ResourceBarWidget> ResourceBar;

    // The classic right-hand command column. Owned here for the same reason as the
    // resource bar; it reports card clicks back and never issues commands itself.
    UPROPERTY(Transient)
    TObjectPtr<class URA4SidebarWidget> Sidebar;

    // Width currently reserved for the sidebar in its viewport slot. Compared against
    // the widget's own computed width each tick; see SyncSidebarReservedWidth.
    float AppliedSidebarReservedWidth = 0.0f;

    void HandleBuildCardClicked(int64 ContentIdValue);
    void HandleRadarClicked(FVector2D WorldPosition);
    void BindMatchResultEvents();
    void HandleMatchEnded(bool bLocalPlayerWon, int32 WinningPlayer);
    void HandleRetryRequested();
    void HandleExitRequested();
    bool HasMainMenuMap() const;

    // --- state ---------------------------------------------------------------
    RA4::Input::SelectionModel Selection;
    RA4::Input::CursorHint CurrentCursorHint = RA4::Input::CursorHint::Select;

    bool bMarqueeActive = false;
    // A press that starts over the HUD belongs to Slate even if the low-level key
    // binding also sees it. Remember it through release so no selection or order can
    // leak into the world under a button.
    bool bPrimaryConsumedByUI = false;
    FVector2D MarqueeStartScreen = FVector2D::ZeroVector;
    FVector2D MarqueeCurrentScreen = FVector2D::ZeroVector;
    RA4::Vec2 MarqueeStartGround;

    bool bAttackMoveArmed = false;
    bool bPlacementArmed = false;
    RA4::ContentId PlacementContent;

    // Space + right-drag spins the view. Space is the modifier so the right button
    // keeps its normal job (deselect, in the classic scheme) when pressed alone.
    bool bRotatingCamera = false;
    FVector2D RotateAnchorScreen = FVector2D::ZeroVector;

    RA4::Vec2 MoveOrderPingLocation;
    double MoveOrderPingSeconds = 0.0;
    bool bHasMoveOrderPing = false;

    double LastPrimaryClickTime = -1.0;
    RA4::EntityId LastClickedEntity;
    bool bCameraInitialized = false;
    bool bInitialCursorPositionSet = false;
    bool bMatchResultEventsBound = false;
    bool bMatchResultVisible = false;
    int32 EdgeScrollWarmupFrames = 0;

    UPROPERTY(Transient)
    TObjectPtr<URA4MatchResultOverlayWidget> MatchResultOverlay;

    bool bDirectControlActive = false;
    RA4::EntityId DirectControlEntityId{};
    FRotator DirectControlCameraRotation = FRotator::ZeroRotator;

    UPROPERTY(Transient)
    TObjectPtr<class URA4CheatConsoleWidget> CheatConsoleOverlay;
    bool bCheatConsoleOpen = false;

    UPROPERTY(Transient)
    TObjectPtr<class UUserWidget> PauseMenuOverlay;

    UPROPERTY(Transient)
    TObjectPtr<class URA4HoverTooltipWidget> HoverTooltip;

    // First-person direct-control combat HUD ViewModel. Created on Enter,
    // destroyed on Exit. The Blueprint widget that renders it is loaded from
    // the active DirectControlProfile's HudWidgetClass soft reference.
    UPROPERTY(Transient)
    TObjectPtr<URA4DirectControlHUDViewModel> DirectControlHUDViewModel;

    // Which entity the cursor is resting on, and since when. The delay is what stops
    // the card flickering while the pointer sweeps across a crowded battlefield.
    RA4::EntityId HoveredEntity;
    double HoverStartedSeconds = 0.0;
    bool bTooltipVisible = false;

    FDelegateHandle MatchEndedHandle;
};
