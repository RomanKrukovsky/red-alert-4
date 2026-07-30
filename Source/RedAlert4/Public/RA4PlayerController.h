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
#include "RA4Input/OrderResolver.h"
#include "RA4Input/SelectionModel.h"

#include "RA4PlayerController.generated.h"

class ARA4CameraPawn;
class URA4SimWorldSubsystem;

UCLASS()
class REDALERT4_API ARA4PlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ARA4PlayerController();

    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void SetupInputComponent() override;
    virtual void PlayerTick(float DeltaTime) override;

    // --- read by the HUD -----------------------------------------------------
    bool IsMarqueeActive() const { return bMarqueeActive; }
    FVector2D GetMarqueeStartScreen() const { return MarqueeStartScreen; }
    FVector2D GetMarqueeCurrentScreen() const { return MarqueeCurrentScreen; }
    RA4::Input::CursorHint GetCursorHint() const { return CurrentCursorHint; }
    const RA4::Input::SelectionModel& GetSelection() const { return Selection; }

    // Armed by pressing A; the next click becomes an attack-move.
    UFUNCTION(BlueprintCallable, Category = "RA4|Input")
    void ArmAttackMove() { bAttackMoveArmed = true; }

    // Armed by the production sidebar once a structure has finished building.
    UFUNCTION(BlueprintCallable, Category = "RA4|Input")
    void BeginPlacement(int64 ContentIdValue);

    UFUNCTION(BlueprintCallable, Category = "RA4|Input")
    void CancelPendingAction();

protected:
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

private:
    // --- input handlers ------------------------------------------------------
    void OnPrimaryPressed();
    void OnPrimaryReleased();
    void OnSecondaryPressed();
    void OnMiddlePressed();
    void OnMiddleReleased();
    void OnZoomIn();
    void OnZoomOut();
    void OnStopPressed();
    void OnGuardPressed();
    void OnControlGroupKeyByKey(FKey Key);
    void OnControlGroupKey(int32 GroupIndex);

    // Shared by both buttons: the scheme decides which one gets here.
    void HandleClick(bool bLeftButton, const FVector2D& EndScreen, bool bWasDrag);
    void PerformSelection(const FVector2D& EndScreen, const RA4::Vec2& EndGround, bool bWasDrag);
    void ApplyCursorShape();

    // --- helpers -------------------------------------------------------------
    URA4SimWorldSubsystem* GetSimSubsystem() const;
    ARA4CameraPawn* GetCameraPawn() const;
    bool TryInitializeCamera();

    bool GetCursorGroundPosition(RA4::Vec2& OutPosition) const;
    bool ScreenToGround(const FVector2D& ScreenPosition, RA4::Vec2& OutPosition) const;

    // Everything the local player is allowed to click on. The fog of war filter
    // belongs here once fog exists: an entity the player cannot see must never
    // enter this list, or picking becomes a maphack.
    void BuildPickCandidates(TArray<RA4::Input::PickCandidate>& OutCandidates) const;

    RA4::Input::OrderContext MakeOrderContext(const RA4::Vec2& GroundPosition) const;
    RA4::EntityId FindHoveredEntity(const RA4::Vec2& GroundPosition) const;
    void SubmitOrders(const std::vector<RA4::Command>& Commands);

    void UpdateCameraInput(float DeltaTime);

    // The in-match HUD. Created here because the controller is what knows this is a
    // local player; the widget itself pulls everything it shows from the UI data
    // provider and never touches the simulation.
    UPROPERTY(Transient)
    TObjectPtr<class URA4ResourceBarWidget> ResourceBar;

    // --- state ---------------------------------------------------------------
    RA4::Input::SelectionModel Selection;
    RA4::Input::CursorHint CurrentCursorHint = RA4::Input::CursorHint::Select;

    bool bMarqueeActive = false;
    FVector2D MarqueeStartScreen = FVector2D::ZeroVector;
    FVector2D MarqueeCurrentScreen = FVector2D::ZeroVector;
    RA4::Vec2 MarqueeStartGround;

    bool bAttackMoveArmed = false;
    bool bPlacementArmed = false;
    RA4::ContentId PlacementContent;

    double LastPrimaryClickTime = -1.0;
    RA4::EntityId LastClickedEntity;
    bool bCameraInitialized = false;
    bool bInitialCursorPositionSet = false;
    int32 EdgeScrollWarmupFrames = 0;
};
