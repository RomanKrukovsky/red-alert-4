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

ERA4PowerPriority ToBlueprint(RA4::PowerPriority Priority)
{
    switch (Priority)
    {
        case RA4::PowerPriority::Vital: return ERA4PowerPriority::Vital;
        case RA4::PowerPriority::Defense: return ERA4PowerPriority::Defense;
        case RA4::PowerPriority::Auxiliary: return ERA4PowerPriority::Auxiliary;
        default: return ERA4PowerPriority::Production;
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
        {"faction.soviet.building.construction_yard", NSLOCTEXT("RA4", "headquarters_su", "Сборочный цех СССР")},
        {"faction.soviet.building.power_plant", NSLOCTEXT("RA4", "power_plant_su", "Электростанция СССР")},
        {"faction.soviet.building.refinery", NSLOCTEXT("RA4", "refinery_su", "Обогатительный комбинат СССР")},
        {"faction.soviet.building.barracks", NSLOCTEXT("RA4", "barracks_su", "Казармы СССР")},
        {"faction.soviet.building.war_factory", NSLOCTEXT("RA4", "war_factory_su", "Военный завод СССР")},
        {"faction.soviet.building.turret", NSLOCTEXT("RA4", "turret_ai_su", "Пулемётная турель")},
        {"faction.soviet.unit.mcv", NSLOCTEXT("RA4", "mcv_su", "МСЦ СССР")},
        {"faction.soviet.unit.harvester", NSLOCTEXT("RA4", "harvester_su", "Рудный комбайн «Богатырь»")},
        {"faction.soviet.unit.basic_infantry", NSLOCTEXT("RA4", "rubezh_rifleman", "Стрелок «Рубеж»")},
        {"faction.soviet.unit.antiarmor_infantry", NSLOCTEXT("RA4", "zaslon_aa", "Расчёт «Заслон»")},
        {"faction.soviet.unit.main_tank", NSLOCTEXT("RA4", "granit_mbt", "Тяжёлый танк «Гранит»")},

        {"faction.alliance.building.construction_yard", NSLOCTEXT("RA4", "headquarters_al", "Штаб строительства Альянса")},
        {"faction.alliance.building.power_plant", NSLOCTEXT("RA4", "power_plant_al", "Электростанция Альянса")},
        {"faction.alliance.building.refinery", NSLOCTEXT("RA4", "refinery_al", "Перерабатывающий завод Альянса")},
        {"faction.alliance.building.barracks", NSLOCTEXT("RA4", "barracks_al", "Казармы Альянса")},
        {"faction.alliance.building.war_factory", NSLOCTEXT("RA4", "war_factory_al", "Военный завод Альянса")},
        {"faction.alliance.building.turret", NSLOCTEXT("RA4", "turret_ai_al", "Оборонительный ДОТ")},
        {"faction.alliance.unit.mcv", NSLOCTEXT("RA4", "mcv_al", "МСЦ Альянса")},
        {"faction.alliance.unit.harvester", NSLOCTEXT("RA4", "harvester_al", "Сборщик руды «Пионер»")},
        {"faction.alliance.unit.basic_infantry", NSLOCTEXT("RA4", "sentinel_rifleman", "Пехотинец «Страж»")},
        {"faction.alliance.unit.antiarmor_infantry", NSLOCTEXT("RA4", "lancer_team", "Ракетчик «Лансер»")},
        {"faction.alliance.unit.main_tank", NSLOCTEXT("RA4", "bulwark_mbt", "Основной танк «Оплот»")},

        {"faction.coalition.building.construction_yard", NSLOCTEXT("RA4", "headquarters_ec", "Командный центр Коалиции")},
        {"faction.coalition.building.power_plant", NSLOCTEXT("RA4", "power_plant_ec", "Термоядерная станция")},
        {"faction.coalition.building.refinery", NSLOCTEXT("RA4", "refinery_ec", "Горнорудный комплекс")},
        {"faction.coalition.building.barracks", NSLOCTEXT("RA4", "barracks_ec", "Учебный центр Коалиции")},
        {"faction.coalition.building.war_factory", NSLOCTEXT("RA4", "war_factory_ec", "Тяжёлый арсенал")},
        {"faction.coalition.unit.basic_infantry", NSLOCTEXT("RA4", "dragon_infantry", "Штурмовик «Дракон»")},
        {"faction.coalition.unit.main_tank", NSLOCTEXT("RA4", "qilin_mbt", "Танк прорыва «Цилинь»")},

        {"faction.chronolegion.building.construction_yard", NSLOCTEXT("RA4", "headquarters_cl", "Хроно-узел Легиона")},
        {"faction.chronolegion.building.power_plant", NSLOCTEXT("RA4", "power_plant_cl", "Квантовый реактор")},
        {"faction.chronolegion.building.refinery", NSLOCTEXT("RA4", "refinery_cl", "Хроно-экстрактор")},
        {"faction.chronolegion.unit.main_tank", NSLOCTEXT("RA4", "paradox_mbt", "Хроно-танк «Парадокс»")},

        {"unit.sov.headquarters", NSLOCTEXT("RA4", "headquarters_su", "Сборочный цех СССР")},
        {"unit.sov.power_plant", NSLOCTEXT("RA4", "power_plant_su", "Электростанция СССР")},
        {"unit.sov.refinery", NSLOCTEXT("RA4", "refinery_su", "Обогатительный комбинат СССР")},
        {"unit.sov.barracks", NSLOCTEXT("RA4", "barracks_su", "Казармы СССР")},
        {"unit.sov.war_factory", NSLOCTEXT("RA4", "war_factory_su", "Военный завод СССР")},
        {"unit.sov.radar", NSLOCTEXT("RA4", "radar_su", "Радарный комплекс СССР")},
        {"unit.sov.turret_ai", NSLOCTEXT("RA4", "turret_ai_su", "Пулемётная турель")},
        {"unit.sov.turret_aa", NSLOCTEXT("RA4", "turret_aa_su", "Зенитный комплекс")},
        {"unit.sov.rubezh_rifleman", NSLOCTEXT("RA4", "rubezh_rifleman", "Стрелок «Рубеж»")},
        {"unit.sov.zaslon_aa_team", NSLOCTEXT("RA4", "zaslon_aa", "Расчёт «Заслон»")},
        {"unit.sov.master_engineer", NSLOCTEXT("RA4", "master_engineer", "Главный инженер")},
        {"unit.sov.rys_scout", NSLOCTEXT("RA4", "rys_scout", "Бронеавтомобиль «Рысь»")},
        {"unit.sov.granit_mbt", NSLOCTEXT("RA4", "granit_mbt", "Тяжёлый танк «Гранит»")},
        {"unit.sov.zarevo_mlrs", NSLOCTEXT("RA4", "zarevo_mlrs", "РСЗО «Зарево»")},

        {"unit.al.headquarters", NSLOCTEXT("RA4", "headquarters_al", "Штаб строительства Альянса")},
        {"unit.al.power_plant", NSLOCTEXT("RA4", "power_plant_al", "Электростанция Альянса")},
        {"unit.al.refinery", NSLOCTEXT("RA4", "refinery_al", "Перерабатывающий завод Альянса")},
        {"unit.al.barracks", NSLOCTEXT("RA4", "barracks_al", "Казармы Альянса")},
        {"unit.al.war_factory", NSLOCTEXT("RA4", "war_factory_al", "Военный завод Альянса")},
        {"unit.al.radar", NSLOCTEXT("RA4", "radar_al", "Радарный узел Альянса")},
        {"unit.al.sentinel_rifleman", NSLOCTEXT("RA4", "sentinel_rifleman", "Пехотинец «Страж»")},
        {"unit.al.lancer_team", NSLOCTEXT("RA4", "lancer_team", "Ракетчик «Лансер»")},
        {"unit.al.field_engineer", NSLOCTEXT("RA4", "field_engineer", "Полевой инженер")},
        {"unit.al.kestrel_scout", NSLOCTEXT("RA4", "kestrel_scout", "Разведчик «Пустельга»")},
        {"unit.al.bulwark_mbt", NSLOCTEXT("RA4", "bulwark_mbt", "Основной танк «Оплот»")},
        {"unit.al.oracle_artillery", NSLOCTEXT("RA4", "oracle_artillery", "САУ «Оракул»")},
        
        {"headquarters", NSLOCTEXT("RA4", "headquarters_generic", "Главный штаб")},
        {"power_plant", NSLOCTEXT("RA4", "power_generic", "Электростанция")},
        {"refinery", NSLOCTEXT("RA4", "refinery_generic", "Обогатительный комбинат")},
        {"barracks", NSLOCTEXT("RA4", "barracks_generic", "Казармы")},
        {"war_factory", NSLOCTEXT("RA4", "factory_generic", "Военный завод")},
        {"radar", NSLOCTEXT("RA4", "radar_generic", "Радарная станция")}
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
        case RP::AlertType::LowPower: return NSLOCTEXT("RA4", "Alert_LowPower", "Дефицит энергии");
        case RP::AlertType::InsufficientFunds: return NSLOCTEXT("RA4", "Alert_NoFunds", "Недостаточно средств");
        case RP::AlertType::BaseUnderAttack: return NSLOCTEXT("RA4", "Alert_BaseAttack", "База атакована");
        case RP::AlertType::UnitsUnderAttack: return NSLOCTEXT("RA4", "Alert_UnitsAttack", "Войска под огнём");
        case RP::AlertType::BuildingLost: return NSLOCTEXT("RA4", "Alert_BuildingLost", "Здание уничтожено");
        case RP::AlertType::UnitLost: return NSLOCTEXT("RA4", "Alert_UnitLost", "Боевая единица потеряна");
        case RP::AlertType::ConstructionComplete: return NSLOCTEXT("RA4", "Alert_Built", "Строительство завершено");
        case RP::AlertType::UnitReady: return NSLOCTEXT("RA4", "Alert_UnitReady", "Боевая единица готова");
        case RP::AlertType::ResourcesDepleted: return NSLOCTEXT("RA4", "Alert_NoOre", "Месторождение истощено");
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
            Old.bPaused != New.bPaused || Old.bAwaitingPlacement != New.bAwaitingPlacement ||
            // A starving item's progress bar does not move, so without this the
            // widget would never refresh to show (or clear) the warning.
            Old.bStarvedForCredits != New.bStarvedForCredits)
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

    const float PrimaryHealthRatio =
        Snapshot.Selection.PrimaryHealthMax > 0
            ? float(Snapshot.Selection.PrimaryHealthCurrent) / float(Snapshot.Selection.PrimaryHealthMax)
            : 0.0f;
    SelectionKind = ToBlueprint(Snapshot.Selection.Kind);
    HUDViewModel->SetSelectionKind(SelectionKind);

    // ADR-0013 building controls. Mirrored straight from the snapshot: the UI reads
    // state and issues commands, and never decides any of this for itself.
    bSelectionIsBuilding = Snapshot.Selection.bPrimaryIsBuilding;
    SelectionPowerPriority = ToBlueprint(Snapshot.Selection.PrimaryPowerPriority);
    bSelectionPowerOffline = Snapshot.Selection.bPrimaryPowerOffline;
    bSelectionRepairing = Snapshot.Selection.bPrimaryIsRepairing;
    bSelectionCanRepair = Snapshot.Selection.bPrimaryCanRepair;

    // What a selection widget actually displays. Compared before broadcasting, because
    // the delegate is what rebuilds the group rows: a widget that clears and reconstructs
    // its children twenty times a second while the selection has not moved is pure waste,
    // and this class promises in its header that an idle match produces no view model
    // traffic. Health is included because a selected unit taking damage is a real change;
    // the entity id is what distinguishes one tank from another tank of the same type.
    const bool bSelectionChanged =
        PreviousSelectionKind != SelectionKind ||
        PreviousSelectionCount != Snapshot.Selection.TotalCount ||
        PreviousPrimaryEntity != Snapshot.Selection.Primary.Packed() ||
        PreviousPrimaryHealth != Snapshot.Selection.PrimaryHealthCurrent ||
        PreviousSelectionGroupCount != int32(Snapshot.Selection.Groups.size());

    PreviousSelectionKind = SelectionKind;
    PreviousSelectionCount = Snapshot.Selection.TotalCount;
    PreviousPrimaryEntity = Snapshot.Selection.Primary.Packed();
    PreviousPrimaryHealth = Snapshot.Selection.PrimaryHealthCurrent;
    PreviousSelectionGroupCount = int32(Snapshot.Selection.Groups.size());

    if (bSelectionChanged)
    {
        OnSelectionChanged.Broadcast();
    }

    // --- selection groups -----------------------------------------------------
    // FRA4SelectionGroup carries a DisplayName, but the snapshot's group rows only carry
    // a content id. Fill it here from the keys already present in this same snapshot --
    // the build options for producible types, and the primary's own key for anything not
    // in a build list (captured structures, campaign-only units) -- so every consumer of
    // the group list gets a name instead of each widget repeating the lookup.
    SelectionGroups.Reset(int32(Snapshot.Selection.Groups.size()));
    for (const RP::SelectionGroup& Group : Snapshot.Selection.Groups)
    {
        FRA4SelectionGroup Out;
        Out.ContentId = int64(Group.Content.Value);
        Out.Count = Group.Count;
        Out.HealthRatio = Group.HealthMax > 0 ? float(Group.HealthCurrent) / float(Group.HealthMax) : 1.0f;

        const std::string* NameKey = nullptr;
        for (const RP::BuildOption& Option : Snapshot.Production.Options)
        {
            if (Option.Content.Value == Group.Content.Value)
            {
                NameKey = &Option.DisplayNameKey;
                break;
            }
        }
        if (NameKey == nullptr && Snapshot.Selection.PrimaryContent.Value == Group.Content.Value)
        {
            NameKey = &Snapshot.Selection.PrimaryDisplayNameKey;
        }
        if (NameKey != nullptr)
        {
            Out.DisplayName = KeyToText(*NameKey);
        }

        SelectionGroups.Add(Out);
    }

    // Broadcast only now, and only on a real change. Deliberately after the group list is
    // filled rather than before: a handler reads GetSelectionGroups(), so firing earlier
    // handed every widget the previous tick's rows -- one frame stale on every selection,
    // and on the first selection of a match, empty.
    if (bSelectionChanged)
    {
        OnSelectionChanged.Broadcast();
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
        Out.bStarvedForCredits = Entry.bStarvedForCredits;
        Out.PaidCredits = Entry.PaidCredits;
        Out.TotalCost = Entry.TotalCost;
        Out.SlotIndex = Entry.SlotIndex;
        ProductionQueue.Add(Out);

        FRA4ProductionQueueItem VMItem;
        VMItem.DisplayName = Out.DisplayName;
        VMItem.Cost = Entry.TotalCost;
        VMItem.Progress = FMath::Clamp(float(Entry.ProgressPercent) / 100.0f, 0.0f, 1.0f);
        VMItem.Quantity = 1;
        VMProductionQueue.Add(VMItem);
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
    bRadarOnline = Snapshot.Radar.bOnline;
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

    // The background is only copied on the ticks it actually changed. Re-uploading a few
    // thousand cells 20 times a second to say "identical" would be the most expensive thing
    // the HUD does, and on a fully explored map that is every tick.
    if (Snapshot.Radar.bBackgroundChanged)
    {
        const RP::MinimapBackground& Background = Snapshot.Radar.Background;
        MinimapCellCounts = FIntPoint(Background.Width, Background.Height);
        MinimapTerrain.SetNumUninitialized(int32(Background.Terrain.size()));
        MinimapShroud.SetNumUninitialized(int32(Background.Shroud.size()));
        if (!Background.Terrain.empty())
        {
            FMemory::Memcpy(MinimapTerrain.GetData(), Background.Terrain.data(),
                            Background.Terrain.size());
        }
        if (!Background.Shroud.empty())
        {
            FMemory::Memcpy(MinimapShroud.GetData(), Background.Shroud.data(),
                            Background.Shroud.size());
        }
        MinimapBackgroundRevision = int32(Snapshot.Radar.BackgroundRevision);
    }

    // Rebuilt every tick because the intensities are counting down; the list is bounded by
    // the alert feed, so this is a handful of entries at most.
    RadarPings.Reset(int32(Snapshot.Radar.Pings.size()));
    for (const RP::RadarPing& Ping : Snapshot.Radar.Pings)
    {
        FRA4RadarPing Out;
        Out.WorldPosition = FVector2D(Ping.Position.X.ToDoubleUnsafe(),
                                      Ping.Position.Y.ToDoubleUnsafe());
        Out.Kind = ERA4RadarPingKind(Ping.Kind);
        Out.Intensity = float(Ping.IntensityPercent) / 100.0f;
        RadarPings.Add(Out);
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

    FRA4HUDSnapshotView ViewSnapshot;
    ViewSnapshot.Credits = Snapshot.Resources.Credits;
    ViewSnapshot.CreditsDelta = Snapshot.Resources.CreditsDelta;
    ViewSnapshot.PowerProduced = Snapshot.Resources.PowerProduced;
    ViewSnapshot.PowerConsumed = Snapshot.Resources.PowerConsumed;
    ViewSnapshot.bPowerShortage = Snapshot.Resources.bPowerShortage;
    ViewSnapshot.SupplyUsed = Snapshot.Resources.SupplyUsed;
    ViewSnapshot.SupplyCap = Snapshot.Resources.SupplyCap;
    ViewSnapshot.MatchElapsedSeconds = Snapshot.Match.ElapsedSeconds;
    ViewSnapshot.SelectionKind = SelectionKind;
    ViewSnapshot.SelectionCount = Snapshot.Selection.TotalCount;
    ViewSnapshot.SelectionHealthRatio = PrimaryHealthRatio;
    ViewSnapshot.PrimaryEntityName = KeyToText(Snapshot.Selection.PrimaryDisplayNameKey).ToString();
    ViewSnapshot.bPrimaryOwned = Snapshot.Selection.bPrimaryIsOwned;
    ViewSnapshot.SelectionGroups = SelectionGroups;
    ViewSnapshot.ProductionQueue = ProductionQueue;
    ViewSnapshot.BuildOptions = BuildOptions;
    ViewSnapshot.Alerts = Alerts;
    HUDViewModel->ApplySnapshot(ViewSnapshot);

    // Fired exactly once: the victory screen must not be pushed every tick after
    // the match ends.
    if (MatchPhase == ERA4MatchPhase::Finished && !bReportedMatchEnd)
    {
        bReportedMatchEnd = true;
        const bool bWon = WinningPlayer == int32(Snapshot.LocalPlayer);
        OnMatchEnded.Broadcast(bWon, WinningPlayer);
    }
}
