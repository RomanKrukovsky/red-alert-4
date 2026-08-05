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

    const TArray<FRA4RadarMarker>& GetMarkers() const;
    FVector2D GetMapSize() const;
    int32 GetLocalPlayer() const;
    void HandleSlateClick(const FVector2D& NormalizedPosition);

    virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;

private:
    URA4UIDataProviderSubsystem* GetProvider() const;

    TSharedPtr<SRA4RadarSlate> RadarSlate;
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
    /** Fixed column width in slate units; the viewport slot needs it to size itself. */
    static constexpr float SidebarWidth = 232.0f;

    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    FRA4OnBuildCardClicked OnBuildCardClicked;
    FRA4OnRadarClicked OnRadarClicked;

    /** Which sidebar tab is showing. Values match ProductionCategory. */
    UFUNCTION(BlueprintCallable, Category = "RA4|UI")
    void SetActiveCategory(int32 Category);

    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    int32 GetActiveCategory() const { return ActiveCategory; }

    /**
     * Activates the build card at the given grid index (row-major, same as hotkey
     * labels). If the card exists and is available, broadcasts OnBuildCardClicked.
     * Returns true if the card was activated.
     */
    bool ActivateCardByIndex(int32 CardIndex);

    /** Number of cards currently visible in the active category. */
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

    /** Hotkey handler called by the player controller when a card hotkey is pressed. */
    void OnBuildCardHotkey(int32 HotkeyIndex);

    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // Widgets rebuilt on refresh rather than kept in sync one by one: the card grid is
    // at most a couple of dozen entries and only changes when availability does.
    UPROPERTY(Transient)
    TObjectPtr<UUniformGridPanel> CardGrid;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> QueueBox;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> CreditsText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> PowerText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> SelectionNameText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> SelectionHealthText;

    UPROPERTY(Transient)
    TObjectPtr<UProgressBar> SelectionHealthBar;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> SelectionDetailsText;

    // Power ratio bar shown under the power text in the resource section.
    UPROPERTY(Transient)
    TObjectPtr<UProgressBar> PowerRatioBar;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> SupplyText;

    UPROPERTY(Transient)
    TObjectPtr<URA4RadarWidget> RadarWidget;

    UPROPERTY(Transient)
    TArray<TObjectPtr<URA4IndexedButton>> TabButtons;

    // Parallel to the buttons in CardGrid, so a click can be resolved back to content
    // without storing state on the button itself.
    UPROPERTY(Transient)
    TArray<TObjectPtr<URA4IndexedButton>> CardButtons;

    TArray<int64> CardContentIds;

    int32 ActiveCategory = 0;

    // What the card grid was last built from. See RefreshCards.
    uint32 CardsSignature = 0;

    // Per-card hover progress for animated scale (0 = idle, 1 = hovered).
    TArray<float> CardHoverProgress;

    FDelegateHandle ResourceChangeHandle;
    FDelegateHandle ProductionChangeHandle;
    FDelegateHandle SelectionChangeHandle;
};
