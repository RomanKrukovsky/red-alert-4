// Copyright (c) Red Alert 4 project.
// Pure C++ catalog intentionally kept independent of Unreal so CI can verify
// that every supplied visual reference has a navigable UI counterpart.

#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace RA4::UI
{
enum class FactionTheme : unsigned char
{
    USSR,
    Allies,
    EasternCoalition,
    Chronolegion
};

enum class ScreenId : unsigned char
{
    Splash,
    MainMenu,
    CampaignSelect,
    SovietCampaign,
    AlliesCampaign,
    EasternCampaign,
    ChronoCampaign,
    SovietMissionMap,
    SovietBriefing,
    VideoComms,
    SovietLoading,
    SovietHud,
    AlliesHud,
    EasternHud,
    ChronoHud,
    MultiplayerLobby,
    EasternCampaignDetail,
    SovietBattleHud,
    SovietAlertHud,
    AlliesNavalHud,
    AlliesAirHud,
    ChronoSuperweaponHud,
    PauseMenu,
    Victory
};

struct ScreenDefinition
{
    ScreenId Id;
    std::string_view Route;
    std::string_view RussianTitleKey;
    FactionTheme Theme;
    bool bIsHud;
};

inline constexpr std::array<ScreenDefinition, 24> ScreenCatalog = {{
    {ScreenId::Splash,                 "/ui/splash",                   "ui.splash.press_any_key",             FactionTheme::USSR,              false},
    {ScreenId::MainMenu,               "/ui/main-menu",                "ui.main_menu.title",                  FactionTheme::USSR,              false},
    {ScreenId::CampaignSelect,         "/ui/campaign-select",          "ui.campaign.select_faction",          FactionTheme::USSR,              false},
    {ScreenId::SovietCampaign,         "/ui/campaign/ussr",            "ui.campaign.ussr.title",              FactionTheme::USSR,              false},
    {ScreenId::AlliesCampaign,         "/ui/campaign/allies",          "ui.campaign.allies.title",            FactionTheme::Allies,            false},
    {ScreenId::EasternCampaign,        "/ui/campaign/eastern",         "ui.campaign.eastern.title",           FactionTheme::EasternCoalition,  false},
    {ScreenId::ChronoCampaign,         "/ui/campaign/chronolegion",    "ui.campaign.chrono.title",            FactionTheme::Chronolegion,      false},
    {ScreenId::SovietMissionMap,       "/ui/mission-map/ussr",         "ui.mission_map.title",                FactionTheme::USSR,              false},
    {ScreenId::SovietBriefing,         "/ui/briefing/ussr",            "ui.briefing.operation_data",          FactionTheme::USSR,              false},
    {ScreenId::VideoComms,             "/ui/video-comms",              "ui.video_comms.secure_channel",       FactionTheme::USSR,              false},
    {ScreenId::SovietLoading,          "/ui/loading/ussr",             "ui.loading.initializing",             FactionTheme::USSR,              false},
    {ScreenId::SovietHud,              "/ui/hud/ussr",                 "ui.hud.soviet.command",               FactionTheme::USSR,              true},
    {ScreenId::AlliesHud,              "/ui/hud/allies",               "ui.hud.allies.command",               FactionTheme::Allies,            true},
    {ScreenId::EasternHud,             "/ui/hud/eastern",              "ui.hud.eastern.command",              FactionTheme::EasternCoalition,  true},
    {ScreenId::ChronoHud,              "/ui/hud/chronolegion",         "ui.hud.chrono.command",               FactionTheme::Chronolegion,      true},
    {ScreenId::MultiplayerLobby,       "/ui/multiplayer/lobby",        "ui.lobby.title",                      FactionTheme::Allies,            false},
    {ScreenId::EasternCampaignDetail,  "/ui/campaign/eastern/detail",  "ui.campaign.eastern.commander",       FactionTheme::EasternCoalition,  false},
    {ScreenId::SovietBattleHud,        "/ui/hud/ussr/battle",          "ui.hud.battle",                       FactionTheme::USSR,              true},
    {ScreenId::SovietAlertHud,         "/ui/hud/ussr/alert",           "ui.hud.alert",                        FactionTheme::USSR,              true},
    {ScreenId::AlliesNavalHud,         "/ui/hud/allies/naval",         "ui.hud.allies.naval",                 FactionTheme::Allies,            true},
    {ScreenId::AlliesAirHud,           "/ui/hud/allies/air",           "ui.hud.allies.air",                   FactionTheme::Allies,            true},
    {ScreenId::ChronoSuperweaponHud,   "/ui/hud/chronolegion/weapon",  "ui.hud.chrono.superweapon",           FactionTheme::Chronolegion,      true},
    {ScreenId::PauseMenu,              "/ui/pause",                    "ui.pause.title",                      FactionTheme::USSR,              false},
    {ScreenId::Victory,                "/ui/victory",                  "ui.result.victory",                   FactionTheme::USSR,              false},
}};

inline constexpr const std::array<ScreenDefinition, 24>& GetScreenCatalog()
{
    return ScreenCatalog;
}

inline constexpr const ScreenDefinition* FindScreen(const ScreenId Id)
{
    for (const ScreenDefinition& Definition : ScreenCatalog)
    {
        if (Definition.Id == Id)
        {
            return &Definition;
        }
    }

    return nullptr;
}
} // namespace RA4::UI
