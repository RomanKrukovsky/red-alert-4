// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "RA4HUDOcclusion.h"
#include "RA4HUDTypes.h"
#include "RA4ScreenRootWidget.h"
#include "RA4HUDWidget.generated.h"

class UProgressBar;
class UButton;
class UCanvasPanel;
class UTextBlock;
class UUniformGridPanel;
class UVerticalBox;
class URA4HUDViewModel;

// ---------------------------------------------------------
// In-Game HUD Screens (C++ logic bindings for Widget Blueprints)
// ---------------------------------------------------------

/** Event-driven RTS HUD shell shared by all faction variants. */
UCLASS(BlueprintType, Blueprintable)
class RA4UI_API URA4HUDWidget : public URA4ScreenRootWidget
{
    GENERATED_BODY()

public:
    URA4HUDWidget(const FObjectInitializer& ObjectInitializer);

    UFUNCTION(BlueprintCallable, Category = "RA4|HUD")
    void ConfigureHUD(
        ERA4FactionTheme InFactionTheme,
        ERA4UIScreenVariant InVariant = ERA4UIScreenVariant::Default,
        int32 InActiveProductionTab = 0);

    UFUNCTION(BlueprintPure, Category = "RA4|HUD")
    ERA4FactionTheme GetFactionTheme() const { return FactionTheme; }

    UFUNCTION(BlueprintPure, Category = "RA4|HUD")
    ERA4UIScreenVariant GetHUDVariant() const { return HUDVariant; }

    UFUNCTION(BlueprintPure, Category = "RA4|HUD")
    int32 GetActiveProductionTab() const { return ActiveProductionTab; }

    UFUNCTION(BlueprintCallable, Category = "RA4|HUD")
    void SetHUDViewModel(URA4HUDViewModel* InViewModel);

    UFUNCTION(BlueprintPure, Category = "RA4|HUD")
    URA4HUDViewModel* GetHUDViewModel() const { return HUDViewModel; }

    UFUNCTION(BlueprintPure, Category = "RA4|HUD|Input")
    bool IsWorldInputBlockedAtReferencePoint(FVector2D Point) const;

    UFUNCTION(BlueprintPure, Category = "RA4|HUD|Input")
    int32 GetInteractiveRegionCount() const { return Occlusion.Num(); }

    /**
     * Share of the reference canvas the player can still see the battle through,
     * with overlapping panels counted once. The design budget is 65-72%: below
     * that the HUD is swallowing the fight, above it the panels are too thin to
     * read.
     */
    UFUNCTION(BlueprintPure, Category = "RA4|HUD|Layout")
    float GetBattlefieldViewFraction() const;

    /** Canvas the HUD panels are authored against. See FRA4HUDOcclusion. */
    static constexpr float ReferenceCanvasWidth = FRA4HUDOcclusion::ReferenceCanvasWidth;
    static constexpr float ReferenceCanvasHeight = FRA4HUDOcclusion::ReferenceCanvasHeight;

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    void ApplyShowcaseSnapshot();
    void HandleHUDChanged(ERA4HUDChangeFlags Changes);
    void RefreshResources();
    void RefreshSelection();
    void RefreshProduction();
    void RefreshObjectives();
    void RefreshAlerts();
    void AddInteractiveRegion(FVector2D Position, FVector2D Size);

    UFUNCTION()
    void CycleProductionTab();

    UFUNCTION()
    void QueueSelectedProduction();

    UFUNCTION()
    void IssuePrimaryCommand();

    UPROPERTY(Transient)
    TObjectPtr<URA4HUDViewModel> HUDViewModel;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> ResourceText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> SelectionTitleText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> SelectionDetailText;

    UPROPERTY(Transient)
    TObjectPtr<UProgressBar> SelectionArmourBar;

    UPROPERTY(Transient)
    TObjectPtr<UProgressBar> SelectionHealthBar;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> AlertText;

    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> CommandStatusText;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> ObjectivesList;

    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> ProductionQueueList;

    UPROPERTY(Transient)
    TObjectPtr<UUniformGridPanel> BuildGrid;

    FRA4HUDOcclusion Occlusion;
    ERA4FactionTheme FactionTheme = ERA4FactionTheme::EurasianPact;
    ERA4UIScreenVariant HUDVariant = ERA4UIScreenVariant::Default;
    int32 ActiveProductionTab = 0;
};

UCLASS(Abstract)
class RA4UI_API URA4SovietHUDWidget : public URA4HUDWidget
{
    GENERATED_BODY()
};

UCLASS(Abstract)
class RA4UI_API URA4AlliesHUDWidget : public URA4HUDWidget
{
    GENERATED_BODY()
};

UCLASS(Abstract)
class RA4UI_API URA4EasternHUDWidget : public URA4HUDWidget
{
    GENERATED_BODY()
};

UCLASS(Abstract)
class RA4UI_API URA4ChronoHUDWidget : public URA4HUDWidget
{
    GENERATED_BODY()
};

/** Production queue container. */
UCLASS(Abstract)
class RA4UI_API URA4ProductionQueueWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};

/** Resource counters, power state, unit count and match clock. Built in C++ so the
 *  layout lives in version control rather than inside a binary asset. */
UCLASS(Blueprintable)
class RA4UI_API URA4ResourceBarWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()

public:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

protected:
    /** Rebuilds every field from the provider. Called on change, never on tick. */
    void Refresh();

    class URA4UIDataProviderSubsystem* GetProvider() const;

    UPROPERTY(Transient)
    TObjectPtr<class UTextBlock> CreditsValue;

    UPROPERTY(Transient)
    TObjectPtr<class UTextBlock> PowerValue;

    UPROPERTY(Transient)
    TObjectPtr<class UTextBlock> SupplyValue;

    UPROPERTY(Transient)
    TObjectPtr<class UTextBlock> TimerValue;

private:
    FDelegateHandle ResourceChangeHandle;
};

/** Category tabs above the unit and structure production grid. */
UCLASS(Abstract, Blueprintable)
class RA4UI_API URA4ProductionTabsWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};

/** One production item, including cost, prerequisites, progress, and disabled state. */
UCLASS(Abstract, Blueprintable)
class RA4UI_API URA4ProductionCardWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};

/** Selected unit portrait, health, veterancy, and contextual information. */
UCLASS(Abstract, Blueprintable)
class RA4UI_API URA4SelectionPanelWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};

/** Contextual unit orders and stance controls. */
UCLASS(Abstract, Blueprintable)
class RA4UI_API URA4CommandGridWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};

/** Timed tactical messages and economy warnings. Built in C++ like the resource
 *  bar: the feed consumes the provider's fog-filtered alert list and renders the
 *  SC-20 reference's EVA block (severity-coloured rows with repeat counters). */
UCLASS(Blueprintable)
class RA4UI_API URA4NotificationFeedWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()

public:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

protected:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    class URA4UIDataProviderSubsystem* GetProvider() const;

    /** Rebuilds alert rows from the provider. Called on change, never on tick. */
    void Refresh();

    void TriggerAlertClick(int32 Index);

    UFUNCTION()
    void OnAlert0Clicked();
    UFUNCTION()
    void OnAlert1Clicked();
    UFUNCTION()
    void OnAlert2Clicked();
    UFUNCTION()
    void OnAlert3Clicked();
    UFUNCTION()
    void OnAlert4Clicked();

    UPROPERTY(Transient)
    TObjectPtr<class UVerticalBox> FeedBox;

    TArray<FVector2D> AlertLocations;

    FDelegateHandle AlertsChangeHandle;

    // Seconds since the newest alert arrived; drives the attention flash.
    float NewestAlertAge = 0.0f;
    bool bFlashActive = false;
};

/** Current primary, secondary, and optional mission objectives. */
UCLASS(Abstract, Blueprintable)
class RA4UI_API URA4ObjectivesWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};

/** Full-width animated EVA alert overlay. */
UCLASS(Abstract, Blueprintable)
class RA4UI_API URA4EVAAlertWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};

/** In-Game Pause menu. */
UCLASS(Abstract)
class RA4UI_API URA4PauseWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};

/** In-Game Victory / Defeat screen. */
UCLASS(Abstract)
class RA4UI_API URA4MatchResultWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
};
