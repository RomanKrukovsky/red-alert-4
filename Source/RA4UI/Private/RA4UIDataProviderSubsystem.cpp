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

    // Russian Localization dictionary for unit & structure IDs
    static const std::unordered_map<std::string, FText> LocMap = {
        {"faction.soviet.building.construction_yard", NSLOCTEXT("RA4", "headquarters_su", "Штаб-квартира СССР")},
        {"faction.soviet.building.power_plant", NSLOCTEXT("RA4", "power_plant_su", "Электростанция")},
        {"faction.soviet.building.refinery", NSLOCTEXT("RA4", "refinery_su", "Переработчик руды")},
        {"faction.soviet.building.barracks", NSLOCTEXT("RA4", "barracks_su", "Казармы СССР")},
        {"faction.soviet.building.war_factory", NSLOCTEXT("RA4", "war_factory_su", "Завод техники СССР")},
        {"faction.soviet.building.turret", NSLOCTEXT("RA4", "turret_ai_su", "Пулеметная турель")},
        {"faction.soviet.unit.mcv", NSLOCTEXT("RA4", "mcv_su", "Мобильная база")},
        {"faction.soviet.unit.harvester", NSLOCTEXT("RA4", "harvester_su", "Рудосборщик «Богатырь»")},
        {"faction.soviet.unit.basic_infantry", NSLOCTEXT("RA4", "rubezh_rifleman", "Стрелок «Рубеж»")},
        {"faction.soviet.unit.antiarmor_infantry", NSLOCTEXT("RA4", "zaslon_aa", "Расчёт «Заслон»")},
        {"faction.soviet.unit.main_tank", NSLOCTEXT("RA4", "granit_mbt", "Танк «Гранит»")},

        {"faction.alliance.building.construction_yard", NSLOCTEXT("RA4", "headquarters_al", "Штаб-квартира Альянса")},
        {"faction.alliance.building.power_plant", NSLOCTEXT("RA4", "power_plant_al", "Электростанция Альянса")},
        {"faction.alliance.building.refinery", NSLOCTEXT("RA4", "refinery_al", "Переработчик руды")},
        {"faction.alliance.building.barracks", NSLOCTEXT("RA4", "barracks_al", "Казармы Альянса")},
        {"faction.alliance.building.war_factory", NSLOCTEXT("RA4", "war_factory_al", "Завод техники Альянса")},
        {"faction.alliance.building.turret", NSLOCTEXT("RA4", "turret_ai_al", "Пулемётный ДОТ")},
        {"faction.alliance.unit.mcv", NSLOCTEXT("RA4", "mcv_al", "Мобильная база")},
        {"faction.alliance.unit.harvester", NSLOCTEXT("RA4", "harvester_al", "Рудосборщик «Пионер»")},
        {"faction.alliance.unit.basic_infantry", NSLOCTEXT("RA4", "sentinel_rifleman", "Пехотинец «Страж»")},
        {"faction.alliance.unit.antiarmor_infantry", NSLOCTEXT("RA4", "lancer_team", "Ракетчик «Лансер»")},
        {"faction.alliance.unit.main_tank", NSLOCTEXT("RA4", "bulwark_mbt", "Танк «Оплот»")},

        {"unit.sov.headquarters", NSLOCTEXT("RA4", "headquarters_su", "Штаб-квартира СССР")},
        {"unit.sov.power_plant", NSLOCTEXT("RA4", "power_plant_su", "Электростанция")},
        {"unit.sov.refinery", NSLOCTEXT("RA4", "refinery_su", "Переработчик руды")},
        {"unit.sov.barracks", NSLOCTEXT("RA4", "barracks_su", "Казармы СССР")},
        {"unit.sov.war_factory", NSLOCTEXT("RA4", "war_factory_su", "Завод техники СССР")},
        {"unit.sov.radar", NSLOCTEXT("RA4", "radar_su", "Радарный комплекс")},
        {"unit.sov.turret_ai", NSLOCTEXT("RA4", "turret_ai_su", "Пулеметная турель")},
        {"unit.sov.turret_aa", NSLOCTEXT("RA4", "turret_aa_su", "ПВО турель")},
        {"unit.sov.rubezh_rifleman", NSLOCTEXT("RA4", "rubezh_rifleman", "Стрелок «Рубеж»")},
        {"unit.sov.zaslon_aa_team", NSLOCTEXT("RA4", "zaslon_aa", "ПВО команда «Заслон»")},
        {"unit.sov.master_engineer", NSLOCTEXT("RA4", "master_engineer", "Мастер-инженер")},
        {"unit.sov.rys_scout", NSLOCTEXT("RA4", "rys_scout", "Разведчик «Рысь»")},
        {"unit.sov.granit_mbt", NSLOCTEXT("RA4", "granit_mbt", "Основной танк «Гранит»")},
        {"unit.sov.zarevo_mlrs", NSLOCTEXT("RA4", "zarevo_mlrs", "РСЗО «Зарево»")},

        {"unit.al.headquarters", NSLOCTEXT("RA4", "headquarters_al", "Штаб-квартира Альянса")},
        {"unit.al.power_plant", NSLOCTEXT("RA4", "power_plant_al", "Электростанция Альянса")},
        {"unit.al.refinery", NSLOCTEXT("RA4", "refinery_al", "Переработчик руды")},
        {"unit.al.barracks", NSLOCTEXT("RA4", "barracks_al", "Казармы Альянса")},
        {"unit.al.war_factory", NSLOCTEXT("RA4", "war_factory_al", "Завод техники Альянса")},
        {"unit.al.radar", NSLOCTEXT("RA4", "radar_al", "Радарный комплекс")},
        {"unit.al.sentinel_rifleman", NSLOCTEXT("RA4", "sentinel_rifleman", "Пехотинец «Страж»")},
        {"unit.al.lancer_team", NSLOCTEXT("RA4", "lancer_team", "Ракетчик «Лансер»")},
        {"unit.al.field_engineer", NSLOCTEXT("RA4", "field_engineer", "Полевой инженер")},
        {"unit.al.kestrel_scout", NSLOCTEXT("RA4", "kestrel_scout", "Разведчик «Пустельга»")},
        {"unit.al.bulwark_mbt", NSLOCTEXT("RA4", "bulwark_mbt", "Танк «Оплот»")},
        {"unit.al.oracle_artillery", NSLOCTEXT("RA4", "oracle_artillery", "Артиллерия «Оракул»")},
        
        {"headquarters", NSLOCTEXT("RA4", "headquarters_generic", "Штаб-квартира")},
        {"power_plant", NSLOCTEXT("RA4", "power_generic", "Электростанция")},
        {"refinery", NSLOCTEXT("RA4", "refinery_generic", "Переработчик руды")},
        {"barracks", NSLOCTEXT("RA4", "barracks_generic", "Казармы")},
        {"war_factory", NSLOCTEXT("RA4", "factory_generic", "Завод техники")},
        {"radar", NSLOCTEXT("RA4", "radar_generic", "Радарный комплекс")}
    };

    auto It = LocMap.find(Key);
    if (It != LocMap.end())
    {
        return It->second;
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

FText URA4UIDataProviderSubsystem::GetDisplayNameForKey(const FString& Key) const
{
    return KeyToText(std::string(TCHAR_TO_UTF8(*Key)));
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
    HUDViewModel->SetSelectionState(
        Snapshot.Selection.TotalCount, PrimaryHealthRatio,
        KeyToText(Snapshot.Selection.PrimaryDisplayNameKey).ToString(),
        Snapshot.Selection.bPrimaryIsOwned);
    OnSelectionChanged.Broadcast();

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
        Out.PowerDelta = Option.PowerDelta;
        Out.PrerequisiteText = KeyToText(Option.PrerequisiteKey);
        Out.Category = int32(Option.Category);
        Out.bAvailable = Option.bAvailable;
        Out.BlockReason = ToBlueprint(Option.BlockReason);
        BuildOptions.Add(Out);
    }

    if (HasVisibleProductionChange(PreviousQueue, PreviousOptions))
    {
        OnProductionChanged.Broadcast();
    }

    // --- radar ---------------------------------------------------------------
    // The snapshot has already applied fog-of-war filtering. The UI only receives
    // markers it is allowed to draw and therefore cannot reveal hidden enemies.
    RadarMapSize = FVector2D(Snapshot.Radar.MapWidthUnits, Snapshot.Radar.MapHeightUnits);
    RadarLocalPlayer = int32(Snapshot.LocalPlayer);
    RadarMarkers.Reset(int32(Snapshot.Radar.Markers.size()));
    for (const RP::RadarMarker& Marker : Snapshot.Radar.Markers)
    {
        FRA4RadarMarker Out;
        Out.WorldPosition = FVector2D(Marker.Position.X.ToDoubleUnsafe(),
                                      Marker.Position.Y.ToDoubleUnsafe());
        Out.Owner = Marker.Owner < RA4::kMaxPlayers ? int32(Marker.Owner) : -1;
        switch (Marker.Kind)
        {
            case RA4::EntityKind::Building:
                Out.Kind = ERA4RadarMarkerKind::Building;
                break;
            case RA4::EntityKind::ResourceNode:
                Out.Kind = ERA4RadarMarkerKind::Resource;
                break;
            default:
                Out.Kind = ERA4RadarMarkerKind::Unit;
                break;
        }
        Out.bSelected = Marker.bSelected;
        RadarMarkers.Add(Out);
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
