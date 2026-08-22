// Copyright (c) Scarlet Horizon project.
// Pure C++ catalog intentionally kept independent of Unreal so CI can verify
// that every supplied visual reference has a navigable UI counterpart.
//
// The catalog mirrors the ScarletHorizonRemaster reference set: nineteen
// screenshots covering five playable directions. Faction HUDs are one logical
// screen per direction; combat references inside one direction differ only by
// ScreenVariant, never by identity.

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

    // Retired direction kept so legacy art lookups keep resolving.
    Chronolegion = 5,

    // Aliases
    USSR = EurasianPact,
    Allies = AtlanticAlliance
};

enum class ScreenId : unsigned char
{
    Splash = 0,
    MainMenu,
    CampaignSelect,
    EurasianCampaign,
    AtlanticCampaign,
    EasternCampaign,
    PacificCampaign,
    IndependentCampaign,
    MissionMap,
    Briefing,
    VideoComms,
    Loading,
    MultiplayerLobby,
    EurasianHud,
    AtlanticHud,
    EasternHud,
    PacificHud,
    IndependentHud,
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
    GroundAssault,
    NavalWarfare,
    BaseDefense,
    AirWarfare,
    InsurgentFront
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

inline constexpr std::array<ScreenDefinition, 20> ScreenCatalog = {{
    {ScreenId::Splash,                 "/ui/splash",                      "ui.splash.press_any_key",        FactionTheme::EurasianPact,      ScreenFamily::Splash,           InputPolicy::MenuOnly,  false},
    {ScreenId::MainMenu,               "/ui/main-menu",                   "ui.main_menu.title",             FactionTheme::EurasianPact,      ScreenFamily::MainMenu,         InputPolicy::MenuOnly,  false},
    {ScreenId::CampaignSelect,         "/ui/campaign-select",             "ui.campaign.select_faction",     FactionTheme::EurasianPact,      ScreenFamily::CampaignSelect,   InputPolicy::MenuOnly,  false},
    {ScreenId::EurasianCampaign,       "/ui/campaign/eurasian-pact",      "ui.campaign.eurasian.title",     FactionTheme::EurasianPact,      ScreenFamily::FactionCampaign,  InputPolicy::MenuOnly,  false},
    {ScreenId::AtlanticCampaign,       "/ui/campaign/atlantic-alliance",  "ui.campaign.atlantic.title",     FactionTheme::AtlanticAlliance,  ScreenFamily::FactionCampaign,  InputPolicy::MenuOnly,  false},
    {ScreenId::EasternCampaign,        "/ui/campaign/eastern-coalition",  "ui.campaign.eastern.title",      FactionTheme::EasternCoalition,  ScreenFamily::FactionCampaign,  InputPolicy::MenuOnly,  false},
    {ScreenId::PacificCampaign,        "/ui/campaign/pacific-pact",       "ui.campaign.pacific.title",      FactionTheme::PacificPact,       ScreenFamily::FactionCampaign,  InputPolicy::MenuOnly,  false},
    {ScreenId::IndependentCampaign,    "/ui/campaign/independent",        "ui.campaign.independent.title",  FactionTheme::Independent,       ScreenFamily::FactionCampaign,  InputPolicy::MenuOnly,  false},
    {ScreenId::MissionMap,             "/ui/mission-map/eurasian-pact",   "ui.mission_map.title",           FactionTheme::EurasianPact,      ScreenFamily::MissionMap,       InputPolicy::MenuOnly,  false},
    {ScreenId::Briefing,               "/ui/briefing/quiet-relay",        "ui.briefing.operation_data",     FactionTheme::EurasianPact,      ScreenFamily::Briefing,         InputPolicy::MenuOnly,  false},
    {ScreenId::VideoComms,             "/ui/video-comms/secure-channel",  "ui.video_comms.secure_channel",  FactionTheme::EurasianPact,      ScreenFamily::VideoComms,       InputPolicy::MenuOnly,  false},
    {ScreenId::Loading,                "/ui/loading/bars",                "ui.loading.initializing",        FactionTheme::EurasianPact,      ScreenFamily::Loading,          InputPolicy::MenuOnly,  false},
    {ScreenId::MultiplayerLobby,       "/ui/multiplayer/lobby",           "ui.lobby.title",                 FactionTheme::EurasianPact,      ScreenFamily::MultiplayerLobby, InputPolicy::MenuOnly,  false},
    {ScreenId::EurasianHud,            "/ui/hud/eurasian-pact",           "ui.hud.eurasian.command",        FactionTheme::EurasianPact,      ScreenFamily::InGameHud,        InputPolicy::GameAndUI, true},
    {ScreenId::AtlanticHud,            "/ui/hud/atlantic-alliance",       "ui.hud.atlantic.command",        FactionTheme::AtlanticAlliance,  ScreenFamily::InGameHud,        InputPolicy::GameAndUI, true},
    {ScreenId::EasternHud,             "/ui/hud/eastern-coalition",       "ui.hud.eastern.command",         FactionTheme::EasternCoalition,  ScreenFamily::InGameHud,        InputPolicy::GameAndUI, true},
    {ScreenId::PacificHud,             "/ui/hud/pacific-pact",            "ui.hud.pacific.command",         FactionTheme::PacificPact,       ScreenFamily::InGameHud,        InputPolicy::GameAndUI, true},
    {ScreenId::IndependentHud,         "/ui/hud/independent",             "ui.hud.independent.command",     FactionTheme::Independent,       ScreenFamily::InGameHud,        InputPolicy::GameAndUI, true},
    {ScreenId::PauseMenu,              "/ui/pause",                       "ui.pause.title",                 FactionTheme::EurasianPact,      ScreenFamily::PauseMenu,        InputPolicy::MenuOnly,  false},
    {ScreenId::Victory,                "/ui/victory",                     "ui.result.victory",              FactionTheme::EurasianPact,      ScreenFamily::Victory,          InputPolicy::MenuOnly,  false}
}};

inline constexpr std::array<ScreenReferenceDefinition, 19> ScreenReferenceCatalog = {{
    {1,  ScreenId::Splash,             ScreenVariant::Default},
    {2,  ScreenId::MainMenu,           ScreenVariant::Default},
    {3,  ScreenId::EurasianCampaign,   ScreenVariant::Default},
    {4,  ScreenId::AtlanticCampaign,   ScreenVariant::Default},
    {5,  ScreenId::EasternCampaign,    ScreenVariant::Default},
    {6,  ScreenId::PacificCampaign,    ScreenVariant::Default},
    {7,  ScreenId::MissionMap,         ScreenVariant::Default},
    {8,  ScreenId::Briefing,           ScreenVariant::Default},
    {9,  ScreenId::VideoComms,         ScreenVariant::Default},
    {10, ScreenId::Loading,            ScreenVariant::Default},
    {11, ScreenId::MultiplayerLobby,   ScreenVariant::Default},
    {12, ScreenId::EurasianHud,        ScreenVariant::GroundAssault},
    {13, ScreenId::AtlanticHud,        ScreenVariant::NavalWarfare},
    {14, ScreenId::EasternHud,         ScreenVariant::BaseDefense},
    {15, ScreenId::PacificHud,         ScreenVariant::AirWarfare},
    {16, ScreenId::IndependentHud,     ScreenVariant::InsurgentFront},
    {17, ScreenId::EurasianHud,        ScreenVariant::BaseDefense},
    {18, ScreenId::PacificHud,         ScreenVariant::BaseDefense},
    {19, ScreenId::IndependentCampaign, ScreenVariant::Default}
}};

inline constexpr const std::array<ScreenDefinition, 20>& GetScreenCatalog()
{
    return ScreenCatalog;
}

inline constexpr const std::array<ScreenReferenceDefinition, 19>& GetScreenReferenceCatalog()
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
