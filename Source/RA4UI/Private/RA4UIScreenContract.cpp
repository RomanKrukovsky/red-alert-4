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
    case ERA4UIScreenVariant::GroundAssault:
        return Screen == ERA4UIScreenId::EurasianHud;
    case ERA4UIScreenVariant::NavalWarfare:
        return Screen == ERA4UIScreenId::AtlanticHud;
    case ERA4UIScreenVariant::BaseDefense:
        return Screen == ERA4UIScreenId::EasternHud
            || Screen == ERA4UIScreenId::EurasianHud
            || Screen == ERA4UIScreenId::PacificHud;
    case ERA4UIScreenVariant::AirWarfare:
        return Screen == ERA4UIScreenId::PacificHud;
    case ERA4UIScreenVariant::InsurgentFront:
        return Screen == ERA4UIScreenId::IndependentHud;
    default:
        checkNoEntry();
        return false;
    }
}

/**
 * A faction HUD requested without a combat flavour still resolves to the
 * direction's signature reference, so every playable route lands on a real
 * screenshot instead of an unnamed screen.
 */
ERA4UIScreenVariant ResolveSignatureVariant(const ERA4UIScreenId Screen)
{
    switch (Screen)
    {
    case ERA4UIScreenId::EurasianHud:
        return ERA4UIScreenVariant::GroundAssault;
    case ERA4UIScreenId::AtlanticHud:
        return ERA4UIScreenVariant::NavalWarfare;
    case ERA4UIScreenId::EasternHud:
        return ERA4UIScreenVariant::BaseDefense;
    case ERA4UIScreenId::PacificHud:
        return ERA4UIScreenVariant::AirWarfare;
    case ERA4UIScreenId::IndependentHud:
        return ERA4UIScreenVariant::InsurgentFront;
    default:
        return ERA4UIScreenVariant::Default;
    }
}

RA4::UI::ScreenVariant ToCatalogVariant(const ERA4UIScreenVariant Variant)
{
    switch (Variant)
    {
    case ERA4UIScreenVariant::Default:
        return RA4::UI::ScreenVariant::Default;
    case ERA4UIScreenVariant::GroundAssault:
        return RA4::UI::ScreenVariant::GroundAssault;
    case ERA4UIScreenVariant::NavalWarfare:
        return RA4::UI::ScreenVariant::NavalWarfare;
    case ERA4UIScreenVariant::BaseDefense:
        return RA4::UI::ScreenVariant::BaseDefense;
    case ERA4UIScreenVariant::AirWarfare:
        return RA4::UI::ScreenVariant::AirWarfare;
    case ERA4UIScreenVariant::InsurgentFront:
        return RA4::UI::ScreenVariant::InsurgentFront;
    default:
        checkNoEntry();
        return RA4::UI::ScreenVariant::Default;
    }
}

TOptional<RA4::UI::ScreenId> ResolveCatalogScreenId(const ERA4UIScreenId Screen)
{
    switch (Screen)
    {
    case ERA4UIScreenId::Splash:
        return RA4::UI::ScreenId::Splash;
    case ERA4UIScreenId::MainMenu:
        return RA4::UI::ScreenId::MainMenu;
    case ERA4UIScreenId::CampaignSelect:
        return RA4::UI::ScreenId::CampaignSelect;
    case ERA4UIScreenId::EurasianCampaign:
        return RA4::UI::ScreenId::EurasianCampaign;
    case ERA4UIScreenId::AtlanticCampaign:
        return RA4::UI::ScreenId::AtlanticCampaign;
    case ERA4UIScreenId::EasternCampaign:
        return RA4::UI::ScreenId::EasternCampaign;
    case ERA4UIScreenId::PacificCampaign:
        return RA4::UI::ScreenId::PacificCampaign;
    case ERA4UIScreenId::IndependentCampaign:
        return RA4::UI::ScreenId::IndependentCampaign;
    case ERA4UIScreenId::MissionMap:
        return RA4::UI::ScreenId::MissionMap;
    case ERA4UIScreenId::Briefing:
        return RA4::UI::ScreenId::Briefing;
    case ERA4UIScreenId::VideoComms:
        return RA4::UI::ScreenId::VideoComms;
    case ERA4UIScreenId::Loading:
        return RA4::UI::ScreenId::Loading;
    case ERA4UIScreenId::MultiplayerLobby:
        return RA4::UI::ScreenId::MultiplayerLobby;
    case ERA4UIScreenId::EurasianHud:
        return RA4::UI::ScreenId::EurasianHud;
    case ERA4UIScreenId::AtlanticHud:
        return RA4::UI::ScreenId::AtlanticHud;
    case ERA4UIScreenId::EasternHud:
        return RA4::UI::ScreenId::EasternHud;
    case ERA4UIScreenId::PacificHud:
        return RA4::UI::ScreenId::PacificHud;
    case ERA4UIScreenId::IndependentHud:
        return RA4::UI::ScreenId::IndependentHud;
    case ERA4UIScreenId::Pause:
        return RA4::UI::ScreenId::PauseMenu;
    case ERA4UIScreenId::Victory:
        return RA4::UI::ScreenId::Victory;
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
    const ERA4UIScreenVariant RequestedVariant =
        IsVariantSupported(Screen, Variant) ? Variant : ERA4UIScreenVariant::Default;
    const ERA4UIScreenVariant NormalizedVariant = RequestedVariant == ERA4UIScreenVariant::Default
        ? ResolveSignatureVariant(Screen)
        : RequestedVariant;

    FRA4UIScreenContract Contract;
    Contract.ScreenId = Screen;
    Contract.Variant = RequestedVariant;

    const TOptional<RA4::UI::ScreenId> CatalogId = ResolveCatalogScreenId(Screen);
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
