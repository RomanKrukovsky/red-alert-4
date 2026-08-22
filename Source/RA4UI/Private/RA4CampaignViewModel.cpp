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
            TEXT("eurasian_pact"), ERA4FactionTheme::EurasianPact,
            LOCTEXT("EurasianName", "ЕВРАЗИЙСКИЙ ПАКТ (РОССИЯ)"),
            LOCTEXT("EurasianMotto", "ЕДИНСТВО. ТЕХНОЛОГИЯ. СУВЕРЕНИТЕТ."),
            LOCTEXT("EurasianDescription", "Континентальный щит и глубинная оборона. Тяжёлая механизация, эшелонированная ПВО, артиллерия и комплексы РЭБ."),
            LOCTEXT("EurasianCommander", "КОМАНДИР ИРИНА ВОЛКОВА"),
            14, 24, 14.0f / 24.0f, false
        },
        {
            TEXT("atlantic_alliance"), ERA4FactionTheme::AtlanticAlliance,
            LOCTEXT("AtlanticName", "АТЛАНТИЧЕСКИЙ АЛЬЯНС (США)"),
            LOCTEXT("AtlanticMotto", "СИЛА ЗАКОНА. ПРЕВОСХОДСТВО СЕТИ."),
            LOCTEXT("AtlanticDescription", "Господство в воздухе и на море, экспедиционные ударные группы, загоризонтная разведка и сетевое командование."),
            LOCTEXT("AtlanticCommander", "АДМИРАЛ МАРКУС РИД"),
            8, 24, 8.0f / 24.0f, false
        },
        {
            TEXT("eastern_coalition"), ERA4FactionTheme::EasternCoalition,
            LOCTEXT("EasternName", "ВОСТОЧНАЯ КОАЛИЦИЯ (КИТАЙ)"),
            LOCTEXT("EasternMotto", "ВЕЛИКОЕ ОБЪЕДИНЕНИЕ. ИНДУСТРИЯ ПОБЕДЫ."),
            LOCTEXT("EasternDescription", "Колоссальное автоматизированное производство, рои боевых БПЛА, танки Тип-99B и ракетные рубежи A2/AD."),
            LOCTEXT("EasternCommander", "ГЕНЕРАЛ-МАЙОР ЧЖАН ВЭЙ"),
            17, 27, 17.0f / 27.0f, false
        },
        {
            TEXT("pacific_pact"), ERA4FactionTheme::PacificPact,
            LOCTEXT("PacificName", "ТИХООКЕАНСКИЙ ПАКТ (ЯПОНИЯ)"),
            LOCTEXT("PacificMotto", "ОБОРОНА РУБЕЖЕЙ. РОБОТИЗАЦИЯ БУДУЩЕГО."),
            LOCTEXT("PacificDescription", "Островная оборона, автономные боевые шагоходы «Кайган», передовые лазерные комплексы и скоростные перехватчики."),
            LOCTEXT("PacificCommander", "КОМАНДОР РЕЙКО ТАНАКА"),
            9, 21, 9.0f / 21.0f, false
        },
        {
            TEXT("independent_powers"), ERA4FactionTheme::Independent,
            LOCTEXT("IndepName", "НЕЗАВИСИМЫЕ ДЕРЖАВЫ (ИРАН)"),
            LOCTEXT("IndepMotto", "КАТЕГОРИЯ ВЫБОРА • НЕ ЯВЛЯЕТСЯ СОЮЗОМ"),
            LOCTEXT("IndepDescription", "Асимметричные действия, мобильные баллистические пусковые комплексы «Хейбар», рои БПЛА «Шахед» и скрытные горные базы."),
            LOCTEXT("IndepCommander", "ПОЛКОВНИК ДАРИУШ РЕЗАИ"),
            5, 16, 5.0f / 16.0f, false
        }
    };

    AllMissionNodes = {
        MakeMission(TEXT("ru_polar_echo"), ERA4FactionTheme::EurasianPact, 1, LOCTEXT("RU1", "ПОЛЯРНЫЙ ЭХО"), LOCTEXT("RU1_Loc", "МУРМАНСКИЙ РУБЕЖ"), LOCTEXT("RU1_Obj", "Развернуть передовой комплекс РЭБ «Громобой» и подавить вражеский радар."), 3, true),
        MakeMission(TEXT("ru_kargaly_base"), ERA4FactionTheme::EurasianPact, 2, LOCTEXT("RU2", "БАЗА «КАРГАЛЫ»"), LOCTEXT("RU2_Loc", "СТЕПНОЙ ПЛАЦДАРМ"), LOCTEXT("RU2_Obj", "Развернуть передовой узел снабжения и удержать периметр до подхода бронегруппы."), 3, true),
        MakeMission(TEXT("ru_north_node"), ERA4FactionTheme::EurasianPact, 3, LOCTEXT("RU3", "УЗЕЛ «СЕВЕР»"), LOCTEXT("RU3_Loc", "СЕВЕРНЫЙ РАДАРНЫЙ ПОЯС"), LOCTEXT("RU3_Obj", "Вскрыть расположение сил противника разведмашинами БРМ-27 «Рысь»."), 3, true),
        MakeMission(TEXT("ru_iron_tooth"), ERA4FactionTheme::EurasianPact, 4, LOCTEXT("RU4", "ЖЕЛЕЗНЫЙ ЗУБ"), LOCTEXT("RU4_Loc", "УРАЛЬСКИЙ КОРИДОР"), LOCTEXT("RU4_Obj", "Отразить танковый прорыв с запада силами ОБТ-92 «Гранит»."), 3, true),
        MakeMission(TEXT("ru_relay_station"), ERA4FactionTheme::EurasianPact, 5, LOCTEXT("RU5", "РЕЛЕЙНАЯ СТАНЦИЯ"), LOCTEXT("RU5_Loc", "ЖЕЛЕЗНОДОРОЖНЫЙ УЗЕЛ"), LOCTEXT("RU5_Obj", "Восстановить железнодорожную логистику и обеспечить питание радарного узла."), 3, true),
        MakeMission(TEXT("ru_deep_signal"), ERA4FactionTheme::EurasianPact, 6, LOCTEXT("RU6", "ГЛУБОКИЙ СИГНАЛ"), LOCTEXT("RU6_Loc", "КАСПИЙСКИЙ УЗЕЛ"), LOCTEXT("RU6_Obj", "Захватить терминал связи и обеспечить развёртывание ТРС-18 «Зарево»."), 3, true),
        MakeMission(TEXT("ru_quiet_relay"), ERA4FactionTheme::EurasianPact, 7, LOCTEXT("RU7", "ТИХИЙ РЕЛЕЙ"), LOCTEXT("RU7_Loc", "ГОРНЫЙ КОРИДОР"), LOCTEXT("RU7_Obj", "Подавить 3 узла связи. Провести бронегруппу через перевал. Сохранить мобильный комплекс РЭБ."), 3, false),
        MakeMission(TEXT("ru_clean_key"), ERA4FactionTheme::EurasianPact, 8, LOCTEXT("RU8", "ЧИСТЫЙ КЛЮЧ"), LOCTEXT("RU8_Loc", "СЕВЕРНЫЙ РУБЕЖ"), LOCTEXT("RU8_Obj", "Прорвать укреплённый рубеж и зачистить плацдарм танками ТТП-11 «Воевода»."), 2, false, true),
        MakeMission(TEXT("us_ice_dawn"), ERA4FactionTheme::AtlanticAlliance, 1, LOCTEXT("US1", "ОПЕРАЦИЯ «ЛЕДЯНОЙ РАССВЕТ»"), LOCTEXT("US1_Loc", "АРКТИКА"), LOCTEXT("US1_Obj", "Защитить северный радарный пояс и развернуть стелс-авиацию F-35."), 3, true),
        MakeMission(TEXT("us_carrier_strike"), ERA4FactionTheme::AtlanticAlliance, 2, LOCTEXT("US2", "ОПЕРАЦИЯ «ДАЛЬНИЙ РУБЕЖ»"), LOCTEXT("US2_Loc", "СЕВЕРНОЕ МОРЕ"), LOCTEXT("US2_Obj", "Обеспечить сопровождение авианосной группы «Свобода»."), 2, false),
        MakeMission(TEXT("cn_jade_network"), ERA4FactionTheme::EasternCoalition, 1, LOCTEXT("CN1", "ОПЕРАЦИЯ «НЕФРИТОВАЯ СЕТЬ»"), LOCTEXT("CN1_Loc", "ДЕЛЬТА РЕКИ"), LOCTEXT("CN1_Obj", "Запустить автоматизированный конвейер и развернуть рои дронов «Шэньлун»."), 3, true),
        MakeMission(TEXT("cn_missile_shield"), ERA4FactionTheme::EasternCoalition, 2, LOCTEXT("CN2", "ОПЕРАЦИЯ «НЕБЕСНЫЙ ЩИТ»"), LOCTEXT("CN2_Loc", "ТИХООКЕАНСКИЙ РУБЕЖ"), LOCTEXT("CN2_Obj", "Развернуть гиперзвуковой комплекс DF-26 и отразить морской удар."), 2, false),
        MakeMission(TEXT("jp_storm_arc"), ERA4FactionTheme::PacificPact, 1, LOCTEXT("JP1", "ОПЕРАЦИЯ «ДУГА ШТОРМА»"), LOCTEXT("JP1_Loc", "ОСТРОВНОЙ УЗЕЛ ПВО"), LOCTEXT("JP1_Obj", "Активировать лазерные батареи «Кагами» и защитить роботизированный цех."), 3, true),
        MakeMission(TEXT("ir_shadow_ridge"), ERA4FactionTheme::Independent, 1, LOCTEXT("IR1", "ОПЕРАЦИЯ «ТЕНЬ НАД ХРЕБТОМ»"), LOCTEXT("IR1_Loc", "ГОРНЫЙ ХРЕБЕТ"), LOCTEXT("IR1_Obj", "Развернуть мобильные СПУ «Хейбар», провести сатурационный залп и сменить позицию."), 2, true)
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
    case ERA4FactionTheme::AtlanticAlliance:
        return ERA4UIScreenId::AtlanticCampaign;
    case ERA4FactionTheme::EasternCoalition:
        return ERA4UIScreenId::EasternCampaign;
    case ERA4FactionTheme::PacificPact:
        return ERA4UIScreenId::PacificCampaign;
    case ERA4FactionTheme::Independent:
        return ERA4UIScreenId::IndependentCampaign;
    case ERA4FactionTheme::EurasianPact:
    default:
        // Retired directions fall back to the shared Eurasian campaign.
        return ERA4UIScreenId::EurasianCampaign;
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

    // Open on the current objective: the first playable mission that is not yet
    // finished, falling back to the first playable one for a completed chapter.
    const FRA4MissionNodeView* Current = MissionNodes.FindByPredicate(
        [](const FRA4MissionNodeView& Mission)
        {
            return !Mission.bLocked && !Mission.bCompleted;
        });
    if (!Current)
    {
        Current = MissionNodes.FindByPredicate(
            [](const FRA4MissionNodeView& Mission)
            {
                return !Mission.bLocked;
            });
    }
    SelectedMissionId = Current ? Current->ContentId : NAME_None;
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(MissionNodes);
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SelectedMissionId);
}

#undef LOCTEXT_NAMESPACE
