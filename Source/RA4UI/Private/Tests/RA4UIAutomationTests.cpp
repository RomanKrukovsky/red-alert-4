// Copyright (c) Red Alert 4 project.

#include "Misc/AutomationTest.h"
#include "RA4UIScreenContract.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRA4ScreenContractReferenceVariantTest,
    "RA4.UI.Contracts.ReferenceVariantsResolve",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRA4ScreenContractReferenceVariantTest::RunTest(const FString& Parameters)
{
    const FRA4UIScreenContract AlliesAir = ResolveScreenContract(
        ERA4UIScreenId::AlliesHud,
        ERA4UIScreenVariant::AlliesAir);
    const FRA4UIScreenContract LoadingBriefing = ResolveScreenContract(
        ERA4UIScreenId::Loading,
        ERA4UIScreenVariant::LoadingBriefing);

    TestEqual(TEXT("Allies air reference"), AlliesAir.ReferenceNumber, 23);
    TestEqual(TEXT("Allies air theme"), AlliesAir.Theme, ERA4FactionTheme::Allies);
    TestEqual(TEXT("Allies air family"), AlliesAir.Family, ERA4UIScreenFamily::InGameHud);
    TestEqual(TEXT("Allies air input"), AlliesAir.InputMode, ERA4UIInputMode::GameAndUI);
    TestEqual(TEXT("Loading briefing reference"), LoadingBriefing.ReferenceNumber, 19);
    TestEqual(TEXT("Loading input"), LoadingBriefing.InputMode, ERA4UIInputMode::UIOnly);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRA4ScreenContractUnsupportedVariantTest,
    "RA4.UI.Contracts.UnsupportedVariantFallsBackToDefault",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRA4ScreenContractUnsupportedVariantTest::RunTest(const FString& Parameters)
{
    const FRA4UIScreenContract MainMenu = ResolveScreenContract(
        ERA4UIScreenId::MainMenu,
        ERA4UIScreenVariant::ChronoSuperweapon);

    TestEqual(TEXT("Main menu reference"), MainMenu.ReferenceNumber, 2);
    TestEqual(TEXT("Main menu variant"), MainMenu.Variant, ERA4UIScreenVariant::Default);
    TestEqual(TEXT("Main menu input"), MainMenu.InputMode, ERA4UIInputMode::UIOnly);
    return true;
}

#endif
