// Copyright (c) Red Alert 4 project.

#include "RA4FactionHUDWidget.h"

#define LOCTEXT_NAMESPACE "RA4FactionHUDWidget"

URA4FactionHUDWidget::URA4FactionHUDWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    ConfigureReference(13);
}

void URA4FactionHUDWidget::NativePreConstruct()
{
    // A placed WBP uses its configured reference, while the showcase game mode can
    // still override the C++ default before the widget reaches the viewport.
    if (InitialReferenceNumber != 13 || ReferenceNumber == 13)
    {
        ConfigureReference(InitialReferenceNumber);
    }
    Super::NativePreConstruct();
}

bool URA4FactionHUDWidget::ConfigureReference(const int32 InReferenceNumber)
{
    switch (InReferenceNumber)
    {
    case 13:
        SetVariantData(13, ERA4FactionTheme::USSR, ERA4UIScreenVariant::Default,
            0, TEXT("SovietProduction"));
        return true;
    case 14:
        SetVariantData(14, ERA4FactionTheme::Allies, ERA4UIScreenVariant::Default,
            3, TEXT("AlliesCombinedArms"));
        return true;
    case 15:
        SetVariantData(15, ERA4FactionTheme::EasternCoalition, ERA4UIScreenVariant::Default,
            0, TEXT("EasternProduction"));
        return true;
    case 16:
        SetVariantData(16, ERA4FactionTheme::Chronolegion, ERA4UIScreenVariant::Default,
            0, TEXT("ChronoAbilities"));
        return true;
    case 20:
        SetVariantData(20, ERA4FactionTheme::USSR, ERA4UIScreenVariant::SovietBattle,
            2, TEXT("SovietArmorBattle"));
        return true;
    case 21:
        SetVariantData(21, ERA4FactionTheme::USSR, ERA4UIScreenVariant::SovietAlert,
            0, TEXT("SovietBaseAlert"));
        return true;
    case 22:
        SetVariantData(22, ERA4FactionTheme::Allies, ERA4UIScreenVariant::AlliesNaval,
            4, TEXT("AlliesNaval"));
        return true;
    case 23:
        SetVariantData(23, ERA4FactionTheme::Allies, ERA4UIScreenVariant::AlliesAir,
            3, TEXT("AlliesAir"));
        return true;
    case 24:
        SetVariantData(24, ERA4FactionTheme::Chronolegion, ERA4UIScreenVariant::ChronoSuperweapon,
            3, TEXT("ChronoSuperweapon"));
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
    bShowSuperweaponPanel = Variant == ERA4UIScreenVariant::ChronoSuperweapon;
    ConfigureHUD(Theme, Variant, ActiveTab);
    RebuildTabs();
}

void URA4FactionHUDWidget::RebuildTabs()
{
    ProductionTabs.Reset();
    switch (GetFactionTheme())
    {
    case ERA4FactionTheme::USSR:
        ProductionTabs = {
            LOCTEXT("SovietBuild", "СТРОИТЬ"), LOCTEXT("SovietForces", "ВОЙСКА"),
            LOCTEXT("SovietUpgrades", "УЛУЧШЕНИЯ"), LOCTEXT("SovietDoctrines", "ДОКТРИНЫ")};
        break;
    case ERA4FactionTheme::Allies:
        ProductionTabs = {
            LOCTEXT("AlliesStructures", "СТРОЕНИЯ"), LOCTEXT("AlliesInfantry", "ПЕХОТА"),
            LOCTEXT("AlliesVehicles", "ТЕХНИКА"), LOCTEXT("AlliesAir", "АВИАЦИЯ")};
        if (GetHUDVariant() == ERA4UIScreenVariant::AlliesNaval)
        {
            ProductionTabs.Add(LOCTEXT("AlliesFleet", "ФЛОТ"));
        }
        break;
    case ERA4FactionTheme::EasternCoalition:
        ProductionTabs = {
            LOCTEXT("EasternStructures", "СТРОЕНИЯ"), LOCTEXT("EasternUnits", "БОЕВЫЕ ЕД."),
            LOCTEXT("EasternUpgrades", "УЛУЧШЕНИЯ"), LOCTEXT("EasternDoctrines", "ДОКТРИНЫ")};
        break;
    case ERA4FactionTheme::Chronolegion:
        ProductionTabs = {
            LOCTEXT("ChronoStructures", "СТРОЕНИЯ"), LOCTEXT("ChronoUnits", "БОЕВЫЕ ЕДИНИЦЫ"),
            LOCTEXT("ChronoSupport", "ПОДДЕРЖКА"), LOCTEXT("ChronoSpecial", "ОСОБОЕ")};
        break;
    default:
        checkNoEntry();
        break;
    }
}

#undef LOCTEXT_NAMESPACE
