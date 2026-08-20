// Copyright (c) Red Alert 4 project.

#include "Components/Image.h"
#include "Misc/AutomationTest.h"
#include "RA4AngularPanelWidget.h"
#include "RA4MainMenuScreenWidget.h"
#include "RA4MainMenuViewModel.h"
#include "RA4ScreenRootWidget.h"
#include "RA4SplashScreenWidget.h"
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRA4ScreenRootConstructionTest,
    "RA4.UI.Widgets.Foundation.ScreenRootBuildsReferenceLayers",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRA4ScreenRootConstructionTest::RunTest(const FString& Parameters)
{
    URA4ScreenRootWidget* ScreenRoot = NewObject<URA4ScreenRootWidget>();
    TestTrue(TEXT("Screen root initializes"), ScreenRoot->Initialize());
    ScreenRoot->TakeWidget();

    TestNotNull(TEXT("Safe zone"), ScreenRoot->GetSafeZone());
    TestNotNull(TEXT("Background layer"), ScreenRoot->GetBackgroundLayer());
    TestNotNull(TEXT("Reference frame"), ScreenRoot->GetReferenceFrame());
    TestNotNull(TEXT("Chrome layer"), ScreenRoot->GetChromeLayer());
    TestNotNull(TEXT("Content layer"), ScreenRoot->GetContentLayer());

    ScreenRoot->ApplyScreenData(nullptr);
    TestFalse(TEXT("Missing data is reported"), ScreenRoot->GetValidationError().IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRA4AngularPanelRolePaddingTest,
    "RA4.UI.Widgets.Foundation.PanelRolesApplySemanticPadding",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRA4AngularPanelRolePaddingTest::RunTest(const FString& Parameters)
{
    URA4AngularPanelWidget* Panel = NewObject<URA4AngularPanelWidget>();

    Panel->SetPanelRole(ERA4PanelRole::Compact);
    TestEqual(TEXT("Compact padding"), Panel->GetPadding().Left, 8.0f);
    Panel->SetPanelRole(ERA4PanelRole::Standard);
    TestEqual(TEXT("Standard padding"), Panel->GetPadding().Left, 16.0f);
    Panel->SetPanelRole(ERA4PanelRole::DenseHUD);
    TestEqual(TEXT("Dense HUD padding"), Panel->GetPadding().Left, 10.0f);
    Panel->SetPanelRole(ERA4PanelRole::Hero);
    TestEqual(TEXT("Hero padding"), Panel->GetPadding().Left, 24.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRA4ActivatableInputContractTest,
    "RA4.UI.Widgets.Foundation.ActivatableWidgetUsesScreenInputPolicy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRA4ActivatableInputContractTest::RunTest(const FString& Parameters)
{
    URA4ScreenRootWidget* ScreenRoot = NewObject<URA4ScreenRootWidget>();

    ScreenRoot->SetScreenIdentity(ERA4UIScreenId::MainMenu);
    TestEqual(
        TEXT("Menu input"),
        ScreenRoot->GetDesiredInputConfig().GetValue().GetInputMode(),
        ECommonInputMode::Menu);

    ScreenRoot->SetScreenIdentity(
        ERA4UIScreenId::AlliesHud,
        ERA4UIScreenVariant::AlliesAir);
    TestEqual(
        TEXT("HUD input"),
        ScreenRoot->GetDesiredInputConfig().GetValue().GetInputMode(),
        ECommonInputMode::All);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRA4MainMenuEntriesTest,
    "RA4.UI.Screens.MainMenu.ViewModelProvidesEightOrderedEntries",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRA4MainMenuEntriesTest::RunTest(const FString& Parameters)
{
    const URA4MainMenuViewModel* ViewModel = NewObject<URA4MainMenuViewModel>();
    const TArray<FRA4MainMenuEntry>& Entries = ViewModel->GetMenuEntries();

    TestEqual(TEXT("Menu entry count"), Entries.Num(), 8);
    TestEqual(TEXT("First label"), Entries[0].Label.ToString(), FString(TEXT("КАМПАНИЯ")));
    TestEqual(TEXT("First route"), Entries[0].TargetScreen, ERA4UIScreenId::CampaignSelect);
    TestTrue(TEXT("First entry selected"), Entries[0].bSelected);
    TestEqual(TEXT("Last label"), Entries[7].Label.ToString(), FString(TEXT("ВЫХОД")));
    TestEqual(TEXT("Last action"), Entries[7].Action, ERA4MainMenuAction::Exit);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRA4SplashContinueOnceTest,
    "RA4.UI.Screens.MainMenu.SplashContinuesOnlyOnce",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRA4SplashContinueOnceTest::RunTest(const FString& Parameters)
{
    URA4SplashScreenWidget* Splash = NewObject<URA4SplashScreenWidget>();

    TestTrue(TEXT("First continue is accepted"), Splash->ContinueToMainMenu());
    TestFalse(TEXT("Second continue is ignored"), Splash->ContinueToMainMenu());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRA4MainMenuCompositionTest,
    "RA4.UI.Screens.MainMenu.CompositionUsesInteractiveWidgets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRA4MainMenuCompositionTest::RunTest(const FString& Parameters)
{
    URA4MainMenuScreenWidget* MainMenu = NewObject<URA4MainMenuScreenWidget>();
    TestTrue(TEXT("Main menu initializes"), MainMenu->Initialize());
    MainMenu->TakeWidget();

    TestEqual(TEXT("Interactive buttons"), MainMenu->GetMenuButtons().Num(), 8);
    TestEqual(TEXT("Selected entry"), MainMenu->GetSelectedMenuIndex(), 0);
    TestNotNull(TEXT("Separate logo widget"), MainMenu->GetLogoImage());
    TestEqual(
        TEXT("Logo ignores hit tests"),
        MainMenu->GetLogoImage()->GetVisibility(),
        ESlateVisibility::HitTestInvisible);
    return true;
}

#endif
