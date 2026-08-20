// Copyright (c) Red Alert 4 project. The classic right-hand command sidebar.
//
// Red Alert never had a bottom command bar: the whole base interface is a column on
// the right, and players navigate it by muscle memory -- minimap at the top, the
// resource readout under it, then category tabs and a grid of build cards. This
// widget reproduces that layout.
//
// Built in C++ rather than authored as a Blueprint asset for the same reason as the
// resource bar: the game has to be playable from a fresh clone with no editor-made
// assets, and a layout in a .uasset cannot be reviewed in a diff.
//
// Data comes from URA4UIDataProviderSubsystem, which is fed by HudSnapshot. This
// widget never reaches into the simulation.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"

#include "RA4HUDTypes.h"

#include "RA4SidebarWidget.generated.h"

class UProgressBar;
class USizeBox;
class UTextBlock;
class UVerticalBox;
class UUniformGridPanel;
class URA4UIDataProviderSubsystem;
class SRA4RadarSlate;

DECLARE_MULTICAST_DELEGATE_OneParam(FRA4OnRadarClicked, FVector2D /*WorldPosition*/);

/** Lightweight Slate-painted radar. It only reads the fog-filtered HUD provider. */
UCLASS()
class RA4UI_API URA4RadarWidget : public UWidget
{
    GENERATED_BODY()

public:
    FRA4OnRadarClicked OnRadarClicked;
    /**
     * A right-click on the panel, in world units. Carried separately from OnRadarClicked
     * because the two mean opposite things: the left button moves the camera, the right
     * button orders the selection somewhere. Collapsing them would make every camera jump
     * also send an army across the map.
     */
    FRA4OnRadarClicked OnRadarOrdered;

    const TArray<FRA4RadarMarker>& GetMarkers() const;
    FVector2D GetMapSize() const;
    int32 GetLocalPlayer() const;
    /** False when a power deficit has taken the radar: the panel draws dark instead. */
    bool IsOnline() const;
    /** Terrain and shroud grids for the background, and how many cells they hold. */
    const TArray<uint8>& GetBackgroundTerrain() const;
    const TArray<uint8>& GetBackgroundShroud() const;
    FIntPoint GetBackgroundCellCounts() const;
    /** Transient alert markers, brightest first. */
    const TArray<FRA4RadarPing>& GetPings() const;
    void HandleSlateClick(const FVector2D& NormalizedPosition);
    /** Right-click equivalent: resolves to world units and fires OnRadarOrdered. */
    void HandleSlateOrder(const FVector2D& NormalizedPosition);

    /**
     * The camera's current view rectangle in world units, so the panel can outline what part
     * of the map is on screen. Without it the player can see where their forces are but not
     * where they themselves are looking, which is what makes the minimap navigable rather
     * than merely informative.
     *
     * Zero extent means unknown, and the frame is not drawn.
     */
    void SetCameraView(const FVector2D& CentreWorld, const FVector2D& ExtentWorld);
    FVector2D GetCameraViewCentre() const { return CameraViewCentre; }
    FVector2D GetCameraViewExtent() const { return CameraViewExtent; }

    /**
     * The rectangle inside a square radar panel that the map actually occupies, letterboxed
     * to preserve the map's aspect ratio. A 128x64 map drawn to fill a square panel is
     * stretched 2:1 vertically, which puts every marker in the wrong place relative to the
     * terrain a player is looking at.
     *
     * Returned as offset plus size in panel-local pixels. Both the painter and the click
     * handler go through this, so a click cannot land somewhere other than where the marker
     * under the cursor was drawn.
     */
    static void ComputeMapRect(const FVector2D& PanelSize, const FVector2D& MapSize,
                               FVector2D& OutOffset, FVector2D& OutSize);

    virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;

private:
    URA4UIDataProviderSubsystem* GetProvider() const;

    TSharedPtr<SRA4RadarSlate> RadarSlate;

    FVector2D CameraViewCentre = FVector2D::ZeroVector;
    FVector2D CameraViewExtent = FVector2D::ZeroVector;
};

/**
 * UButton::OnClicked carries no payload, so a grid of build cards cannot tell which
 * card was pressed. Rather than searching for the hovered widget -- which guesses
 * wrong the moment the pointer moves between press and release -- each button knows
 * its own index.
 */
UCLASS()
class RA4UI_API URA4IndexedButton : public UButton
{
    GENERATED_BODY()

public:
    DECLARE_MULTICAST_DELEGATE_OneParam(FOnIndexedClicked, int32 /*Index*/);
    FOnIndexedClicked OnIndexedClicked;

    void SetIndex(int32 InIndex) { Index = InIndex; }
    int32 GetIndex() const { return Index; }

    /** Binds this button's own OnClicked to the forwarding handler. Call once. */
    void BindForwarding();

private:
    UFUNCTION()
    void HandleClicked();

    int32 Index = INDEX_NONE;
};

/**
 * Issued when the player commits a card. The controller owns command submission, so
 * the sidebar reports intent and does not talk to the simulation itself.
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FRA4OnBuildCardClicked, int64 /*ContentId*/);

UCLASS()
class RA4UI_API URA4SidebarWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /**
     * Reference column width in slate units, measured against a 1080p-tall viewport.
     * The sidebar is a fixed slice of screen by design -- players reach for cards by
     * muscle memory, so it must not reflow as the queue grows -- but a slice sized for
     * 1080p eats a third of a small window and shrinks to a sliver on a 4K panel.
     * ComputeSidebarWidth applies a clamped scale so the column keeps the same visual
     * weight at any resolution.
     */
    static constexpr float SidebarWidth = 232.0f;

    /** Scale factor for the viewport the given object lives in. Never returns 0. */
    static float ComputeSidebarScale(const UObject* WorldContextObject);

    /**
     * Actual column width for this viewport. The player controller reserves the same
     * width in its viewport slot, so both sides must go through this one function or
     * the world will be drawn under the sidebar.
     */
    static float ComputeSidebarWidth(const UObject* WorldContextObject);

    /** How many build cards can be reached from the keyboard. */
    static int32 GetCardHotkeyCount();

    /**
     * The key that activates the card at the given grid index, as shown on the card's
     * badge. One table serves both the label and the binding, so a badge can never
     * advertise a key that does nothing. Returns nullptr past the end of the table.
     */
    static const TCHAR* GetCardHotkeyLabel(int32 CardIndex);

    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    FRA4OnBuildCardClicked OnBuildCardClicked;
    FRA4OnRadarClicked OnRadarClicked;
    /** Forwarded from the radar panel's right button: an order, not a camera move. */
    FRA4OnRadarClicked OnRadarOrdered;

    /** Forwards the camera's ground footprint to the radar panel, which outlines it. */
    void SetRadarCameraView(const FVector2D& CentreWorld, const FVector2D& ExtentWorld);

    /** Which sidebar tab is showing. Values match ProductionCategory. */
    UFUNCTION(BlueprintCallable, Category = "RA4|UI")
    void SetActiveCategory(int32 Category);

    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    int32 GetActiveCategory() const { return ActiveCategory; }

    /**
     * Commits the build card at the given grid index (row-major, matching the hotkey
     * badges). Broadcasts OnBuildCardClicked when the card exists and is available;
     * returns whether it did, so a caller can fall through to another binding.
     */
    UFUNCTION(BlueprintCallable, Category = "RA4|UI")
    bool ActivateCardByIndex(int32 CardIndex);

    /** Number of cards currently visible in the active category. */
    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    int32 GetVisibleCardCount() const { return CardContentIds.Num(); }

private:
    URA4UIDataProviderSubsystem* GetProvider() const;

    void RefreshResources();
    void RefreshCards();
    void RefreshQueue();
    void RefreshSelection();

    void HandleTabClicked(int32 TabIndex);
    void HandleCardClicked(int32 CardIndex);
    void HandleRadarClicked(FVector2D WorldPosition);
    void HandleRadarOrdered(FVector2D WorldPosition);

    /**
     * Drives the hover swell on the cards and follows viewport resizes. Both are
     * frame-rate concerns rather than simulation state, which is why they live here
     * and not on the provider's change delegates.
     */
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // Widgets rebuilt on refresh rather than kept in sync one by one: the card grid is
    // at most a couple of dozen entries and only changes when availability does.
    UPROPERTY(Transient)
    TObjectPtr<UUniformGridPanel> CardGrid;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> QueueBox;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> QueueHeader;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> CreditsText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> PowerText;

    // Reads "+40 SPARE" or "-15 DEFICIT": the number that decides whether the next
    // structure can be powered, which the produced/consumed pair only implies.
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> PowerSurplusText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> SelectionKindText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> SelectionNameText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> SelectionCountText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> SelectionHealthText;

    UPROPERTY(Transient)
    TObjectPtr<UProgressBar> SelectionHealthBar;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> SelectionDetailsText;

    // One row per unit type in a multi-selection, as the reference HUD groups them.
    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> SelectionGroupBox;

    // Power headroom at a glance, under the produced/consumed line.
    UPROPERTY(Transient)
    TObjectPtr<UProgressBar> PowerRatioBar;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> SupplyText;

    // The scaling wrapper, kept so a resized window can be followed without rebuilding
    // the widget tree.
    UPROPERTY(Transient)
    TObjectPtr<USizeBox> WidthBox;

    UPROPERTY(Transient)
    TObjectPtr<URA4RadarWidget> RadarWidget;

    UPROPERTY(Transient)
    TArray<TObjectPtr<URA4IndexedButton>> TabButtons;

    // Parallel to the buttons in CardGrid, so a click can be resolved back to content
    // without storing state on the button itself.
    UPROPERTY(Transient)
    TArray<TObjectPtr<URA4IndexedButton>> CardButtons;

    // Parallel to CardButtons: the widget that carries the hover swell, since a
    // UButton's own render transform is overwritten by its style states.
    UPROPERTY(Transient)
    TArray<TObjectPtr<UWidget>> CardHoverTargets;

    TArray<int64> CardContentIds;

    int32 ActiveCategory = 0;

    // What the card grid was last built from. See RefreshCards.
    uint32 CardsSignature = 0;

    // What the queue box was last built from, for the same reason. See RefreshQueue.
    uint32 QueueSignature = 0;

    // Per-card hover swell, 0 idle to 1 hovered. Eased in NativeTick so the grid reads
    // as physical rather than snapping between two states.
    TArray<float> CardHoverProgress;

    // Last width pushed into WidthBox, so the override is only written when the
    // viewport actually changed size.
    float AppliedSidebarWidth = 0.0f;

    FDelegateHandle ResourceChangeHandle;
    FDelegateHandle ProductionChangeHandle;
    FDelegateHandle SelectionChangeHandle;
};
