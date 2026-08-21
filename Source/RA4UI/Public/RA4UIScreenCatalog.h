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
    EurasianPact = 0,
    AtlanticAlliance = 1,
    EasternCoalition = 2,
    PacificPact = 3,
    Independent = 4,
    Chronolegion = 5,

    // Aliases
    USSR = EurasianPact,
    Allies = AtlanticAlliance
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

enum class ScreenFamily : unsigned char
{
    Splash,
    MainMenu,
    CampaignSelect,
    FactionCampaign,
    MissionMap,
    Briefing,
    VideoComms,
    Loading,
    MultiplayerLobby,
    InGameHud,
    PauseMenu,
    Victory
};

enum class ScreenVariant : unsigned char
{
    Default,
    AlliesAlternate,
    LoadingBriefing,
    EasternDetail,
    SovietBattle,
    SovietAlert,
    AlliesNaval,
    AlliesAir,
    ChronoSuperweapon
};

enum class InputPolicy : unsigned char
{
    MenuOnly,
    GameAndUI
};

struct ScreenDefinition
{
    ScreenId Id;
    std::string_view Route;
    std::string_view RussianTitleKey;
    FactionTheme Theme;
    ScreenFamily Family;
    InputPolicy Input;
    bool bIsHud;
};

struct ScreenReferenceDefinition
{
    int ReferenceNumber;
    ScreenId Id;
    ScreenVariant Variant;
};

inline constexpr std::array<ScreenDefinition, 24> ScreenCatalog = {{
    {ScreenId::Splash,                 "/ui/splash",                   "ui.splash.press_any_key",             FactionTheme::USSR,              ScreenFamily::Splash,             InputPolicy::MenuOnly,  false},
    {ScreenId::MainMenu,               "/ui/main-menu",                "ui.main_menu.title",                  FactionTheme::USSR,              ScreenFamily::MainMenu,           InputPolicy::MenuOnly,  false},
    {ScreenId::CampaignSelect,         "/ui/campaign-select",          "ui.campaign.select_faction",          FactionTheme::USSR,              ScreenFamily::CampaignSelect,     InputPolicy::MenuOnly,  false},
    {ScreenId::SovietCampaign,         "/ui/campaign/ussr",            "ui.campaign.ussr.title",              FactionTheme::USSR,              ScreenFamily::FactionCampaign,    InputPolicy::MenuOnly,  false},
    {ScreenId::AlliesCampaign,         "/ui/campaign/allies",          "ui.campaign.allies.title",            FactionTheme::Allies,            ScreenFamily::FactionCampaign,    InputPolicy::MenuOnly,  false},
    {ScreenId::EasternCampaign,        "/ui/campaign/eastern",         "ui.campaign.eastern.title",           FactionTheme::EasternCoalition,  ScreenFamily::FactionCampaign,    InputPolicy::MenuOnly,  false},
    {ScreenId::ChronoCampaign,         "/ui/campaign/chronolegion",    "ui.campaign.chrono.title",            FactionTheme::Chronolegion,      ScreenFamily::FactionCampaign,    InputPolicy::MenuOnly,  false},
    {ScreenId::SovietMissionMap,       "/ui/mission-map/ussr",         "ui.mission_map.title",                FactionTheme::USSR,              ScreenFamily::MissionMap,         InputPolicy::MenuOnly,  false},
    {ScreenId::SovietBriefing,         "/ui/briefing/ussr",            "ui.briefing.operation_data",          FactionTheme::USSR,              ScreenFamily::Briefing,           InputPolicy::MenuOnly,  false},
    {ScreenId::VideoComms,             "/ui/video-comms",              "ui.video_comms.secure_channel",       FactionTheme::USSR,              ScreenFamily::VideoComms,         InputPolicy::MenuOnly,  false},
    {ScreenId::SovietLoading,          "/ui/loading/ussr",             "ui.loading.initializing",             FactionTheme::USSR,              ScreenFamily::Loading,            InputPolicy::MenuOnly,  false},
    {ScreenId::SovietHud,              "/ui/hud/ussr",                 "ui.hud.soviet.command",               FactionTheme::USSR,              ScreenFamily::InGameHud,          InputPolicy::GameAndUI, true},
    {ScreenId::AlliesHud,              "/ui/hud/allies",               "ui.hud.allies.command",               FactionTheme::Allies,            ScreenFamily::InGameHud,          InputPolicy::GameAndUI, true},
    {ScreenId::EasternHud,             "/ui/hud/eastern",              "ui.hud.eastern.command",              FactionTheme::EasternCoalition,  ScreenFamily::InGameHud,          InputPolicy::GameAndUI, true},
    {ScreenId::ChronoHud,              "/ui/hud/chronolegion",         "ui.hud.chrono.command",               FactionTheme::Chronolegion,      ScreenFamily::InGameHud,          InputPolicy::GameAndUI, true},
    {ScreenId::MultiplayerLobby,       "/ui/multiplayer/lobby",        "ui.lobby.title",                      FactionTheme::Allies,            ScreenFamily::MultiplayerLobby,   InputPolicy::MenuOnly,  false},
    {ScreenId::EasternCampaignDetail,  "/ui/campaign/eastern/detail",  "ui.campaign.eastern.commander",       FactionTheme::EasternCoalition,  ScreenFamily::FactionCampaign,    InputPolicy::MenuOnly,  false},
    {ScreenId::SovietBattleHud,        "/ui/hud/ussr/battle",          "ui.hud.battle",                       FactionTheme::USSR,              ScreenFamily::InGameHud,          InputPolicy::GameAndUI, true},
    {ScreenId::SovietAlertHud,         "/ui/hud/ussr/alert",           "ui.hud.alert",                        FactionTheme::USSR,              ScreenFamily::InGameHud,          InputPolicy::GameAndUI, true},
    {ScreenId::AlliesNavalHud,         "/ui/hud/allies/naval",         "ui.hud.allies.naval",                 FactionTheme::Allies,            ScreenFamily::InGameHud,          InputPolicy::GameAndUI, true},
    {ScreenId::AlliesAirHud,           "/ui/hud/allies/air",           "ui.hud.allies.air",                   FactionTheme::Allies,            ScreenFamily::InGameHud,          InputPolicy::GameAndUI, true},
    {ScreenId::ChronoSuperweaponHud,   "/ui/hud/chronolegion/weapon",  "ui.hud.chrono.superweapon",           FactionTheme::Chronolegion,      ScreenFamily::InGameHud,          InputPolicy::GameAndUI, true},
    {ScreenId::PauseMenu,              "/ui/pause",                    "ui.pause.title",                      FactionTheme::USSR,              ScreenFamily::PauseMenu,          InputPolicy::MenuOnly,  false},
    {ScreenId::Victory,                "/ui/victory",                  "ui.result.victory",                   FactionTheme::USSR,              ScreenFamily::Victory,            InputPolicy::MenuOnly,  false},
}};

inline constexpr std::array<ScreenReferenceDefinition, 24> ScreenReferenceCatalog = {{
    {1,  ScreenId::Splash,                ScreenVariant::Default},
    {2,  ScreenId::MainMenu,              ScreenVariant::Default},
    {3,  ScreenId::CampaignSelect,        ScreenVariant::Default},
    {4,  ScreenId::SovietCampaign,        ScreenVariant::Default},
    {5,  ScreenId::AlliesCampaign,        ScreenVariant::Default},
    {6,  ScreenId::EasternCampaign,       ScreenVariant::Default},
    {7,  ScreenId::ChronoCampaign,        ScreenVariant::Default},
    {8,  ScreenId::SovietMissionMap,      ScreenVariant::Default},
    {9,  ScreenId::SovietBriefing,        ScreenVariant::Default},
    {10, ScreenId::VideoComms,            ScreenVariant::Default},
    {11, ScreenId::AlliesCampaign,        ScreenVariant::AlliesAlternate},
    {12, ScreenId::SovietLoading,         ScreenVariant::Default},
    {13, ScreenId::SovietHud,             ScreenVariant::Default},
    {14, ScreenId::AlliesHud,             ScreenVariant::Default},
    {15, ScreenId::EasternHud,            ScreenVariant::Default},
    {16, ScreenId::ChronoHud,             ScreenVariant::Default},
    {17, ScreenId::MultiplayerLobby,      ScreenVariant::Default},
    {18, ScreenId::EasternCampaignDetail, ScreenVariant::EasternDetail},
    {19, ScreenId::SovietLoading,         ScreenVariant::LoadingBriefing},
    {20, ScreenId::SovietBattleHud,       ScreenVariant::SovietBattle},
    {21, ScreenId::SovietAlertHud,        ScreenVariant::SovietAlert},
    {22, ScreenId::AlliesNavalHud,        ScreenVariant::AlliesNaval},
    {23, ScreenId::AlliesAirHud,          ScreenVariant::AlliesAir},
    {24, ScreenId::ChronoSuperweaponHud,  ScreenVariant::ChronoSuperweapon},
}};

inline constexpr const std::array<ScreenDefinition, 24>& GetScreenCatalog()
{
    return ScreenCatalog;
}

inline constexpr const std::array<ScreenReferenceDefinition, 24>& GetScreenReferenceCatalog()
{
    return ScreenReferenceCatalog;
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

inline constexpr const ScreenReferenceDefinition* FindScreenByReference(const int ReferenceNumber)
{
    for (const ScreenReferenceDefinition& Reference : ScreenReferenceCatalog)
    {
        if (Reference.ReferenceNumber == ReferenceNumber)
        {
            return &Reference;
        }
    }

    return nullptr;
}
} // namespace RA4::UI
