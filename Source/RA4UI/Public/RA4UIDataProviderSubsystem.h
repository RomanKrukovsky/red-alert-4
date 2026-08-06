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

    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    const TArray<FRA4RadarMarker>& GetRadarMarkers() const { return RadarMarkers; }

    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    FVector2D GetRadarMapSize() const { return RadarMapSize; }

    /** False when a power deficit has taken the player's radar, so the panel draws dark. */
    UFUNCTION(BlueprintPure, Category = "RA4|Radar")
    bool IsRadarOnline() const { return bRadarOnline; }

    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    int32 GetRadarLocalPlayer() const { return RadarLocalPlayer; }

    /**
     * The minimap terrain/shroud background, as a row-major grid of at most
     * kMinimapMaxCellsPerAxis cells per axis. Terrain values are ERA4MinimapTerrain and
     * shroud values are ERA4MinimapShroud, stored as bytes so a painter can index them
     * directly without a per-cell conversion.
     *
     * Copied only when it changed, so an explored map costs nothing per tick.
     */
    const TArray<uint8>& GetMinimapTerrain() const { return MinimapTerrain; }
    const TArray<uint8>& GetMinimapShroud() const { return MinimapShroud; }

    UFUNCTION(BlueprintPure, Category = "RA4|Radar")
    FIntPoint GetMinimapCellCounts() const { return MinimapCellCounts; }

    /** Bumped whenever the background changes, so a cached texture knows to re-upload. */
    UFUNCTION(BlueprintPure, Category = "RA4|Radar")
    int32 GetMinimapBackgroundRevision() const { return MinimapBackgroundRevision; }

    /**
     * True while this provider has never been handed a minimap grid. The snapshot only carries
     * the grid on the ticks it changed, so a provider created after the map was already
     * explored has nothing to draw; the owner answers this by requesting a resend.
     */
    bool NeedsMinimapBackground() const { return MinimapCellCounts.X <= 0 || MinimapCellCounts.Y <= 0; }

    /** Transient alert markers, brightest first. Empty when nothing has just happened. */
    UFUNCTION(BlueprintPure, Category = "RA4|Radar")
    const TArray<FRA4RadarPing>& GetRadarPings() const { return RadarPings; }

    /** Resolves the stable simulation display key through the HUD localization map. */
    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    FText GetDisplayNameForKey(const FString& Key) const;

    /** Build options filtered to one sidebar tab. */
    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    TArray<FRA4BuildOption> GetBuildOptionsForCategory(int32 Category) const;

    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    ERA4SelectionKind GetSelectionKind() const { return SelectionKind; }

    // --- ADR-0013 building controls -----------------------------------------
    // The simulation has carried power priority and repair state for a while, but
    // neither reached Blueprints, so the mechanics existed and no player could see or
    // use them. These expose the selected building's state; the actual changes go out
    // as ordinary validated commands, never by mutating the simulation from here.

    /** True when the selection's primary is a building, so the card can show its controls. */
    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    bool IsSelectionABuilding() const { return bSelectionIsBuilding; }

    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    ERA4PowerPriority GetSelectionPowerPriority() const { return SelectionPowerPriority; }

    /** True when this building's priority band is offline at the current power tier. */
    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    bool IsSelectionPowerOffline() const { return bSelectionPowerOffline; }

    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    bool IsSelectionRepairing() const { return bSelectionRepairing; }

    /** True only for an owned, finished, damaged building -- the one case repair applies. */
    UFUNCTION(BlueprintPure, Category = "RA4|UI")
    bool CanSelectionBeRepaired() const { return bSelectionCanRepair; }

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

    /** Fired when selection changes, so the sidebar updates object info card. */
    DECLARE_MULTICAST_DELEGATE(FOnSelectionChanged);
    FOnSelectionChanged OnSelectionChanged;

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

    UPROPERTY(Transient)
    TArray<FRA4RadarMarker> RadarMarkers;

    FVector2D RadarMapSize = FVector2D::ZeroVector;
    bool bRadarOnline = true;
    int32 RadarLocalPlayer = 0;

    TArray<uint8> MinimapTerrain;
    TArray<uint8> MinimapShroud;
    FIntPoint MinimapCellCounts = FIntPoint::ZeroValue;
    int32 MinimapBackgroundRevision = 0;
    TArray<FRA4RadarPing> RadarPings;

    int32 Credits = 0;
    int32 PowerProduced = 0;
    int32 PowerConsumed = 0;
    bool bPowerShortage = false;
    int32 SupplyUsed = 0;
    int32 SupplyCap = 0;
    bool bSupplyModelled = false;

    ERA4SelectionKind SelectionKind = ERA4SelectionKind::Empty;
    // ADR-0013 building controls, refreshed from the snapshot each frame.
    bool bSelectionIsBuilding = false;
    ERA4PowerPriority SelectionPowerPriority = ERA4PowerPriority::Production;
    bool bSelectionPowerOffline = false;
    bool bSelectionRepairing = false;
    bool bSelectionCanRepair = false;
    ERA4MatchPhase MatchPhase = ERA4MatchPhase::NotStarted;
    int32 MatchElapsedSeconds = 0;
    int32 WinningPlayer = -1;
    bool bLocalPlayerDefeated = false;
    bool bReportedMatchEnd = false;
};
