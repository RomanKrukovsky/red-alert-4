// Copyright (c) Red Alert 4 project.

#include "Components/Image.h"
#include "Misc/AutomationTest.h"
#include "RA4AngularPanelWidget.h"
#include "RA4CampaignScreenWidget.h"
#include "RA4CampaignViewModel.h"
#include "RA4HUDViewModel.h"
#include "RA4HUDWidget.h"
#include "RA4MainMenuScreenWidget.h"
#include "RA4MainMenuViewModel.h"
#include "RA4LobbyScreenWidget.h"
#include "RA4LobbyViewModel.h"
#include "RA4MinimapWidget.h"
#include "RA4MissionFlowWidgets.h"
#include "RA4ScreenRootWidget.h"
#include "RA4SplashScreenWidget.h"
#include "RA4UIScreenContract.h"
#include "Slate/SRA4Minimap.h"
#include "Slate/SRA4WorldMarkerLayer.h"

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRA4CampaignViewModelContentTest,
    "RA4.UI.Screens.Campaign.ViewModelProvidesPlayableContent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRA4CampaignViewModelContentTest::RunTest(const FString& Parameters)
{
    URA4CampaignViewModel* ViewModel = NewObject<URA4CampaignViewModel>();

    TestEqual(TEXT("Faction count"), ViewModel->GetFactionCards().Num(), 4);
    TestEqual(TEXT("Initial faction"), ViewModel->GetSelectedFaction(), ERA4FactionTheme::USSR);
    TestTrue(TEXT("USSR has missions"), ViewModel->GetMissionNodes().Num() >= 8);
    TestTrue(TEXT("Known mission can be selected"), ViewModel->SelectMission(TEXT("ussr_operation_molot")));
    TestEqual(TEXT("Selected mission"), ViewModel->GetSelectedMissionId(), FName(TEXT("ussr_operation_molot")));
    TestFalse(TEXT("Unknown mission is rejected"), ViewModel->SelectMission(TEXT("missing_content")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRA4CampaignViewModelProgressTest,
    "RA4.UI.Screens.Campaign.ProgressAndLocksAreValidated",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRA4CampaignViewModelProgressTest::RunTest(const FString& Parameters)
{
    URA4CampaignViewModel* ViewModel = NewObject<URA4CampaignViewModel>();

    TestTrue(TEXT("Known faction progress updates"), ViewModel->SetCampaignProgress(
        ERA4FactionTheme::USSR, 99, 18));
    TestEqual(TEXT("Completed missions clamp"), ViewModel->GetFactionCards()[0].CompletedMissions, 18);
    TestEqual(TEXT("Progress clamps"), ViewModel->GetFactionCards()[0].Progress, 1.0f);
    TestFalse(TEXT("Locked mission cannot be selected"), ViewModel->SelectMission(TEXT("ussr_final_protocol")));
    TestTrue(TEXT("Unlocked faction can be selected"), ViewModel->SelectFaction(ERA4FactionTheme::Allies));
    TestEqual(TEXT("Allies selected"), ViewModel->GetSelectedFaction(), ERA4FactionTheme::Allies);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRA4CampaignReferenceVariantsTest,
    "RA4.UI.Screens.Campaign.DuplicateReferencesKeepDistinctVariants",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRA4CampaignReferenceVariantsTest::RunTest(const FString& Parameters)
{
    const FRA4UIScreenContract AlliesDefault = ResolveScreenContract(ERA4UIScreenId::AlliesCampaign);
    const FRA4UIScreenContract AlliesAlternate = ResolveScreenContract(
        ERA4UIScreenId::AlliesCampaign, ERA4UIScreenVariant::AlliesAlternate);
    const FRA4UIScreenContract LoadingDefault = ResolveScreenContract(ERA4UIScreenId::Loading);
    const FRA4UIScreenContract LoadingBriefing = ResolveScreenContract(
        ERA4UIScreenId::Loading, ERA4UIScreenVariant::LoadingBriefing);

    TestEqual(TEXT("Allies default reference"), AlliesDefault.ReferenceNumber, 5);
    TestEqual(TEXT("Allies alternate reference"), AlliesAlternate.ReferenceNumber, 11);
    TestEqual(TEXT("Loading default reference"), LoadingDefault.ReferenceNumber, 12);
    TestEqual(TEXT("Loading briefing reference"), LoadingBriefing.ReferenceNumber, 19);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRA4CampaignScreenCompositionTest,
    "RA4.UI.Screens.Campaign.FactionScreenUsesSharedInteractiveComposition",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRA4CampaignScreenCompositionTest::RunTest(const FString& Parameters)
{
    URA4CampaignScreenWidget* Screen = NewObject<URA4CampaignScreenWidget>();
    Screen->ConfigureCampaign(ERA4FactionTheme::EasternCoalition, ERA4UIScreenVariant::EasternDetail);
    TestTrue(TEXT("Campaign screen initializes"), Screen->Initialize());
    Screen->TakeWidget();

    TestEqual(TEXT("Configured faction"), Screen->GetFactionTheme(), ERA4FactionTheme::EasternCoalition);
    TestEqual(TEXT("Configured variant"), Screen->GetScreenVariant(), ERA4UIScreenVariant::EasternDetail);
    TestEqual(TEXT("Primary campaign actions"), Screen->GetActionButtons().Num(), 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRA4MissionFlowCompositionTest,
    "RA4.UI.Screens.Campaign.MissionFlowUsesRealWidgets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRA4MissionFlowCompositionTest::RunTest(const FString& Parameters)
{
    URA4MissionMapScreenWidget* MissionMap = NewObject<URA4MissionMapScreenWidget>();
    TestTrue(TEXT("Mission map initializes"), MissionMap->Initialize());
    MissionMap->TakeWidget();
    TestTrue(TEXT("Mission nodes are buttons"), MissionMap->GetMissionButtons().Num() >= 8);

    URA4BriefingScreenWidget* Briefing = NewObject<URA4BriefingScreenWidget>();
    TestTrue(TEXT("Briefing initializes"), Briefing->Initialize());
    Briefing->TakeWidget();
    TestNotNull(TEXT("Briefing continue action"), Briefing->GetContinueButton());

    URA4VideoCommsScreenWidget* Comms = NewObject<URA4VideoCommsScreenWidget>();
    TestTrue(TEXT("Video comms initializes"), Comms->Initialize());
    Comms->TakeWidget();
    TestNotNull(TEXT("Comms end session action"), Comms->GetEndSessionButton());

    URA4LoadingScreenWidget* Loading = NewObject<URA4LoadingScreenWidget>();
    Loading->SetLoadingProgress(2.0f);
    TestEqual(TEXT("Loading progress clamps"), Loading->GetLoadingProgress(), 1.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRA4LobbyViewModelValidationTest,
    "RA4.UI.Screens.Lobby.ValidatesPlayersAndHostStart",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRA4LobbyViewModelValidationTest::RunTest(const FString& Parameters)
{
    URA4LobbyViewModel* ViewModel = NewObject<URA4LobbyViewModel>();

    TestEqual(TEXT("Eight lobby slots"), ViewModel->GetPlayers().Num(), 8);
    TestTrue(TEXT("Host can start ready lobby"), ViewModel->CanStartMatch());
    TestFalse(TEXT("Duplicate color is rejected"), ViewModel->ChangeColor(TEXT("allied_command"), 0));
    TestFalse(TEXT("Invalid team is rejected"), ViewModel->ChangeTeam(TEXT("allied_command"), 0));
    TestTrue(TEXT("Player can become not ready"), ViewModel->SetReady(TEXT("allied_command"), false));
    TestFalse(TEXT("Not-ready player blocks start"), ViewModel->CanStartMatch());
    ViewModel->SetLocalHost(false);
    TestFalse(TEXT("Non-host cannot start"), ViewModel->StartMatch());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRA4LobbyViewModelChatTest,
    "RA4.UI.Screens.Lobby.ChatAndDisconnectState",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRA4LobbyViewModelChatTest::RunTest(const FString& Parameters)
{
    URA4LobbyViewModel* ViewModel = NewObject<URA4LobbyViewModel>();

    TestFalse(TEXT("Empty chat rejected"), ViewModel->SendChat(TEXT("   ")));
    TestTrue(TEXT("Chat accepted"), ViewModel->SendChat(TEXT("Готов к бою")));
    TestEqual(TEXT("Chat appended"), ViewModel->GetChatMessages().Num(), 8);
    ViewModel->HandleDisconnected();
    TestFalse(TEXT("Disconnected lobby cannot start"), ViewModel->CanStartMatch());
    TestTrue(TEXT("Disconnect is exposed"), ViewModel->IsDisconnected());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRA4LobbyScreenCompositionTest,
    "RA4.UI.Screens.Lobby.UsesVirtualizedPlayerList",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRA4LobbyScreenCompositionTest::RunTest(const FString& Parameters)
{
    URA4LobbyScreenWidget* Lobby = NewObject<URA4LobbyScreenWidget>();
    TestTrue(TEXT("Lobby initializes"), Lobby->Initialize());
    Lobby->TakeWidget();

    TestNotNull(TEXT("Player list"), Lobby->GetPlayerList());
    TestEqual(TEXT("Player list item count"), Lobby->GetPlayerList()->GetNumItems(), 8);
    TestNotNull(TEXT("Start button"), Lobby->GetStartButton());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRA4HUDViewModelProjectionTest,
    "RA4.UI.HUD.ViewModel.AppliesSnapshotsOnlyWhenSectionsChange",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRA4HUDViewModelProjectionTest::RunTest(const FString& Parameters)
{
    URA4HUDViewModel* ViewModel = NewObject<URA4HUDViewModel>();
    FRA4HUDSnapshotView Snapshot;
    Snapshot.Credits = 23450;
    Snapshot.CreditsDelta = 850;
    Snapshot.PowerProduced = 17820;
    Snapshot.PowerConsumed = 9680;
    Snapshot.MatchElapsedSeconds = 754;
    Snapshot.SelectionKind = ERA4SelectionKind::SingleUnit;
    Snapshot.SelectionCount = 1;
    Snapshot.PrimaryEntityName = TEXT("Рудный комбайн «Богатырь»");
    Snapshot.SelectionHealthRatio = 0.72f;
    Snapshot.HarvesterCargo = 820;
    Snapshot.HarvesterCapacity = 1400;

    FRA4ProductionEntry QueueEntry;
    QueueEntry.ContentId = 44;
    QueueEntry.DisplayName = FText::FromString(TEXT("Танк Т-34"));
    QueueEntry.ProgressPercent = 42;
    Snapshot.ProductionQueue.Add(QueueEntry);

    FRA4BuildOption BlockedOption;
    BlockedOption.ContentId = 91;
    BlockedOption.DisplayName = FText::FromString(TEXT("Ракетная шахта"));
    BlockedOption.BlockReason = ERA4BuildBlockReason::MissingPrerequisite;
    Snapshot.BuildOptions.Add(BlockedOption);

    FRA4HUDObjective Objective;
    Objective.Label = FText::FromString(TEXT("Уничтожить базу противника"));
    Snapshot.Objectives.Add(Objective);

    FRA4Alert Alert;
    Alert.Message = FText::FromString(TEXT("Наша база атакована"));
    Alert.Severity = ERA4AlertSeverity::Critical;
    Snapshot.Alerts.Add(Alert);

    const ERA4HUDChangeFlags FirstChanges = ViewModel->ApplySnapshot(Snapshot);
    TestTrue(TEXT("First snapshot updates resources"), EnumHasAnyFlags(FirstChanges, ERA4HUDChangeFlags::Resources));
    TestTrue(TEXT("First snapshot updates selection"), EnumHasAnyFlags(FirstChanges, ERA4HUDChangeFlags::Selection));
    TestTrue(TEXT("First snapshot updates production"), EnumHasAnyFlags(FirstChanges, ERA4HUDChangeFlags::Production));
    TestTrue(TEXT("First snapshot updates objectives"), EnumHasAnyFlags(FirstChanges, ERA4HUDChangeFlags::Objectives));
    TestTrue(TEXT("First snapshot updates alerts"), EnumHasAnyFlags(FirstChanges, ERA4HUDChangeFlags::Alerts));
    TestEqual(TEXT("Cargo projected"), ViewModel->GetHarvesterCargo(), 820);
    TestEqual(TEXT("Blocked reason projected"), ViewModel->GetBuildOptions()[0].BlockReason, ERA4BuildBlockReason::MissingPrerequisite);

    TestEqual(TEXT("Identical snapshot is silent"), ViewModel->ApplySnapshot(Snapshot), ERA4HUDChangeFlags::None);
    Snapshot.Credits += 100;
    TestEqual(TEXT("Credit-only change stays local"), ViewModel->ApplySnapshot(Snapshot), ERA4HUDChangeFlags::Resources);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRA4HUDShellInputRegionsTest,
    "RA4.UI.HUD.ViewModel.InteractivePanelsBlockWorldInput",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRA4HUDShellInputRegionsTest::RunTest(const FString& Parameters)
{
    URA4HUDWidget* HUD = NewObject<URA4HUDWidget>();
    TestTrue(TEXT("HUD initializes"), HUD->Initialize());
    HUD->TakeWidget();

    TestTrue(TEXT("Objectives panel blocks input"), HUD->IsWorldInputBlockedAtReferencePoint(FVector2D(120.0f, 150.0f)));
    TestTrue(TEXT("Minimap blocks input"), HUD->IsWorldInputBlockedAtReferencePoint(FVector2D(1720.0f, 220.0f)));
    TestTrue(TEXT("Production blocks input"), HUD->IsWorldInputBlockedAtReferencePoint(FVector2D(1760.0f, 650.0f)));
    TestTrue(TEXT("Selection panel blocks input"), HUD->IsWorldInputBlockedAtReferencePoint(FVector2D(180.0f, 950.0f)));
    TestTrue(TEXT("Command grid blocks input"), HUD->IsWorldInputBlockedAtReferencePoint(FVector2D(1510.0f, 960.0f)));
    TestFalse(TEXT("World viewport passes input"), HUD->IsWorldInputBlockedAtReferencePoint(FVector2D(900.0f, 420.0f)));
    TestEqual(TEXT("All HUD regions exist"), HUD->GetInteractiveRegionCount(), 6);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRA4MinimapGeometryTest,
    "RA4.UI.HUD.SlateLayers.MinimapGeometryAndClicks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRA4MinimapGeometryTest::RunTest(const FString& Parameters)
{
    const FVector2D MapPoint = SRA4Minimap::WorldToMap(
        FVector2D(50.0f, 25.0f), FVector2D(100.0f, 50.0f), FVector2D(300.0f, 150.0f));
    TestTrue(TEXT("World center maps to center"), MapPoint.Equals(FVector2D(150.0f, 75.0f)));
    const FVector2D WorldPoint = SRA4Minimap::MapToWorld(
        FVector2D(150.0f, 75.0f), FVector2D(300.0f, 150.0f), FVector2D(100.0f, 50.0f));
    TestTrue(TEXT("Map click converts back to world"), WorldPoint.Equals(FVector2D(50.0f, 25.0f)));
    TestTrue(TEXT("Coordinates clamp to map"), SRA4Minimap::WorldToMap(
        FVector2D(-40.0f, 90.0f), FVector2D(100.0f, 50.0f), FVector2D(300.0f, 150.0f))
        .Equals(FVector2D(0.0f, 150.0f)));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRA4MinimapBatchedMarkersTest,
    "RA4.UI.HUD.SlateLayers.UsesOneSlateLayerForLargeSnapshots",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRA4MinimapBatchedMarkersTest::RunTest(const FString& Parameters)
{
    TArray<FRA4RadarMarker> Markers;
    Markers.SetNum(1000);
    for (int32 Index = 0; Index < Markers.Num(); ++Index)
    {
        Markers[Index].WorldPosition = FVector2D(Index % 100, Index / 100);
        Markers[Index].Owner = Index % 4;
        Markers[Index].Kind = Index % 7 == 0
            ? ERA4RadarMarkerKind::Building
            : ERA4RadarMarkerKind::Unit;
    }

    URA4MinimapWidget* Minimap = NewObject<URA4MinimapWidget>();
    Minimap->SetSnapshot(Markers, FVector2D(100.0f, 100.0f), 0);
    TestTrue(TEXT("Minimap initializes"), Minimap->Initialize());
    Minimap->TakeWidget();

    TestEqual(TEXT("All markers stay in the Slate snapshot"), Minimap->GetMarkerCount(), 1000);
    TestEqual(TEXT("No marker child widgets are created"), Minimap->GetMarkerWidgetCount(), 0);
    TestTrue(TEXT("Click conversion uses current map size"), Minimap->ConvertMapClickToWorld(
        FVector2D(128.0f, 128.0f), FVector2D(256.0f, 256.0f)).Equals(FVector2D(50.0f, 50.0f)));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRA4WorldMarkerLayerStateTest,
    "RA4.UI.HUD.SlateLayers.WorldMarkersResolveTeamAndIntelState",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRA4WorldMarkerLayerStateTest::RunTest(const FString& Parameters)
{
    FRA4WorldMarkerView Friendly;
    Friendly.Team = ERA4MarkerTeam::Friendly;
    Friendly.Intel = ERA4MarkerIntel::Visible;
    Friendly.HealthRatio = 0.8f;
    Friendly.bSelected = true;

    FRA4WorldMarkerView HiddenEnemy;
    HiddenEnemy.Team = ERA4MarkerTeam::Enemy;
    HiddenEnemy.Intel = ERA4MarkerIntel::Hidden;

    TestEqual(TEXT("Friendly marker uses friendly glyph"),
        SRA4WorldMarkerLayer::ResolveGlyph(Friendly), ERA4MarkerGlyph::FriendlySelected);
    TestEqual(TEXT("Hidden enemy is not painted"),
        SRA4WorldMarkerLayer::ResolveGlyph(HiddenEnemy), ERA4MarkerGlyph::Hidden);
    TestEqual(TEXT("Healthy marker is green"),
        SRA4WorldMarkerLayer::ResolveHealthBand(Friendly.HealthRatio), ERA4HealthBand::Healthy);
    return true;
}

#endif
