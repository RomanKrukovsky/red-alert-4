// Copyright (c) Red Alert 4 project. The one path game data takes into the HUD.
//
// The simulation produces a HudSnapshot once per fixed tick; this subsystem converts
// it into Blueprint types and pushes the result into the view models. Widgets bind to
// the view models and never touch the simulation, so there is no per-frame polling,
// no GetAllActorsOfClass and no Cast chains anywhere in the UI.
//
// Direction of dependency: RedAlert4 -> RA4UI -> RA4Presentation -> RA4Simulation.
// The push comes from RedAlert4 (which owns the world), so RA4UI never needs to know
// that a SimWorld subsystem exists and no cycle forms.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "RA4HUDTypes.h"

#include "RA4UIDataProviderSubsystem.generated.h"

namespace RA4 { namespace Presentation { struct HudSnapshot; } }

class URA4HUDViewModel;

UCLASS()
class RA4UI_API URA4UIDataProviderSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /**
     * Called once per simulation tick by the module that owns the world. Diffs the
     * snapshot against the previous one and only writes fields that changed, so an
     * idle match produces no view model traffic and Slate is not invalidated.
     */
    void ApplySnapshot(const RA4::Presentation::HudSnapshot& Snapshot);

    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    URA4HUDViewModel* GetHUDViewModel() const { return HUDViewModel; }

    // --- read by widgets that need lists rather than scalars ------------------
    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    const TArray<FRA4SelectionGroup>& GetSelectionGroups() const { return SelectionGroups; }

    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    const TArray<FRA4ProductionEntry>& GetProductionQueue() const { return ProductionQueue; }

    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    const TArray<FRA4BuildOption>& GetBuildOptions() const { return BuildOptions; }

    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    const TArray<FRA4Alert>& GetAlerts() const { return Alerts; }

    /** Build options filtered to one sidebar tab. */
    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    TArray<FRA4BuildOption> GetBuildOptionsForCategory(int32 Category) const;

    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    ERA4SelectionKind GetSelectionKind() const { return SelectionKind; }

    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    ERA4MatchPhase GetMatchPhase() const { return MatchPhase; }

    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    int32 GetMatchElapsedSeconds() const { return MatchElapsedSeconds; }

    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    int32 GetWinningPlayer() const { return WinningPlayer; }

    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    bool IsLocalPlayerDefeated() const { return bLocalPlayerDefeated; }

    /** Fired when the match ends, so the victory or defeat screen can be pushed. */
    DECLARE_MULTICAST_DELEGATE_TwoParams(FOnMatchEnded, bool /*bLocalPlayerWon*/, int32 /*WinningPlayer*/);
    FOnMatchEnded OnMatchEnded;

    /** Fired when the alert list changes, so the feed animates only on real news. */
    DECLARE_MULTICAST_DELEGATE(FOnAlertsChanged);
    FOnAlertsChanged OnAlertsChanged;

    /**
     * Fired only when a displayed resource value actually changed. Widgets refresh
     * on this instead of ticking: an idle base produces no UI work at all.
     */
    DECLARE_MULTICAST_DELEGATE(FOnResourcesChanged);
    FOnResourcesChanged OnResourcesChanged;

    /**
     * Fired when the build cards or the queue changed in a way the sidebar shows.
     * Queue progress ticks constantly, so this deliberately compares the rounded
     * percentage the widget displays rather than the raw value: a bar that only
     * redraws when it visibly moves costs nothing while a factory runs.
     */
    DECLARE_MULTICAST_DELEGATE(FOnProductionChanged);
    FOnProductionChanged OnProductionChanged;

    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    int32 GetCredits() const { return Credits; }

    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    int32 GetPowerProduced() const { return PowerProduced; }

    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    int32 GetPowerConsumed() const { return PowerConsumed; }

    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    bool IsPowerShortage() const { return bPowerShortage; }

    /** Units fielded. Meaningless unless IsSupplyModelled() is true. */
    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    int32 GetSupplyUsed() const { return SupplyUsed; }

    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    int32 GetSupplyCap() const { return SupplyCap; }

    /** False while the simulation has no population cap: the counter must be hidden. */
    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    bool IsSupplyModelled() const { return bSupplyModelled; }

private:
    bool HasVisibleProductionChange(const TArray<FRA4ProductionEntry>& PreviousQueue,
                                    const TArray<FRA4BuildOption>& PreviousOptions) const;

    UPROPERTY(Transient)
    TObjectPtr<URA4HUDViewModel> HUDViewModel;

    UPROPERTY(Transient)
    TArray<FRA4SelectionGroup> SelectionGroups;

    UPROPERTY(Transient)
    TArray<FRA4ProductionEntry> ProductionQueue;

    UPROPERTY(Transient)
    TArray<FRA4BuildOption> BuildOptions;

    UPROPERTY(Transient)
    TArray<FRA4Alert> Alerts;

    int32 Credits = 0;
    int32 PowerProduced = 0;
    int32 PowerConsumed = 0;
    bool bPowerShortage = false;
    int32 SupplyUsed = 0;
    int32 SupplyCap = 0;
    bool bSupplyModelled = false;

    ERA4SelectionKind SelectionKind = ERA4SelectionKind::Empty;
    ERA4MatchPhase MatchPhase = ERA4MatchPhase::NotStarted;
    int32 MatchElapsedSeconds = 0;
    int32 WinningPlayer = -1;
    bool bLocalPlayerDefeated = false;
    bool bReportedMatchEnd = false;
};
