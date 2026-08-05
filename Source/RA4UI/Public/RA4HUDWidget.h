// Copyright (c) Red Alert 4 project.

#pragma once

#include "CoreMinimal.h"
#include "RA4ActivatableWidget.h"
#include "RA4HUDWidget.generated.h"

// ---------------------------------------------------------
// In-Game HUD Screens (C++ logic bindings for Widget Blueprints)
// ---------------------------------------------------------

/** Base class for the main in-game HUD covering the whole screen. */
UCLASS(Abstract)
class RA4UI_API URA4HUDWidget : public URA4ActivatableWidget
{
    GENERATED_BODY()
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

/** Container for the minimap rendering. */
UCLASS(Abstract)
class RA4UI_API URA4MinimapWidget : public URA4ActivatableWidget
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

    UPROPERTY(Transient)
    TObjectPtr<class UVerticalBox> FeedBox;

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
