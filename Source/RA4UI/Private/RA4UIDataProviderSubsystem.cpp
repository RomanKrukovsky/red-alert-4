// Copyright (c) Red Alert 4 project.
#include "RA4UIDataProviderSubsystem.h"

#include "RA4HUDViewModel.h"

#include "RA4Core/SimConfig.h"
#include "RA4Presentation/HudSnapshot.h"

// Deliberately no `using namespace`: the engine declares its own AlertType, so the
// presentation types are always spelled out.
namespace RP = RA4::Presentation;

namespace
{
ERA4SelectionKind ToBlueprint(RP::SelectionKind Kind)
{
    switch (Kind)
    {
        case RP::SelectionKind::SingleUnit: return ERA4SelectionKind::SingleUnit;
        case RP::SelectionKind::SingleBuilding: return ERA4SelectionKind::SingleBuilding;
        case RP::SelectionKind::MultipleUnits: return ERA4SelectionKind::MultipleUnits;
        case RP::SelectionKind::Mixed: return ERA4SelectionKind::Mixed;
        default: return ERA4SelectionKind::Empty;
    }
}

ERA4BuildBlockReason ToBlueprint(RP::BuildBlockReason Reason)
{
    switch (Reason)
    {
        case RP::BuildBlockReason::MissingPrerequisite: return ERA4BuildBlockReason::MissingPrerequisite;
        case RP::BuildBlockReason::InsufficientCredits: return ERA4BuildBlockReason::InsufficientCredits;
        case RP::BuildBlockReason::NoProducer: return ERA4BuildBlockReason::NoProducer;
        case RP::BuildBlockReason::QueueFull: return ERA4BuildBlockReason::QueueFull;
        case RP::BuildBlockReason::MatchOver: return ERA4BuildBlockReason::MatchOver;
        default: return ERA4BuildBlockReason::None;
    }
}

ERA4AlertSeverity ToBlueprint(RP::AlertSeverity Severity)
{
    switch (Severity)
    {
        case RP::AlertSeverity::Warning: return ERA4AlertSeverity::Warning;
        case RP::AlertSeverity::Critical: return ERA4AlertSeverity::Critical;
        default: return ERA4AlertSeverity::Info;
    }
}

ERA4MatchPhase ToBlueprint(RA4::MatchPhase Phase)
{
    switch (Phase)
    {
        case RA4::MatchPhase::Running: return ERA4MatchPhase::Running;
        case RA4::MatchPhase::Finished: return ERA4MatchPhase::Finished;
        default: return ERA4MatchPhase::NotStarted;
    }
}

// Localization keys travel from the simulation as plain strings. Until the string
// tables are authored they are shown as-is rather than silently blanked, so a
// missing key is visible during development instead of looking like empty UI.
FText KeyToText(const std::string& Key)
{
    if (Key.empty())
    {
        return FText::GetEmpty();
    }
    const FString AsString(UTF8_TO_TCHAR(Key.c_str()));
    return FText::AsCultureInvariant(AsString);
}

FText AlertToText(RP::AlertType Type)
{
    switch (Type)
    {
        case RP::AlertType::LowPower: return NSLOCTEXT("RA4", "Alert_LowPower", "Недостаточно энергии");
        case RP::AlertType::InsufficientFunds: return NSLOCTEXT("RA4", "Alert_NoFunds", "Недостаточно средств");
        case RP::AlertType::BaseUnderAttack: return NSLOCTEXT("RA4", "Alert_BaseAttack", "База атакована");
        case RP::AlertType::UnitsUnderAttack: return NSLOCTEXT("RA4", "Alert_UnitsAttack", "Наши войска атакованы");
        case RP::AlertType::BuildingLost: return NSLOCTEXT("RA4", "Alert_BuildingLost", "Здание потеряно");
        case RP::AlertType::UnitLost: return NSLOCTEXT("RA4", "Alert_UnitLost", "Юнит потерян");
        case RP::AlertType::ConstructionComplete: return NSLOCTEXT("RA4", "Alert_Built", "Строительство завершено");
        case RP::AlertType::UnitReady: return NSLOCTEXT("RA4", "Alert_UnitReady", "Юнит готов");
        case RP::AlertType::ResourcesDepleted: return NSLOCTEXT("RA4", "Alert_NoOre", "Месторождение исчерпано");
        default: return FText::GetEmpty();
    }
}

float TicksToSeconds(int32 Ticks)
{
    return float(Ticks) / float(RA4::kTicksPerSecond);
}
} // namespace

void URA4UIDataProviderSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    HUDViewModel = NewObject<URA4HUDViewModel>(this);
}

void URA4UIDataProviderSubsystem::Deinitialize()
{
    HUDViewModel = nullptr;
    SelectionGroups.Reset();
    ProductionQueue.Reset();
    BuildOptions.Reset();
    Alerts.Reset();
    Super::Deinitialize();
}

bool URA4UIDataProviderSubsystem::HasVisibleProductionChange(const TArray<FRA4ProductionEntry>& PreviousQueue,
                                                             const TArray<FRA4BuildOption>& PreviousOptions) const
{
    if (PreviousQueue.Num() != ProductionQueue.Num() || PreviousOptions.Num() != BuildOptions.Num())
    {
        return true;
    }

    for (int32 Index = 0; Index < ProductionQueue.Num(); ++Index)
    {
        const FRA4ProductionEntry& Old = PreviousQueue[Index];
        const FRA4ProductionEntry& New = ProductionQueue[Index];
        if (Old.ContentId != New.ContentId || Old.ProgressPercent != New.ProgressPercent ||
            Old.bPaused != New.bPaused || Old.bAwaitingPlacement != New.bAwaitingPlacement)
        {
            return true;
        }
    }

    for (int32 Index = 0; Index < BuildOptions.Num(); ++Index)
    {
        const FRA4BuildOption& Old = PreviousOptions[Index];
        const FRA4BuildOption& New = BuildOptions[Index];
        // Cost and category are static per content id, so availability and the reason
        // shown on a blocked card are the only things that can change under the player.
        if (Old.ContentId != New.ContentId || Old.bAvailable != New.bAvailable ||
            Old.BlockReason != New.BlockReason)
        {
            return true;
        }
    }

    return false;
}

TArray<FRA4BuildOption> URA4UIDataProviderSubsystem::GetBuildOptionsForCategory(int32 Category) const
{
    TArray<FRA4BuildOption> Result;
    for (const FRA4BuildOption& Option : BuildOptions)
    {
        if (Option.Category == Category)
        {
            Result.Add(Option);
        }
    }
    return Result;
}

void URA4UIDataProviderSubsystem::ApplySnapshot(const RA4::Presentation::HudSnapshot& Snapshot)
{
    if (HUDViewModel == nullptr)
    {
        return;
    }

    // --- scalars: the view model already guards against redundant writes -------
    // Mirrored here too so widgets can read them without a view model, and so the
    // change delegate fires on real movement rather than every tick.
    const bool bResourcesChanged = Credits != Snapshot.Resources.Credits ||
                                   PowerProduced != Snapshot.Resources.PowerProduced ||
                                   PowerConsumed != Snapshot.Resources.PowerConsumed ||
                                   bPowerShortage != Snapshot.Resources.bPowerShortage ||
                                   SupplyUsed != Snapshot.Resources.SupplyUsed ||
                                   SupplyCap != Snapshot.Resources.SupplyCap ||
                                   MatchElapsedSeconds != Snapshot.Match.ElapsedSeconds;
    Credits = Snapshot.Resources.Credits;
    PowerProduced = Snapshot.Resources.PowerProduced;
    PowerConsumed = Snapshot.Resources.PowerConsumed;
    bPowerShortage = Snapshot.Resources.bPowerShortage;
    SupplyUsed = Snapshot.Resources.SupplyUsed;
    SupplyCap = Snapshot.Resources.SupplyCap;
    bSupplyModelled = Snapshot.Resources.bSupplyModelled;
    if (bResourcesChanged)
    {
        OnResourcesChanged.Broadcast();
    }

    HUDViewModel->SetCredits(Snapshot.Resources.Credits);
    HUDViewModel->SetPower(Snapshot.Resources.PowerProduced, Snapshot.Resources.PowerConsumed);
    HUDViewModel->SetPowerShortage(Snapshot.Resources.bPowerShortage);

    const float PrimaryHealthRatio =
        Snapshot.Selection.PrimaryHealthMax > 0
            ? float(Snapshot.Selection.PrimaryHealthCurrent) / float(Snapshot.Selection.PrimaryHealthMax)
            : 0.0f;
    HUDViewModel->SetSelectionState(Snapshot.Selection.TotalCount, PrimaryHealthRatio,
                                    UTF8_TO_TCHAR(Snapshot.Selection.PrimaryDisplayNameKey.c_str()),
                                    Snapshot.Selection.bPrimaryIsOwned);

    SelectionKind = ToBlueprint(Snapshot.Selection.Kind);

    // --- selection groups -----------------------------------------------------
    SelectionGroups.Reset(int32(Snapshot.Selection.Groups.size()));
    for (const RP::SelectionGroup& Group : Snapshot.Selection.Groups)
    {
        FRA4SelectionGroup Out;
        Out.ContentId = int64(Group.Content.Value);
        Out.Count = Group.Count;
        Out.HealthRatio = Group.HealthMax > 0 ? float(Group.HealthCurrent) / float(Group.HealthMax) : 1.0f;
        SelectionGroups.Add(Out);
    }

    // --- production queue -----------------------------------------------------
    const TArray<FRA4ProductionEntry> PreviousQueue = ProductionQueue;
    const TArray<FRA4BuildOption> PreviousOptions = BuildOptions;

    ProductionQueue.Reset(int32(Snapshot.Production.Queue.size()));
    for (const RP::QueueEntry& Entry : Snapshot.Production.Queue)
    {
        FRA4ProductionEntry Out;
        Out.ContentId = int64(Entry.Content.Value);
        Out.DisplayName = KeyToText(Entry.DisplayNameKey);
        Out.ProgressPercent = Entry.ProgressPercent;
        Out.RemainingSeconds = TicksToSeconds(Entry.RemainingTicks);
        Out.bPaused = Entry.bPaused;
        Out.bAwaitingPlacement = Entry.bAwaitingPlacement;
        Out.SlotIndex = Entry.SlotIndex;
        ProductionQueue.Add(Out);
    }

    // --- build sidebar --------------------------------------------------------
    BuildOptions.Reset(int32(Snapshot.Production.Options.size()));
    for (const RP::BuildOption& Option : Snapshot.Production.Options)
    {
        FRA4BuildOption Out;
        Out.ContentId = int64(Option.Content.Value);
        Out.DisplayName = KeyToText(Option.DisplayNameKey);
        Out.Cost = Option.Cost;
        Out.BuildSeconds = TicksToSeconds(Option.BuildTimeTicks);
        Out.Category = int32(Option.Category);
        Out.bAvailable = Option.bAvailable;
        Out.BlockReason = ToBlueprint(Option.BlockReason);
        BuildOptions.Add(Out);
    }

    if (HasVisibleProductionChange(PreviousQueue, PreviousOptions))
    {
        OnProductionChanged.Broadcast();
    }

    // --- alerts ---------------------------------------------------------------
    // Compared by content, not rebuilt blindly: the feed should animate when there
    // is news, not every tick.
    const int32 PreviousAlertCount = Alerts.Num();
    bool bAlertsDiffer = PreviousAlertCount != int32(Snapshot.Alerts.size());
    TArray<FRA4Alert> NewAlerts;
    NewAlerts.Reserve(int32(Snapshot.Alerts.size()));
    int32 Index = 0;
    for (const RP::Alert& In : Snapshot.Alerts)
    {
        FRA4Alert Out;
        Out.Message = AlertToText(In.Type);
        Out.Severity = ToBlueprint(In.Severity);
        Out.RepeatCount = In.RepeatCount;
        Out.bHasLocation = In.bHasLocation;
        Out.WorldLocation = FVector2D(In.Location.X.ToDoubleUnsafe(), In.Location.Y.ToDoubleUnsafe());
        if (!bAlertsDiffer && Index < PreviousAlertCount)
        {
            const FRA4Alert& Old = Alerts[Index];
            bAlertsDiffer = Old.Severity != Out.Severity || Old.RepeatCount != Out.RepeatCount ||
                            !Old.Message.EqualTo(Out.Message);
        }
        NewAlerts.Add(Out);
        ++Index;
    }
    if (bAlertsDiffer)
    {
        Alerts = MoveTemp(NewAlerts);
        OnAlertsChanged.Broadcast();
    }

    // --- match state ----------------------------------------------------------
    MatchElapsedSeconds = Snapshot.Match.ElapsedSeconds;
    MatchPhase = ToBlueprint(Snapshot.Match.Phase);
    WinningPlayer = Snapshot.Match.Winner == RA4::kInvalidPlayer ? -1 : int32(Snapshot.Match.Winner);
    bLocalPlayerDefeated = Snapshot.Match.bLocalPlayerDefeated;

    // Fired exactly once: the victory screen must not be pushed every tick after
    // the match ends.
    if (MatchPhase == ERA4MatchPhase::Finished && !bReportedMatchEnd)
    {
        bReportedMatchEnd = true;
        const bool bWon = WinningPlayer == int32(Snapshot.LocalPlayer);
        OnMatchEnded.Broadcast(bWon, WinningPlayer);
    }
}
