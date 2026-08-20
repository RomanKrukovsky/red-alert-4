// Copyright (c) Red Alert 4 project.

#include "RA4CampaignViewModel.h"

#define LOCTEXT_NAMESPACE "RA4CampaignViewModel"

namespace
{
FRA4MissionNodeView MakeMission(
    const TCHAR* Id,
    const ERA4FactionTheme Theme,
    const int32 Number,
    const FText& Name,
    const FText& Location,
    const FText& Objective,
    const int32 Stars,
    const bool bCompleted,
    const bool bLocked = false)
{
    FRA4MissionNodeView Mission;
    Mission.ContentId = FName(Id);
    Mission.Theme = Theme;
    Mission.MissionNumber = Number;
    Mission.DisplayName = Name;
    Mission.Location = Location;
    Mission.Objective = Objective;
    Mission.Stars = FMath::Clamp(Stars, 0, 3);
    Mission.bCompleted = bCompleted;
    Mission.bLocked = bLocked;
    return Mission;
}
} // namespace

URA4CampaignViewModel::URA4CampaignViewModel()
{
    FactionCards = {
        {
            TEXT("ussr"), ERA4FactionTheme::USSR,
            LOCTEXT("USSRName", "СССР"),
            LOCTEXT("USSRMotto", "СЛАВА РОДИНЕ. БУДУЩЕЕ ЗА НАМИ."),
            LOCTEXT("USSRDescription", "Возглавьте возрождённый Советский Союз. Тяжёлая броня, дисциплина и стальная воля сокрушат врагов революции."),
            LOCTEXT("USSRCommander", "МАРШАЛ ВИКТОР СОКОЛОВ"),
            14, 24, 14.0f / 24.0f, false
        },
        {
            TEXT("allies"), ERA4FactionTheme::Allies,
            LOCTEXT("AlliesName", "АЛЬЯНС"),
            LOCTEXT("AlliesMotto", "ВЕРНОСТЬ. ЕДИНСТВО. ПОБЕДА."),
            LOCTEXT("AlliesDescription", "Альянс объединяет технологии, авиацию и точные удары, чтобы удержать мир от новой глобальной войны."),
            LOCTEXT("AlliesCommander", "ПРЕЗИДЕНТ ЭЛЕАНОР УОРД"),
            8, 24, 8.0f / 24.0f, false
        },
        {
            TEXT("eastern_coalition"), ERA4FactionTheme::EasternCoalition,
            LOCTEXT("EasternName", "ВОСТОЧНАЯ КОАЛИЦИЯ"),
            LOCTEXT("EasternMotto", "ЕДИНСТВО СОЗДАЁТ ПОБЕДУ."),
            LOCTEXT("EasternDescription", "Восточная коалиция соединяет древние традиции, промышленную мощь, дроны и технологии будущего."),
            LOCTEXT("EasternCommander", "ПРЕДСЕДАТЕЛЬ ЛИ ВЭЙ"),
            17, 27, 17.0f / 27.0f, false
        },
        {
            TEXT("chronolegion"), ERA4FactionTheme::Chronolegion,
            LOCTEXT("ChronoName", "ХРОНОЛЕГИОН"),
            LOCTEXT("ChronoMotto", "ВЛАСТЬ НАД ВРЕМЕНЕМ. ГОСПОДСТВО НАД ВСЕЛЕННОЙ."),
            LOCTEXT("ChronoDescription", "Хронолегион существует вне линейности. Управляйте временными аномалиями и переписывайте исход войны."),
            LOCTEXT("ChronoCommander", "ХРАНИТЕЛЬ ХРОНОС"),
            3, 12, 3.0f / 12.0f, false
        }
    };

    AllMissionNodes = {
        MakeMission(TEXT("ussr_warsaw"), ERA4FactionTheme::USSR, 1, LOCTEXT("Warsaw", "ОПЕРАЦИЯ «ИСКРА»"), LOCTEXT("WarsawLocation", "ВАРШАВА"), LOCTEXT("WarsawObjective", "Вернуть узел связи под контроль СССР."), 3, true),
        MakeMission(TEXT("ussr_berlin"), ERA4FactionTheme::USSR, 2, LOCTEXT("Berlin", "ОПЕРАЦИЯ «СТАЛЬ»"), LOCTEXT("BerlinLocation", "БЕРЛИН"), LOCTEXT("BerlinObjective", "Подавить бронетанковые силы противника."), 3, true),
        MakeMission(TEXT("ussr_baltic"), ERA4FactionTheme::USSR, 3, LOCTEXT("Baltic", "ОПЕРАЦИЯ «БУРЯ»"), LOCTEXT("BalticLocation", "ПРИБАЛТИКА"), LOCTEXT("BalticObjective", "Захватить побережье и аэродромы."), 3, true),
        MakeMission(TEXT("ussr_kiev"), ERA4FactionTheme::USSR, 4, LOCTEXT("Kiev", "ОПЕРАЦИЯ «КИЕВ-86»"), LOCTEXT("KievLocation", "КИЕВ"), LOCTEXT("KievObjective", "Уничтожить командный центр Альянса."), 3, true),
        MakeMission(TEXT("ussr_leningrad"), ERA4FactionTheme::USSR, 5, LOCTEXT("Leningrad", "ОПЕРАЦИЯ «ЩИТ»"), LOCTEXT("LeningradLocation", "ЛЕНИНГРАД"), LOCTEXT("LeningradObjective", "Отразить морское вторжение."), 3, true),
        MakeMission(TEXT("ussr_stalingrad"), ERA4FactionTheme::USSR, 6, LOCTEXT("Stalingrad", "ОПЕРАЦИЯ «КРЕПОСТЬ»"), LOCTEXT("StalingradLocation", "СТАЛИНГРАД"), LOCTEXT("StalingradObjective", "Удержать промышленный район."), 3, true),
        MakeMission(TEXT("ussr_caucasus"), ERA4FactionTheme::USSR, 7, LOCTEXT("Caucasus", "ОПЕРАЦИЯ «НЕФТЬ»"), LOCTEXT("CaucasusLocation", "КАВКАЗ"), LOCTEXT("CaucasusObjective", "Защитить ресурсные комплексы."), 2, true),
        MakeMission(TEXT("ussr_tehran"), ERA4FactionTheme::USSR, 8, LOCTEXT("Tehran", "ОПЕРАЦИЯ «РУБЕЖ»"), LOCTEXT("TehranLocation", "ТЕГЕРАН"), LOCTEXT("TehranObjective", "Перехватить вражескую ударную группу."), 2, true),
        MakeMission(TEXT("ussr_operation_molot"), ERA4FactionTheme::USSR, 9, LOCTEXT("Molot", "ОПЕРАЦИЯ «МОЛОТ»"), LOCTEXT("MolotLocation", "НОВОСИБИРСК"), LOCTEXT("MolotObjective", "Прорвите оборону НАТО и захватите исследовательский комплекс."), 2, false),
        MakeMission(TEXT("ussr_final_protocol"), ERA4FactionTheme::USSR, 10, LOCTEXT("FinalProtocol", "ФИНАЛЬНЫЙ ПРОТОКОЛ"), LOCTEXT("FinalLocation", "ЗАСЕКРЕЧЕНО"), LOCTEXT("FinalObjective", "Данные недоступны."), 0, false, true),
        MakeMission(TEXT("allies_ice_dawn"), ERA4FactionTheme::Allies, 3, LOCTEXT("IceDawn", "ЛЕДЯНОЙ РАССВЕТ"), LOCTEXT("IceDawnLocation", "АРКТИКА"), LOCTEXT("IceDawnObjective", "Защитить северный радарный пояс."), 2, false),
        MakeMission(TEXT("eastern_sky_shield"), ERA4FactionTheme::EasternCoalition, 18, LOCTEXT("SkyShield", "НЕБЕСНЫЙ ЩИТ"), LOCTEXT("SkyShieldLocation", "ТИХООКЕАНСКИЙ РЕГИОН"), LOCTEXT("SkyShieldObjective", "Испытать прототип энергетического щита."), 3, false),
        MakeMission(TEXT("chrono_time_rift"), ERA4FactionTheme::Chronolegion, 3, LOCTEXT("TimeRift", "РАЗЛОМ ВРЕМЕНИ"), LOCTEXT("TimeRiftLocation", "ВРЕМЕННОЙ УЗЕЛ 07"), LOCTEXT("TimeRiftObjective", "Стабилизировать хронокоридор."), 1, false)
    };

    RefreshMissionNodes();
}

bool URA4CampaignViewModel::SelectFaction(const ERA4FactionTheme InFaction)
{
    const FRA4FactionCardView* Card = FindFaction(InFaction);
    if (!Card || Card->bLocked)
    {
        return false;
    }

    if (SelectedFaction != InFaction)
    {
        SelectedFaction = InFaction;
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SelectedFaction);
    }
    FlowStage = ERA4CampaignFlowStage::CampaignOverview;
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(FlowStage);
    RefreshMissionNodes();
    return true;
}

bool URA4CampaignViewModel::SelectMission(const FName MissionId)
{
    const FRA4MissionNodeView* Mission = MissionNodes.FindByPredicate(
        [MissionId](const FRA4MissionNodeView& Candidate)
        {
            return Candidate.ContentId == MissionId;
        });
    if (!Mission || Mission->bLocked)
    {
        return false;
    }

    SelectedMissionId = MissionId;
    FlowStage = ERA4CampaignFlowStage::MissionMap;
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SelectedMissionId);
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(FlowStage);
    return true;
}

bool URA4CampaignViewModel::SetCampaignProgress(
    const ERA4FactionTheme Faction,
    const int32 CompletedMissions,
    const int32 TotalMissions)
{
    FRA4FactionCardView* Card = FactionCards.FindByPredicate(
        [Faction](const FRA4FactionCardView& Candidate)
        {
            return Candidate.Theme == Faction;
        });
    if (!Card)
    {
        return false;
    }

    Card->TotalMissions = FMath::Max(1, TotalMissions);
    Card->CompletedMissions = FMath::Clamp(CompletedMissions, 0, Card->TotalMissions);
    Card->Progress = static_cast<float>(Card->CompletedMissions) / static_cast<float>(Card->TotalMissions);
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(FactionCards);
    return true;
}

void URA4CampaignViewModel::SetDifficulty(const ERA4CampaignDifficulty InDifficulty)
{
    if (Difficulty != InDifficulty)
    {
        Difficulty = InDifficulty;
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Difficulty);
    }
}

bool URA4CampaignViewModel::StartMission()
{
    if (!FindSelectedMission())
    {
        return false;
    }
    FlowStage = ERA4CampaignFlowStage::Briefing;
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(FlowStage);
    return true;
}

void URA4CampaignViewModel::SkipBriefing()
{
    FlowStage = ERA4CampaignFlowStage::Loading;
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(FlowStage);
}

const FRA4FactionCardView* URA4CampaignViewModel::FindFaction(const ERA4FactionTheme Faction) const
{
    return FactionCards.FindByPredicate(
        [Faction](const FRA4FactionCardView& Candidate)
        {
            return Candidate.Theme == Faction;
        });
}

const FRA4MissionNodeView* URA4CampaignViewModel::FindSelectedMission() const
{
    return MissionNodes.FindByPredicate(
        [this](const FRA4MissionNodeView& Candidate)
        {
            return Candidate.ContentId == SelectedMissionId && !Candidate.bLocked;
        });
}

ERA4UIScreenId URA4CampaignViewModel::GetSelectedCampaignScreen() const
{
    switch (SelectedFaction)
    {
    case ERA4FactionTheme::USSR:
        return ERA4UIScreenId::SovietCampaign;
    case ERA4FactionTheme::Allies:
        return ERA4UIScreenId::AlliesCampaign;
    case ERA4FactionTheme::EasternCoalition:
        return ERA4UIScreenId::EasternCampaign;
    case ERA4FactionTheme::Chronolegion:
        return ERA4UIScreenId::ChronoCampaign;
    default:
        checkNoEntry();
        return ERA4UIScreenId::SovietCampaign;
    }
}

void URA4CampaignViewModel::RefreshMissionNodes()
{
    MissionNodes.Reset();
    for (const FRA4MissionNodeView& Mission : AllMissionNodes)
    {
        if (Mission.Theme == SelectedFaction)
        {
            MissionNodes.Add(Mission);
        }
    }

    const FRA4MissionNodeView* FirstUnlocked = MissionNodes.FindByPredicate(
        [](const FRA4MissionNodeView& Mission)
        {
            return !Mission.bLocked;
        });
    SelectedMissionId = FirstUnlocked ? FirstUnlocked->ContentId : NAME_None;
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(MissionNodes);
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SelectedMissionId);
}

#undef LOCTEXT_NAMESPACE
