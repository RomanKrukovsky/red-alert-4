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
        // ---------------------------------------------------------------
        // Евразийский пакт — Россия (RU_). Прежние SU_/soviet-ключи
        // сохранены как LegacyAliases для реплеев, сохранений и тестов,
        // но показывают только актуальные названия Scarlet Horizon.
        // ---------------------------------------------------------------
        {"faction.eurasian.building.construction_yard", NSLOCTEXT("RA4", "hq_ru", "Штаб")},
        {"faction.eurasian.building.power_plant", NSLOCTEXT("RA4", "power_plant_ru", "Электростанция")},
        {"faction.eurasian.building.refinery", NSLOCTEXT("RA4", "refinery_ru", "Перерабатывающий комплекс")},
        {"faction.eurasian.building.barracks", NSLOCTEXT("RA4", "barracks_ru", "Казарма")},
        {"faction.eurasian.building.war_factory", NSLOCTEXT("RA4", "war_factory_ru", "Завод бронетехники")},
        {"faction.eurasian.building.radar", NSLOCTEXT("RA4", "radar_ru", "Радарный узел")},
        {"faction.eurasian.building.turret", NSLOCTEXT("RA4", "turret_ai_ru", "Пулемётный ДОТ")},
        {"faction.eurasian.building.aa_turret", NSLOCTEXT("RA4", "turret_aa_ru", "Зенитный комплекс")},
        {"faction.eurasian.building.tech_center", NSLOCTEXT("RA4", "tech_center_ru", "Центр перспективных разработок")},
        {"faction.eurasian.building.emp_complex", NSLOCTEXT("RA4", "emp_ru", "Комплекс ЭМИ «Перун»")},

        {"faction.soviet.building.construction_yard", NSLOCTEXT("RA4", "hq_ru", "Штаб")},
        {"faction.soviet.building.power_plant", NSLOCTEXT("RA4", "power_plant_ru", "Электростанция")},
        {"faction.soviet.building.refinery", NSLOCTEXT("RA4", "refinery_ru", "Перерабатывающий комплекс")},
        {"faction.soviet.building.barracks", NSLOCTEXT("RA4", "barracks_ru", "Казарма")},
        {"faction.soviet.building.war_factory", NSLOCTEXT("RA4", "war_factory_ru", "Завод бронетехники")},
        {"faction.soviet.building.radar", NSLOCTEXT("RA4", "radar_ru", "Радарный узел")},
        {"faction.soviet.building.turret", NSLOCTEXT("RA4", "turret_ai_ru", "Пулемётный ДОТ")},
        {"faction.soviet.building.aa_turret", NSLOCTEXT("RA4", "turret_aa_ru", "Зенитный комплекс")},
        {"faction.soviet.building.tech_center", NSLOCTEXT("RA4", "tech_center_ru", "Центр перспективных разработок")},
        {"faction.soviet.building.tesla_coil", NSLOCTEXT("RA4", "emp_ru", "Комплекс ЭМИ «Перун»")},

        {"RU_Headquarters", NSLOCTEXT("RA4", "hq_ru", "Штаб")},
        {"RU_PowerPlant", NSLOCTEXT("RA4", "power_plant_ru", "Электростанция")},
        {"RU_Refinery", NSLOCTEXT("RA4", "refinery_ru", "Перерабатывающий комплекс")},
        {"RU_Barracks", NSLOCTEXT("RA4", "barracks_ru", "Казарма")},
        {"RU_WarFactory", NSLOCTEXT("RA4", "war_factory_ru", "Завод бронетехники")},
        {"RU_Radar", NSLOCTEXT("RA4", "radar_ru", "Радарный узел")},
        {"SU_ConYard", NSLOCTEXT("RA4", "hq_ru", "Штаб")},
        {"SU_PowerPlant", NSLOCTEXT("RA4", "power_plant_ru", "Электростанция")},
        {"SU_Refinery", NSLOCTEXT("RA4", "refinery_ru", "Перерабатывающий комплекс")},
        {"SU_Barracks", NSLOCTEXT("RA4", "barracks_ru", "Казарма")},
        {"SU_WarFactory", NSLOCTEXT("RA4", "war_factory_ru", "Завод бронетехники")},
        {"SU_Radar", NSLOCTEXT("RA4", "radar_ru", "Радарный узел")},
        {"SU_SentryTurret", NSLOCTEXT("RA4", "turret_ai_ru", "Пулемётный ДОТ")},
        {"SU_GunTurret", NSLOCTEXT("RA4", "turret_ai_ru", "Пулемётный ДОТ")},
        {"SU_FlakTurret", NSLOCTEXT("RA4", "turret_aa_ru", "Зенитный комплекс")},

        {"faction.eurasian.unit.mcv", NSLOCTEXT("RA4", "mcv_ru", "Полевой командный пункт")},
        {"faction.eurasian.unit.harvester", NSLOCTEXT("RA4", "harvester_ru", "ГРМ-8 «Богатырь»")},
        {"faction.eurasian.unit.basic_infantry", NSLOCTEXT("RA4", "rubezh_rifleman", "МС-12 «Рубеж»")},
        {"faction.eurasian.unit.antiarmor_infantry", NSLOCTEXT("RA4", "zaslon_aa", "ПЗК-9 «Заслон»")},
        {"faction.eurasian.unit.engineer", NSLOCTEXT("RA4", "master_engineer", "ИС-3 «Мастер»")},
        {"faction.eurasian.unit.shock_trooper", NSLOCTEXT("RA4", "razryad_trooper", "ЭШ-8 «Разряд»")},
        {"faction.eurasian.unit.officer", NSLOCTEXT("RA4", "vektor_officer", "КС-6 «Вектор»")},
        {"faction.eurasian.unit.scout", NSLOCTEXT("RA4", "rys_scout", "БРМ-27 «Рысь»")},
        {"faction.eurasian.unit.main_tank", NSLOCTEXT("RA4", "granit_mbt", "ОБТ-92 «Гранит»")},
        {"faction.eurasian.unit.artillery", NSLOCTEXT("RA4", "zarevo_mlrs", "ТРС-18 «Зарево»")},
        {"faction.eurasian.unit.ew_vehicle", NSLOCTEXT("RA4", "gromoboy_ew", "ЭМП-7 «Громобой»")},
        {"faction.eurasian.unit.heavy_tank", NSLOCTEXT("RA4", "voevoda_heavy", "ТТП-11 «Воевода»")},

        {"faction.soviet.unit.mcv", NSLOCTEXT("RA4", "mcv_ru", "Полевой командный пункт")},
        {"faction.soviet.unit.harvester", NSLOCTEXT("RA4", "harvester_ru", "ГРМ-8 «Богатырь»")},
        {"faction.soviet.unit.basic_infantry", NSLOCTEXT("RA4", "rubezh_rifleman", "МС-12 «Рубеж»")},
        {"faction.soviet.unit.antiarmor_infantry", NSLOCTEXT("RA4", "zaslon_aa", "ПЗК-9 «Заслон»")},
        {"faction.soviet.unit.engineer", NSLOCTEXT("RA4", "master_engineer", "ИС-3 «Мастер»")},
        {"faction.soviet.unit.shock_trooper", NSLOCTEXT("RA4", "razryad_trooper", "ЭШ-8 «Разряд»")},
        {"faction.soviet.unit.commissar", NSLOCTEXT("RA4", "vektor_officer", "КС-6 «Вектор»")},
        {"faction.soviet.unit.scout", NSLOCTEXT("RA4", "rys_scout", "БРМ-27 «Рысь»")},
        {"faction.soviet.unit.main_tank", NSLOCTEXT("RA4", "granit_mbt", "ОБТ-92 «Гранит»")},
        {"faction.soviet.unit.artillery", NSLOCTEXT("RA4", "zarevo_mlrs", "ТРС-18 «Зарево»")},
        {"faction.soviet.unit.flak_vehicle", NSLOCTEXT("RA4", "zaslon_aa", "ПЗК-9 «Заслон»")},

        {"RU_MCV", NSLOCTEXT("RA4", "mcv_ru", "Полевой командный пункт")},
        {"RU_BogatyrOreCarrier", NSLOCTEXT("RA4", "harvester_ru", "ГРМ-8 «Богатырь»")},
        {"RU_RubezhRifleman", NSLOCTEXT("RA4", "rubezh_rifleman", "МС-12 «Рубеж»")},
        {"RU_ZapalGrenadier", NSLOCTEXT("RA4", "zapal_grenadier", "ОШ-4 «Запал»")},
        {"RU_ZaslonAATeam", NSLOCTEXT("RA4", "zaslon_aa", "ПЗК-9 «Заслон»")},
        {"RU_MasterEngineer", NSLOCTEXT("RA4", "master_engineer", "ИС-3 «Мастер»")},
        {"RU_RazryadTrooper", NSLOCTEXT("RA4", "razryad_trooper", "ЭШ-8 «Разряд»")},
        {"RU_VektorOfficer", NSLOCTEXT("RA4", "vektor_officer", "КС-6 «Вектор»")},
        {"RU_RysScout", NSLOCTEXT("RA4", "rys_scout", "БРМ-27 «Рысь»")},
        {"RU_GranitMBT", NSLOCTEXT("RA4", "granit_mbt", "ОБТ-92 «Гранит»")},
        {"RU_ZarevoMLRS", NSLOCTEXT("RA4", "zarevo_mlrs", "ТРС-18 «Зарево»")},
        {"RU_GromoboyRam", NSLOCTEXT("RA4", "gromoboy_ew", "ЭМП-7 «Громобой»")},
        {"RU_VoevodaHeavyTank", NSLOCTEXT("RA4", "voevoda_heavy", "ТТП-11 «Воевода»")},

        {"SU_MCV", NSLOCTEXT("RA4", "mcv_ru", "Полевой командный пункт")},
        {"SU_Harvester", NSLOCTEXT("RA4", "harvester_ru", "ГРМ-8 «Богатырь»")},
        {"SU_BogatyrOreCarrier", NSLOCTEXT("RA4", "harvester_ru", "ГРМ-8 «Богатырь»")},
        {"SU_Conscript", NSLOCTEXT("RA4", "rubezh_rifleman", "МС-12 «Рубеж»")},
        {"SU_RubezhRifleman", NSLOCTEXT("RA4", "rubezh_rifleman", "МС-12 «Рубеж»")},
        {"SU_Flak", NSLOCTEXT("RA4", "zaslon_aa", "ПЗК-9 «Заслон»")},
        {"SU_ZaslonAATeam", NSLOCTEXT("RA4", "zaslon_aa", "ПЗК-9 «Заслон»")},
        {"SU_RysScout", NSLOCTEXT("RA4", "rys_scout", "БРМ-27 «Рысь»")},
        {"SU_GranitMBT", NSLOCTEXT("RA4", "granit_mbt", "ОБТ-92 «Гранит»")},
        {"SU_HammerTank", NSLOCTEXT("RA4", "granit_mbt", "ОБТ-92 «Гранит»")},
        {"SU_ZarevoMLRS", NSLOCTEXT("RA4", "zarevo_mlrs", "ТРС-18 «Зарево»")},
        {"SU_Buratino", NSLOCTEXT("RA4", "zarevo_mlrs", "ТРС-18 «Зарево»")},

        // ---------------------------------------------------------------
        // Атлантический альянс. Общий арсенал ATL_; прежние AL_-ключи
        // остаются LegacyAliases.
        // ---------------------------------------------------------------
        {"faction.atlantic.building.construction_yard", NSLOCTEXT("RA4", "hq_atl", "Сетевой штаб")},
        {"faction.atlantic.building.power_plant", NSLOCTEXT("RA4", "power_plant_atl", "Компактный реактор")},
        {"faction.atlantic.building.refinery", NSLOCTEXT("RA4", "refinery_atl", "Переработчик")},
        {"faction.atlantic.building.barracks", NSLOCTEXT("RA4", "barracks_atl", "Тактическая казарма")},
        {"faction.atlantic.building.war_factory", NSLOCTEXT("RA4", "war_factory_atl", "Модульный завод")},
        {"faction.atlantic.building.radar", NSLOCTEXT("RA4", "radar_atl", "Разведцентр")},
        {"faction.atlantic.building.turret", NSLOCTEXT("RA4", "turret_ai_atl", "Автопушка")},
        {"faction.atlantic.building.aa_turret", NSLOCTEXT("RA4", "turret_aa_atl", "Ракетная ПВО")},
        {"faction.atlantic.building.tech_center", NSLOCTEXT("RA4", "tech_center_atl", "Центр перспективных систем")},

        {"faction.alliance.building.construction_yard", NSLOCTEXT("RA4", "hq_atl", "Сетевой штаб")},
        {"faction.alliance.building.power_plant", NSLOCTEXT("RA4", "power_plant_atl", "Компактный реактор")},
        {"faction.alliance.building.refinery", NSLOCTEXT("RA4", "refinery_atl", "Переработчик")},
        {"faction.alliance.building.barracks", NSLOCTEXT("RA4", "barracks_atl", "Тактическая казарма")},
        {"faction.alliance.building.war_factory", NSLOCTEXT("RA4", "war_factory_atl", "Модульный завод")},
        {"faction.alliance.building.radar", NSLOCTEXT("RA4", "radar_atl", "Разведцентр")},
        {"faction.alliance.building.turret", NSLOCTEXT("RA4", "turret_ai_atl", "Автопушка")},
        {"faction.alliance.building.aa_turret", NSLOCTEXT("RA4", "turret_aa_atl", "Ракетная ПВО")},
        {"faction.alliance.building.tech_center", NSLOCTEXT("RA4", "tech_center_atl", "Центр перспективных систем")},

        {"ATL_Headquarters", NSLOCTEXT("RA4", "hq_atl", "Сетевой штаб")},
        {"ATL_PowerPlant", NSLOCTEXT("RA4", "power_plant_atl", "Компактный реактор")},
        {"ATL_Refinery", NSLOCTEXT("RA4", "refinery_atl", "Переработчик")},
        {"ATL_Barracks", NSLOCTEXT("RA4", "barracks_atl", "Тактическая казарма")},
        {"ATL_WarFactory", NSLOCTEXT("RA4", "war_factory_atl", "Модульный завод")},
        {"ATL_Radar", NSLOCTEXT("RA4", "radar_atl", "Разведцентр")},
        {"AL_ConYard", NSLOCTEXT("RA4", "hq_atl", "Сетевой штаб")},
        {"AL_PowerPlant", NSLOCTEXT("RA4", "power_plant_atl", "Компактный реактор")},
        {"AL_Refinery", NSLOCTEXT("RA4", "refinery_atl", "Переработчик")},
        {"AL_Barracks", NSLOCTEXT("RA4", "barracks_atl", "Тактическая казарма")},
        {"AL_WarFactory", NSLOCTEXT("RA4", "war_factory_atl", "Модульный завод")},
        {"AL_Radar", NSLOCTEXT("RA4", "radar_atl", "Разведцентр")},
        {"AL_MultigunTurret", NSLOCTEXT("RA4", "turret_ai_atl", "Автопушка")},
        {"AL_GunTurret", NSLOCTEXT("RA4", "turret_ai_atl", "Автопушка")},

        {"faction.atlantic.unit.mcv", NSLOCTEXT("RA4", "mcv_atl", "Мобильный узел")},
        {"faction.atlantic.unit.harvester", NSLOCTEXT("RA4", "harvester_atl", "M88 «Pioneer»")},
        {"faction.atlantic.unit.basic_infantry", NSLOCTEXT("RA4", "sentinel_rifleman", "M6 «Sentinel»")},
        {"faction.atlantic.unit.antiarmor_infantry", NSLOCTEXT("RA4", "lancer_team", "Расчёт «Lancer»")},
        {"faction.atlantic.unit.engineer", NSLOCTEXT("RA4", "field_engineer", "Полевой инженер")},
        {"faction.atlantic.unit.scout", NSLOCTEXT("RA4", "kestrel_scout", "«Kestrel»")},
        {"faction.atlantic.unit.main_tank", NSLOCTEXT("RA4", "bulwark_mbt", "ОБТ «Bulwark»")},
        {"faction.atlantic.unit.artillery", NSLOCTEXT("RA4", "oracle_artillery", "САУ «Oracle»")},

        {"faction.alliance.unit.mcv", NSLOCTEXT("RA4", "mcv_atl", "Мобильный узел")},
        {"faction.alliance.unit.harvester", NSLOCTEXT("RA4", "harvester_atl", "M88 «Pioneer»")},
        {"faction.alliance.unit.basic_infantry", NSLOCTEXT("RA4", "sentinel_rifleman", "M6 «Sentinel»")},
        {"faction.alliance.unit.antiarmor_infantry", NSLOCTEXT("RA4", "lancer_team", "Расчёт «Lancer»")},
        {"faction.alliance.unit.engineer", NSLOCTEXT("RA4", "field_engineer", "Полевой инженер")},
        {"faction.alliance.unit.scout", NSLOCTEXT("RA4", "kestrel_scout", "«Kestrel»")},
        {"faction.alliance.unit.main_tank", NSLOCTEXT("RA4", "bulwark_mbt", "ОБТ «Bulwark»")},
        {"faction.alliance.unit.artillery", NSLOCTEXT("RA4", "oracle_artillery", "САУ «Oracle»")},

        {"ATL_SentinelRifleman", NSLOCTEXT("RA4", "sentinel_rifleman", "M6 «Sentinel»")},
        {"ATL_PioneerHarvester", NSLOCTEXT("RA4", "harvester_atl", "M88 «Pioneer»")},
        {"ATL_KestrelScout", NSLOCTEXT("RA4", "kestrel_scout", "«Kestrel»")},
        {"ATL_BulwarkMBT", NSLOCTEXT("RA4", "bulwark_mbt", "ОБТ «Bulwark»")},
        {"ATL_OracleArtillery", NSLOCTEXT("RA4", "oracle_artillery", "САУ «Oracle»")},
        {"AL_MCV", NSLOCTEXT("RA4", "mcv_atl", "Мобильный узел")},
        {"AL_Prospector", NSLOCTEXT("RA4", "harvester_atl", "M88 «Pioneer»")},
        {"AL_PioneerHarvester", NSLOCTEXT("RA4", "harvester_atl", "M88 «Pioneer»")},
        {"AL_Peacekeeper", NSLOCTEXT("RA4", "sentinel_rifleman", "M6 «Sentinel»")},
        {"AL_Javelin", NSLOCTEXT("RA4", "lancer_team", "Расчёт «Lancer»")},
        {"AL_Medic", NSLOCTEXT("RA4", "medic_atl", "Боевой медик")},
        {"AL_Jackal", NSLOCTEXT("RA4", "kestrel_scout", "«Kestrel»")},
        {"AL_KestrelScout", NSLOCTEXT("RA4", "kestrel_scout", "«Kestrel»")},
        {"AL_GuardianTank", NSLOCTEXT("RA4", "bulwark_mbt", "ОБТ «Bulwark»")},
        {"AL_BulwarkMBT", NSLOCTEXT("RA4", "bulwark_mbt", "ОБТ «Bulwark»")},
        {"AL_Aegis", NSLOCTEXT("RA4", "ward_shield", "Комплекс «Ward»")},
        {"AL_Athena", NSLOCTEXT("RA4", "oracle_artillery", "САУ «Oracle»")},
        {"AL_OracleArtillery", NSLOCTEXT("RA4", "oracle_artillery", "САУ «Oracle»")},

        // ---------------------------------------------------------------
        // Восточная коалиция — Китай (CN_). Прежние CO_-ключи остаются
        // LegacyAliases; японские и индийские мотивы вынесены в свои страны.
        // ---------------------------------------------------------------
        {"faction.eastern.building.construction_yard", NSLOCTEXT("RA4", "hq_cn", "Командный центр")},
        {"faction.eastern.building.power_plant", NSLOCTEXT("RA4", "power_plant_cn", "Солнечная электростанция")},
        {"faction.eastern.building.refinery", NSLOCTEXT("RA4", "refinery_cn", "Переработчик")},
        {"faction.eastern.building.barracks", NSLOCTEXT("RA4", "barracks_cn", "Учебный центр")},
        {"faction.eastern.building.war_factory", NSLOCTEXT("RA4", "war_factory_cn", "Завод робототехники")},
        {"faction.eastern.building.radar", NSLOCTEXT("RA4", "radar_cn", "Центр сетевого управления")},
        {"faction.eastern.building.turret", NSLOCTEXT("RA4", "turret_ai_cn", "Автотурель")},
        {"faction.eastern.building.aa_turret", NSLOCTEXT("RA4", "turret_aa_cn", "Зенитный комплекс")},
        {"faction.eastern.building.tech_center", NSLOCTEXT("RA4", "tech_center_cn", "Центр разработок")},

        {"faction.coalition.building.construction_yard", NSLOCTEXT("RA4", "hq_cn", "Командный центр")},
        {"faction.coalition.building.power_plant", NSLOCTEXT("RA4", "power_plant_cn", "Солнечная электростанция")},
        {"faction.coalition.building.refinery", NSLOCTEXT("RA4", "refinery_cn", "Переработчик")},
        {"faction.coalition.building.barracks", NSLOCTEXT("RA4", "barracks_cn", "Учебный центр")},
        {"faction.coalition.building.war_factory", NSLOCTEXT("RA4", "war_factory_cn", "Завод робототехники")},

        {"CN_Headquarters", NSLOCTEXT("RA4", "hq_cn", "Командный центр")},
        {"CN_PowerPlant", NSLOCTEXT("RA4", "power_plant_cn", "Солнечная электростанция")},
        {"CN_Refinery", NSLOCTEXT("RA4", "refinery_cn", "Переработчик")},
        {"CN_Barracks", NSLOCTEXT("RA4", "barracks_cn", "Учебный центр")},
        {"CN_WarFactory", NSLOCTEXT("RA4", "war_factory_cn", "Завод робототехники")},
        {"CO_ConYard", NSLOCTEXT("RA4", "hq_cn", "Командный центр")},
        {"CO_PowerPlant", NSLOCTEXT("RA4", "power_plant_cn", "Солнечная электростанция")},
        {"CO_Refinery", NSLOCTEXT("RA4", "refinery_cn", "Переработчик")},
        {"CO_Barracks", NSLOCTEXT("RA4", "barracks_cn", "Учебный центр")},
        {"CO_WarFactory", NSLOCTEXT("RA4", "war_factory_cn", "Завод робототехники")},

        {"faction.eastern.unit.mcv", NSLOCTEXT("RA4", "mcv_cn", "Мобильный узел")},
        {"faction.eastern.unit.harvester", NSLOCTEXT("RA4", "yuan_collector", "GRP-12 «Юань»")},
        {"faction.eastern.unit.basic_infantry", NSLOCTEXT("RA4", "qianwei_rifleman", "Тип 21 «Цяньвэй»")},
        {"faction.eastern.unit.main_tank", NSLOCTEXT("RA4", "qinglong_mbt", "ОБТ «Цинлун»")},
        {"faction.eastern.unit.artillery", NSLOCTEXT("RA4", "monsoon_artillery", "РСЗО «Муссон»")},
        {"faction.coalition.unit.basic_infantry", NSLOCTEXT("RA4", "qianwei_rifleman", "Тип 21 «Цяньвэй»")},
        {"faction.coalition.unit.main_tank", NSLOCTEXT("RA4", "qinglong_mbt", "ОБТ «Цинлун»")},
        {"CN_QianweiRifleman", NSLOCTEXT("RA4", "qianwei_rifleman", "Тип 21 «Цяньвэй»")},
        {"CN_YuanCollector", NSLOCTEXT("RA4", "yuan_collector", "GRP-12 «Юань»")},
        {"CN_QinglongMBT", NSLOCTEXT("RA4", "qinglong_mbt", "ОБТ «Цинлун»")},
        {"CN_MonsoonArtillery", NSLOCTEXT("RA4", "monsoon_artillery", "РСЗО «Муссон»")},
        {"CO_YuanCollector", NSLOCTEXT("RA4", "yuan_collector", "GRP-12 «Юань»")},
        {"CO_QinglongMBT", NSLOCTEXT("RA4", "qinglong_mbt", "ОБТ «Цинлун»")},
        {"CO_MonsoonArtillery", NSLOCTEXT("RA4", "monsoon_artillery", "РСЗО «Муссон»")},
        {"CO_Vanguard", NSLOCTEXT("RA4", "qianwei_rifleman", "Тип 21 «Цяньвэй»")},

        // ---------------------------------------------------------------
        // Тихоокеанский пакт — Япония (JP_).
        // ---------------------------------------------------------------
        {"faction.pacific.building.construction_yard", NSLOCTEXT("RA4", "hq_jp", "Островной командный терминал")},
        {"faction.pacific.building.power_plant", NSLOCTEXT("RA4", "power_plant_jp", "Прибрежная электростанция")},
        {"faction.pacific.building.refinery", NSLOCTEXT("RA4", "refinery_jp", "Переработчик")},
        {"faction.pacific.building.barracks", NSLOCTEXT("RA4", "barracks_jp", "Учебный центр")},
        {"faction.pacific.building.war_factory", NSLOCTEXT("RA4", "war_factory_jp", "Роботический цех")},
        {"faction.pacific.building.radar", NSLOCTEXT("RA4", "radar_jp", "Береговой радар")},
        {"faction.pacific.building.aa_turret", NSLOCTEXT("RA4", "turret_aa_jp", "Лазерная батарея перехвата")},
        {"JP_KamakiriWalker", NSLOCTEXT("RA4", "kamakiri_walker", "Шагоход «Камакири»")},
        {"JP_KawasemiScout", NSLOCTEXT("RA4", "kawasemi_scout", "«Кавасэми»")},
        {"CO_KamakiriWalker", NSLOCTEXT("RA4", "kamakiri_walker", "Шагоход «Камакири»")},

        // ---------------------------------------------------------------
        // Самостоятельные государства. Общей символики и общего блока нет.
        // ---------------------------------------------------------------
        {"faction.independent.building.construction_yard", NSLOCTEXT("RA4", "hq_indep", "Мобильный командный узел")},
        {"IN_VajraLancer", NSLOCTEXT("RA4", "vajra_lancer", "AT-8 «Ваджра»")},
        {"CO_StormLancer", NSLOCTEXT("RA4", "vajra_lancer", "AT-8 «Ваджра»")},

        // ---------------------------------------------------------------
        // Experimental / Legacy. В обычном мире недоступно; названия
        // сохранены только для модов, реплеев и особых режимов.
        // ---------------------------------------------------------------
        {"faction.legacy.building.construction_yard", NSLOCTEXT("RA4", "hq_legacy", "Центр причинности (Legacy)")},
        {"faction.chronolegion.building.construction_yard", NSLOCTEXT("RA4", "hq_legacy", "Центр причинности (Legacy)")},
        {"faction.chronolegion.building.power_plant", NSLOCTEXT("RA4", "power_plant_legacy", "Реактор замедленного распада (Legacy)")},
        {"faction.chronolegion.building.refinery", NSLOCTEXT("RA4", "refinery_legacy", "Квантовый переработчик (Legacy)")},
        {"faction.chronolegion.building.barracks", NSLOCTEXT("RA4", "barracks_legacy", "Казарма эха (Legacy)")},
        {"faction.chronolegion.building.war_factory", NSLOCTEXT("RA4", "war_factory_legacy", "Фабрика континуума (Legacy)")},
        {"CH_ConYard", NSLOCTEXT("RA4", "hq_legacy", "Центр причинности (Legacy)")},
        {"CH_PowerPlant", NSLOCTEXT("RA4", "power_plant_legacy", "Реактор замедленного распада (Legacy)")},
        {"CH_Refinery", NSLOCTEXT("RA4", "refinery_legacy", "Квантовый переработчик (Legacy)")},
        {"CH_Barracks", NSLOCTEXT("RA4", "barracks_legacy", "Казарма эха (Legacy)")},
        {"CH_WarFactory", NSLOCTEXT("RA4", "war_factory_legacy", "Фабрика континуума (Legacy)")},
        {"CH_ResonanceRifleman", NSLOCTEXT("RA4", "echo_rifleman_legacy", "ECHO-7 «Резонанс» (Legacy)")},
        {"CH_ProbabilistHarvester", NSLOCTEXT("RA4", "probabilist_legacy", "QH-4 «Вероятник» (Legacy)")},

        // ---------------------------------------------------------------
        // Нейтральные и общие ключи.
        // ---------------------------------------------------------------
        {"headquarters", NSLOCTEXT("RA4", "headquarters_generic", "Штаб")},
        {"power_plant", NSLOCTEXT("RA4", "power_generic", "Электростанция")},
        {"refinery", NSLOCTEXT("RA4", "refinery_generic", "Перерабатывающий комплекс")},
        {"barracks", NSLOCTEXT("RA4", "barracks_generic", "Казарма")},
        {"war_factory", NSLOCTEXT("RA4", "factory_generic", "Военный завод")},
        {"radar", NSLOCTEXT("RA4", "radar_generic", "Радарный узел")},

        // Content keys that reach the HUD through DefaultContent. Without these
        // the humaniser fell through and printed the raw id, so a build card
        // read "superweapon" in a Russian interface.
        {"building.superweapon", NSLOCTEXT("RA4", "superweapon_generic", "Ракетная шахта")},
        {"superweapon", NSLOCTEXT("RA4", "superweapon_generic", "Ракетная шахта")},
        {"faction.eurasian.building.superweapon", NSLOCTEXT("RA4", "superweapon_ru", "Ракетная шахта «Каратель»")},
        {"faction.soviet.building.superweapon", NSLOCTEXT("RA4", "superweapon_ru", "Ракетная шахта «Каратель»")},
        {"faction.atlantic.building.superweapon", NSLOCTEXT("RA4", "superweapon_atl", "Гиперзвуковой ударный комплекс")},
        {"faction.alliance.building.superweapon", NSLOCTEXT("RA4", "superweapon_atl", "Гиперзвуковой ударный комплекс")},
        {"faction.eastern.building.superweapon", NSLOCTEXT("RA4", "superweapon_cn", "Сейсмический комплекс")},
        {"faction.coalition.building.superweapon", NSLOCTEXT("RA4", "superweapon_cn", "Сейсмический комплекс")},
        {"faction.pacific.building.superweapon", NSLOCTEXT("RA4", "superweapon_jp", "Матрица перехвата")},

        // ---------------------------------------------------------------
        // Африканская федерация (African Federation)
        // ---------------------------------------------------------------
        {"faction.african.building.construction_yard", NSLOCTEXT("RA4", "hq_af", "Штабной модуль")},
        {"faction.african.building.power_plant", NSLOCTEXT("RA4", "power_plant_af", "Гелиотермальная станция «Гелиос»")},
        {"faction.african.building.refinery", NSLOCTEXT("RA4", "refinery_af", "Обогатитель «Сахель»")},
        {"faction.african.building.barracks", NSLOCTEXT("RA4", "barracks_af", "Тактический лагерь")},
        {"faction.african.building.war_factory", NSLOCTEXT("RA4", "war_factory_af", "Завод колёсной техники")},
        {"faction.african.building.radar", NSLOCTEXT("RA4", "radar_af", "Радарный узел «Атлас»")},
        {"faction.african.building.turret", NSLOCTEXT("RA4", "turret_sonic_af", "Акустический ДОТ")},
        {"faction.african.building.aa_turret", NSLOCTEXT("RA4", "turret_aa_af", "ЗРК «Зулу»")},
        {"faction.african.building.superweapon", NSLOCTEXT("RA4", "superweapon_af", "Орбитальный комплекс «Око Сахары»")},
        {"faction.african.unit.mcv", NSLOCTEXT("RA4", "mcv_af", "Штабной комплекс")},
        {"faction.african.unit.harvester", NSLOCTEXT("RA4", "harvester_af", "Комбайн «Саванна»")},
        {"faction.african.unit.basic_infantry", NSLOCTEXT("RA4", "askari_rifleman", "Стрелок «Аскари»")},
        {"faction.african.unit.antiarmor_infantry", NSLOCTEXT("RA4", "samum_atgm", "ПТРК «Самум»")},
        {"faction.african.unit.main_tank", NSLOCTEXT("RA4", "mamba_mbt", "ОБТ «Мамба»")},
        {"faction.african.unit.artillery", NSLOCTEXT("RA4", "baobab_artillery", "РСЗО «Баобаб»")},
        {"AU_MambaMBT", NSLOCTEXT("RA4", "mamba_mbt", "ОБТ «Мамба»")},
        {"AU_CheetahIFV", NSLOCTEXT("RA4", "cheetah_ifv", "БТР «Гепард»")},
        {"AU_ElephantSuperheavy", NSLOCTEXT("RA4", "elephant_heavy", "Платформа «Слон»")},
        {"AU_AskariRifleman", NSLOCTEXT("RA4", "askari_rifleman", "Стрелок «Аскари»")},
        {"AU_AminaCommando", NSLOCTEXT("RA4", "amina_commando", "Капитан Амина Диалло")}
    };

    auto It = LocMap.find(Key);
    if (It != LocMap.end())
    {
        return It->second;
    }

    // Humanize fallback: Never expose technical ID strings like faction.soviet... to the player
    FString KeyStr(UTF8_TO_TCHAR(Key.c_str()));

    // Extract last segment after dot or slash
    int32 LastDot = INDEX_NONE;
    if (KeyStr.FindLastChar(TEXT('.'), LastDot))
    {
        KeyStr = KeyStr.RightChop(LastDot + 1);
    }
    if (KeyStr.FindLastChar(TEXT('/'), LastDot))
    {
        KeyStr = KeyStr.RightChop(LastDot + 1);
    }

    // Replace underscores with spaces
    KeyStr.ReplaceInline(TEXT("_"), TEXT(" "));

    // Map common english terms to clean Russian RTS terminology
    if (KeyStr.Contains(TEXT("refinery"), ESearchCase::IgnoreCase)) KeyStr = TEXT("Перерабатывающий комплекс");
    else if (KeyStr.Contains(TEXT("power"), ESearchCase::IgnoreCase)) KeyStr = TEXT("Электростанция");
    else if (KeyStr.Contains(TEXT("barrack"), ESearchCase::IgnoreCase)) KeyStr = TEXT("Казарма");
    else if (KeyStr.Contains(TEXT("factory"), ESearchCase::IgnoreCase) || KeyStr.Contains(TEXT("war"), ESearchCase::IgnoreCase)) KeyStr = TEXT("Военный завод");
    else if (KeyStr.Contains(TEXT("yard"), ESearchCase::IgnoreCase) || KeyStr.Contains(TEXT("conyard"), ESearchCase::IgnoreCase) || KeyStr.Contains(TEXT("hq"), ESearchCase::IgnoreCase)) KeyStr = TEXT("Штаб");
    else if (KeyStr.Contains(TEXT("radar"), ESearchCase::IgnoreCase)) KeyStr = TEXT("Радарный узел");
    else if (KeyStr.Contains(TEXT("turret"), ESearchCase::IgnoreCase)) KeyStr = TEXT("Оборонительное орудие");
    else if (KeyStr.Contains(TEXT("tech"), ESearchCase::IgnoreCase)) KeyStr = TEXT("Центр разработок");
    else if (KeyStr.Contains(TEXT("harvester"), ESearchCase::IgnoreCase)) KeyStr = TEXT("Горнорудная машина");
    else if (KeyStr.Contains(TEXT("mcv"), ESearchCase::IgnoreCase)) KeyStr = TEXT("Командный модуль");
    else if (KeyStr.Contains(TEXT("tank"), ESearchCase::IgnoreCase)) KeyStr = TEXT("Тяжёлый танк");
    else if (KeyStr.Contains(TEXT("scout"), ESearchCase::IgnoreCase)) KeyStr = TEXT("Разведчик");
    else if (KeyStr.Contains(TEXT("infantry"), ESearchCase::IgnoreCase)) KeyStr = TEXT("Пехота");
    else if (KeyStr.Contains(TEXT("artillery"), ESearchCase::IgnoreCase)) KeyStr = TEXT("Артиллерия");
    else if (KeyStr.Contains(TEXT("superweapon"), ESearchCase::IgnoreCase)) KeyStr = TEXT("Стратегический комплекс");
    else if (KeyStr.Contains(TEXT("wall"), ESearchCase::IgnoreCase)) KeyStr = TEXT("Стена");
    else if (KeyStr.Contains(TEXT("defense"), ESearchCase::IgnoreCase)) KeyStr = TEXT("Оборонительный комплекс");

    return FText::FromString(KeyStr);
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
        case RP::AlertType::MCVDeployed: return NSLOCTEXT("RA4", "Alert_MCVDeployed", "Командный пункт развёрнут");
        case RP::AlertType::MCVUndeployed: return NSLOCTEXT("RA4", "Alert_MCVUndeployed", "Командный пункт свёрнут");
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

    // Secondary Ability / Special Mode
    bSelectionHasAbility = Snapshot.Selection.bPrimaryHasAbility;
    bSelectionAbilityActive = Snapshot.Selection.bPrimaryAbilityActive;
    SelectionAbilityCooldownRatio = Snapshot.Selection.PrimaryAbilityTotalCooldownTicks > 0
        ? float(Snapshot.Selection.PrimaryAbilityCooldownTicks) / float(Snapshot.Selection.PrimaryAbilityTotalCooldownTicks)
        : 0.0f;
    if (!Snapshot.Selection.PrimaryAbilityNameKey.empty())
    {
        SelectionAbilityName = KeyToText(Snapshot.Selection.PrimaryAbilityNameKey);
    }
    else
    {
        SelectionAbilityName = NSLOCTEXT("RA4", "Sidebar_DefaultAbility", "СПОСОБНОСТЬ [F]");
    }

    // Veterancy
    SelectionVeterancyRank = int32(Snapshot.Selection.PrimaryVeterancyRank);
    SelectionKillsValue = Snapshot.Selection.PrimaryKillsValue;

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

FText URA4UIDataProviderSubsystem::GetSelectionVeterancyText() const
{
    switch (SelectionVeterancyRank)
    {
    case 1:
        return NSLOCTEXT("RA4", "Rank_Veteran", "★ ВЕТЕРАН");
    case 2:
        return NSLOCTEXT("RA4", "Rank_Elite", "★★ ЭЛИТА");
    case 3:
        return NSLOCTEXT("RA4", "Rank_Heroic", "★★★ ГЕРОЙ");
    default:
        return FText::GetEmpty();
    }
}
