// Copyright (c) Red Alert 4 project.

#include "RA4UIScreenContract.h"

#include "RA4UIScreenCatalog.h"

namespace
{
bool IsVariantSupported(const ERA4UIScreenId Screen, const ERA4UIScreenVariant Variant)
{
    switch (Variant)
    {
    case ERA4UIScreenVariant::Default:
        return true;
    case ERA4UIScreenVariant::AlliesAlternate:
        return Screen == ERA4UIScreenId::AlliesCampaign;
    case ERA4UIScreenVariant::LoadingBriefing:
        return Screen == ERA4UIScreenId::Loading;
    case ERA4UIScreenVariant::EasternDetail:
        return Screen == ERA4UIScreenId::EasternCampaign;
    case ERA4UIScreenVariant::SovietBattle:
    case ERA4UIScreenVariant::SovietAlert:
        return Screen == ERA4UIScreenId::SovietHud;
    case ERA4UIScreenVariant::AlliesNaval:
    case ERA4UIScreenVariant::AlliesAir:
        return Screen == ERA4UIScreenId::AlliesHud;
    case ERA4UIScreenVariant::ChronoSuperweapon:
        return Screen == ERA4UIScreenId::ChronoHud;
    default:
        checkNoEntry();
        return false;
    }
}

RA4::UI::ScreenVariant ToCatalogVariant(const ERA4UIScreenVariant Variant)
{
    switch (Variant)
    {
    case ERA4UIScreenVariant::Default:
        return RA4::UI::ScreenVariant::Default;
    case ERA4UIScreenVariant::AlliesAlternate:
        return RA4::UI::ScreenVariant::AlliesAlternate;
    case ERA4UIScreenVariant::LoadingBriefing:
        return RA4::UI::ScreenVariant::LoadingBriefing;
    case ERA4UIScreenVariant::EasternDetail:
        return RA4::UI::ScreenVariant::EasternDetail;
    case ERA4UIScreenVariant::SovietBattle:
        return RA4::UI::ScreenVariant::SovietBattle;
    case ERA4UIScreenVariant::SovietAlert:
        return RA4::UI::ScreenVariant::SovietAlert;
    case ERA4UIScreenVariant::AlliesNaval:
        return RA4::UI::ScreenVariant::AlliesNaval;
    case ERA4UIScreenVariant::AlliesAir:
        return RA4::UI::ScreenVariant::AlliesAir;
    case ERA4UIScreenVariant::ChronoSuperweapon:
        return RA4::UI::ScreenVariant::ChronoSuperweapon;
    default:
        checkNoEntry();
        return RA4::UI::ScreenVariant::Default;
    }
}

TOptional<RA4::UI::ScreenId> ResolveCatalogScreenId(
    const ERA4UIScreenId Screen,
    const ERA4UIScreenVariant Variant)
{
    switch (Variant)
    {
    case ERA4UIScreenVariant::EasternDetail:
        return RA4::UI::ScreenId::EasternCampaignDetail;
    case ERA4UIScreenVariant::SovietBattle:
        return RA4::UI::ScreenId::SovietBattleHud;
    case ERA4UIScreenVariant::SovietAlert:
        return RA4::UI::ScreenId::SovietAlertHud;
    case ERA4UIScreenVariant::AlliesNaval:
        return RA4::UI::ScreenId::AlliesNavalHud;
    case ERA4UIScreenVariant::AlliesAir:
        return RA4::UI::ScreenId::AlliesAirHud;
    case ERA4UIScreenVariant::ChronoSuperweapon:
        return RA4::UI::ScreenId::ChronoSuperweaponHud;
    case ERA4UIScreenVariant::Default:
    case ERA4UIScreenVariant::AlliesAlternate:
    case ERA4UIScreenVariant::LoadingBriefing:
        break;
    default:
        checkNoEntry();
        return {};
    }

    switch (Screen)
    {
    case ERA4UIScreenId::Splash:
        return RA4::UI::ScreenId::Splash;
    case ERA4UIScreenId::MainMenu:
        return RA4::UI::ScreenId::MainMenu;
    case ERA4UIScreenId::CampaignSelect:
        return RA4::UI::ScreenId::CampaignSelect;
    case ERA4UIScreenId::SovietCampaign:
        return RA4::UI::ScreenId::SovietCampaign;
    case ERA4UIScreenId::AlliesCampaign:
        return RA4::UI::ScreenId::AlliesCampaign;
    case ERA4UIScreenId::EasternCampaign:
        return RA4::UI::ScreenId::EasternCampaign;
    case ERA4UIScreenId::ChronoCampaign:
        return RA4::UI::ScreenId::ChronoCampaign;
    case ERA4UIScreenId::MissionMap:
        return RA4::UI::ScreenId::SovietMissionMap;
    case ERA4UIScreenId::Briefing:
        return RA4::UI::ScreenId::SovietBriefing;
    case ERA4UIScreenId::VideoComms:
        return RA4::UI::ScreenId::VideoComms;
    case ERA4UIScreenId::Loading:
        return RA4::UI::ScreenId::SovietLoading;
    case ERA4UIScreenId::SovietHud:
        return RA4::UI::ScreenId::SovietHud;
    case ERA4UIScreenId::AlliesHud:
        return RA4::UI::ScreenId::AlliesHud;
    case ERA4UIScreenId::EasternHud:
        return RA4::UI::ScreenId::EasternHud;
    case ERA4UIScreenId::ChronoHud:
        return RA4::UI::ScreenId::ChronoHud;
    case ERA4UIScreenId::Pause:
        return RA4::UI::ScreenId::PauseMenu;
    case ERA4UIScreenId::Victory:
        return RA4::UI::ScreenId::Victory;
    case ERA4UIScreenId::MultiplayerLobby:
        return RA4::UI::ScreenId::MultiplayerLobby;
    case ERA4UIScreenId::Encyclopedia:
    case ERA4UIScreenId::TechTree:
    case ERA4UIScreenId::Mods:
    case ERA4UIScreenId::Settings:
        return {};
    default:
        checkNoEntry();
        return {};
    }
}

ERA4FactionTheme ToUnrealTheme(const RA4::UI::FactionTheme Theme)
{
    switch (Theme)
    {
    case RA4::UI::FactionTheme::EurasianPact:
        return ERA4FactionTheme::EurasianPact;
    case RA4::UI::FactionTheme::AtlanticAlliance:
        return ERA4FactionTheme::AtlanticAlliance;
    case RA4::UI::FactionTheme::EasternCoalition:
        return ERA4FactionTheme::EasternCoalition;
    case RA4::UI::FactionTheme::PacificPact:
        return ERA4FactionTheme::PacificPact;
    case RA4::UI::FactionTheme::Independent:
        return ERA4FactionTheme::Independent;
    case RA4::UI::FactionTheme::Chronolegion:
        return ERA4FactionTheme::Chronolegion;
    default:
        return ERA4FactionTheme::EurasianPact;
    }
}

ERA4UIScreenFamily ToUnrealFamily(const RA4::UI::ScreenFamily Family)
{
    switch (Family)
    {
    case RA4::UI::ScreenFamily::Splash:
        return ERA4UIScreenFamily::Splash;
    case RA4::UI::ScreenFamily::MainMenu:
        return ERA4UIScreenFamily::MainMenu;
    case RA4::UI::ScreenFamily::CampaignSelect:
        return ERA4UIScreenFamily::CampaignSelect;
    case RA4::UI::ScreenFamily::FactionCampaign:
        return ERA4UIScreenFamily::FactionCampaign;
    case RA4::UI::ScreenFamily::MissionMap:
        return ERA4UIScreenFamily::MissionMap;
    case RA4::UI::ScreenFamily::Briefing:
        return ERA4UIScreenFamily::Briefing;
    case RA4::UI::ScreenFamily::VideoComms:
        return ERA4UIScreenFamily::VideoComms;
    case RA4::UI::ScreenFamily::Loading:
        return ERA4UIScreenFamily::Loading;
    case RA4::UI::ScreenFamily::MultiplayerLobby:
        return ERA4UIScreenFamily::MultiplayerLobby;
    case RA4::UI::ScreenFamily::InGameHud:
        return ERA4UIScreenFamily::InGameHud;
    case RA4::UI::ScreenFamily::PauseMenu:
        return ERA4UIScreenFamily::PauseMenu;
    case RA4::UI::ScreenFamily::Victory:
        return ERA4UIScreenFamily::Victory;
    default:
        checkNoEntry();
        return ERA4UIScreenFamily::MainMenu;
    }
}

ERA4UIInputMode ToUnrealInputMode(const RA4::UI::InputPolicy Input)
{
    switch (Input)
    {
    case RA4::UI::InputPolicy::MenuOnly:
        return ERA4UIInputMode::UIOnly;
    case RA4::UI::InputPolicy::GameAndUI:
        return ERA4UIInputMode::GameAndUI;
    default:
        checkNoEntry();
        return ERA4UIInputMode::UIOnly;
    }
}

int32 FindReferenceNumber(
    const RA4::UI::ScreenId Id,
    const RA4::UI::ScreenVariant Variant)
{
    for (const RA4::UI::ScreenReferenceDefinition& Reference :
        RA4::UI::GetScreenReferenceCatalog())
    {
        if (Reference.Id == Id && Reference.Variant == Variant)
        {
            return Reference.ReferenceNumber;
        }
    }

    return 0;
}
} // namespace

FRA4UIScreenContract ResolveScreenContract(
    const ERA4UIScreenId Screen,
    const ERA4UIScreenVariant Variant)
{
    const ERA4UIScreenVariant NormalizedVariant =
        IsVariantSupported(Screen, Variant) ? Variant : ERA4UIScreenVariant::Default;

    FRA4UIScreenContract Contract;
    Contract.ScreenId = Screen;
    Contract.Variant = NormalizedVariant;

    const TOptional<RA4::UI::ScreenId> CatalogId =
        ResolveCatalogScreenId(Screen, NormalizedVariant);
    if (!CatalogId.IsSet())
    {
        Contract.Family = ERA4UIScreenFamily::MainMenu;
        Contract.InputMode = ERA4UIInputMode::UIOnly;
        return Contract;
    }

    const RA4::UI::ScreenDefinition* Definition = RA4::UI::FindScreen(CatalogId.GetValue());
    check(Definition != nullptr);
    Contract.ReferenceNumber = FindReferenceNumber(
        CatalogId.GetValue(),
        ToCatalogVariant(NormalizedVariant));
    Contract.Theme = ToUnrealTheme(Definition->Theme);
    Contract.Family = ToUnrealFamily(Definition->Family);
    Contract.InputMode = ToUnrealInputMode(Definition->Input);
    Contract.bIsHud = Definition->bIsHud;
    return Contract;
}
