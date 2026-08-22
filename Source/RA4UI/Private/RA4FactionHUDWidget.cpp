// Copyright (c) Red Alert 4 project.

#include "RA4FactionHUDWidget.h"

#define LOCTEXT_NAMESPACE "RA4FactionHUDWidget"

URA4FactionHUDWidget::URA4FactionHUDWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    ConfigureReference(12);
}

void URA4FactionHUDWidget::NativePreConstruct()
{
    // A placed WBP uses its configured reference, while the showcase game mode can
    // still override the C++ default before the widget reaches the viewport.
    if (InitialReferenceNumber != 12 || ReferenceNumber == 12)
    {
        ConfigureReference(InitialReferenceNumber);
    }
    Super::NativePreConstruct();
}

bool URA4FactionHUDWidget::ConfigureReference(const int32 InReferenceNumber)
{
    switch (InReferenceNumber)
    {
    case 12:
        SetVariantData(12, ERA4FactionTheme::EurasianPact, ERA4UIScreenVariant::GroundAssault,
            1, TEXT("EurasianArmorPush"));
        return true;
    case 13:
        SetVariantData(13, ERA4FactionTheme::AtlanticAlliance, ERA4UIScreenVariant::NavalWarfare,
            4, TEXT("AtlanticCarrierGroup"));
        return true;
    case 14:
        SetVariantData(14, ERA4FactionTheme::EasternCoalition, ERA4UIScreenVariant::BaseDefense,
            0, TEXT("EasternIndustrialSiege"));
        return true;
    case 15:
        SetVariantData(15, ERA4FactionTheme::PacificPact, ERA4UIScreenVariant::AirWarfare,
            2, TEXT("PacificAirCampaign"));
        return true;
    case 16:
        SetVariantData(16, ERA4FactionTheme::Independent, ERA4UIScreenVariant::InsurgentFront,
            2, TEXT("IndependentSaturationStrike"));
        return true;
    case 17:
        SetVariantData(17, ERA4FactionTheme::EurasianPact, ERA4UIScreenVariant::BaseDefense,
            0, TEXT("EurasianBaseDefense"));
        return true;
    case 18:
        SetVariantData(18, ERA4FactionTheme::PacificPact, ERA4UIScreenVariant::BaseDefense,
            0, TEXT("PacificIslandDefense"));
        return true;
    default:
        return false;
    }
}

void URA4FactionHUDWidget::SetVariantData(
    const int32 InReferenceNumber,
    const ERA4FactionTheme Theme,
    const ERA4UIScreenVariant Variant,
    const int32 ActiveTab,
    const FName InSpecializedPanelId)
{
    ReferenceNumber = InReferenceNumber;
    SpecializedPanelId = InSpecializedPanelId;
    bShowSuperweaponPanel = Variant == ERA4UIScreenVariant::InsurgentFront;
    ConfigureHUD(Theme, Variant, ActiveTab);
    RebuildTabs();
}

void URA4FactionHUDWidget::RebuildTabs()
{
    ProductionTabs.Reset();
    switch (GetFactionTheme())
    {
    case ERA4FactionTheme::EurasianPact:
        ProductionTabs = {
            LOCTEXT("EurasianBuild", "СТРОИТЬ"), LOCTEXT("EurasianForces", "ВОЙСКА"),
            LOCTEXT("EurasianUpgrades", "УЛУЧШЕНИЯ"), LOCTEXT("EurasianDoctrines", "ДОКТРИНЫ")};
        break;
    case ERA4FactionTheme::AtlanticAlliance:
        ProductionTabs = {
            LOCTEXT("AtlanticStructures", "СТРОЕНИЯ"), LOCTEXT("AtlanticInfantry", "ПЕХОТА"),
            LOCTEXT("AtlanticVehicles", "ТЕХНИКА"), LOCTEXT("AtlanticAir", "АВИАЦИЯ")};
        if (GetHUDVariant() == ERA4UIScreenVariant::NavalWarfare)
        {
            ProductionTabs.Add(LOCTEXT("AtlanticFleet", "ФЛОТ"));
        }
        break;
    case ERA4FactionTheme::EasternCoalition:
        ProductionTabs = {
            LOCTEXT("EasternStructures", "СТРОЕНИЯ"), LOCTEXT("EasternUnits", "ДРОНЫ И ВОЙСКА"),
            LOCTEXT("EasternUpgrades", "ПРОИЗВОДСТВО"), LOCTEXT("EasternDoctrines", "ДОКТРИНЫ")};
        break;
    case ERA4FactionTheme::PacificPact:
        ProductionTabs = {
            LOCTEXT("PacificDef", "ОБОРОНА"), LOCTEXT("PacificRobots", "РОБОТОТЕХНИКА"),
            LOCTEXT("PacificAir", "АВИАЦИЯ"), LOCTEXT("PacificLasers", "ЛАЗЕРЫ")};
        break;
    case ERA4FactionTheme::Independent:
        ProductionTabs = {
            LOCTEXT("IndepShelter", "УКРЫТИЯ"), LOCTEXT("IndepSPU", "МОБИЛЬНЫЕ СПУ"),
            LOCTEXT("IndepDrones", "БПЛА «ШАХЕД»"), LOCTEXT("IndepDoctrines", "ДОКТРИНЫ")};
        break;
    case ERA4FactionTheme::Chronolegion:
        ProductionTabs = {
            LOCTEXT("ChronoStructures", "СТРОЕНИЯ"), LOCTEXT("ChronoUnits", "БОЕВЫЕ ЕДИНИЦЫ"),
            LOCTEXT("ChronoSupport", "ПОДДЕРЖКА"), LOCTEXT("ChronoSpecial", "ОСОБОЕ")};
        break;
    default:
        break;
    }
}

#undef LOCTEXT_NAMESPACE
