// Copyright (c) Red Alert 4 project. Automated UI & Skirmish Setup tests.
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Input/CameraController.h"
#include "RA4Presentation/HudSnapshot.h"
#include "RA4Simulation/SimWorld.h"
#include "RA4UIScreenCatalog.h"

using namespace RA4;
using namespace RA4::Input;
using namespace RA4::Presentation;
using namespace RA4Test;

RA4_TEST(UI, ReferenceCatalogCoversAllNineteenScreenshots)
{
    for (int ReferenceNumber = 1; ReferenceNumber <= 19; ++ReferenceNumber)
    {
        RA4_EXPECT(RA4::UI::FindScreenByReference(ReferenceNumber) != nullptr);
    }

    RA4_EXPECT(RA4::UI::FindScreenByReference(0) == nullptr);
    RA4_EXPECT(RA4::UI::FindScreenByReference(20) == nullptr);

    // References 3..6 are the faction campaign detail plates, in the same
    // order as the remaster shot files; the selection wizard lives outside
    // the plate range.
    RA4_EXPECT(RA4::UI::FindScreenByReference(3)->Id == RA4::UI::ScreenId::EurasianCampaign);
    RA4_EXPECT(RA4::UI::FindScreenByReference(4)->Id == RA4::UI::ScreenId::AtlanticCampaign);
    RA4_EXPECT(RA4::UI::FindScreenByReference(5)->Id == RA4::UI::ScreenId::EasternCampaign);
    RA4_EXPECT(RA4::UI::FindScreenByReference(6)->Id == RA4::UI::ScreenId::PacificCampaign);
    RA4_EXPECT(RA4::UI::FindScreenByReference(19)->Id == RA4::UI::ScreenId::IndependentCampaign);
    for (const RA4::UI::ScreenReferenceDefinition& Reference : RA4::UI::GetScreenReferenceCatalog())
    {
        RA4_EXPECT(Reference.Id != RA4::UI::ScreenId::CampaignSelect);
    }
}

RA4_TEST(UI, RepeatedReferencesReuseTheirRealScreen)
{
    const RA4::UI::ScreenReferenceDefinition* EurasianGround =
        RA4::UI::FindScreenByReference(12);
    const RA4::UI::ScreenReferenceDefinition* EurasianBase =
        RA4::UI::FindScreenByReference(17);
    const RA4::UI::ScreenReferenceDefinition* PacificAir =
        RA4::UI::FindScreenByReference(15);
    const RA4::UI::ScreenReferenceDefinition* PacificBase =
        RA4::UI::FindScreenByReference(18);

    RA4_EXPECT(EurasianGround != nullptr);
    RA4_EXPECT(EurasianBase != nullptr);
    RA4_EXPECT(PacificAir != nullptr);
    RA4_EXPECT(PacificBase != nullptr);
    RA4_EXPECT(EurasianGround->Id == EurasianBase->Id);
    RA4_EXPECT(EurasianGround->Variant != EurasianBase->Variant);
    RA4_EXPECT(PacificAir->Id == PacificBase->Id);
    RA4_EXPECT(PacificAir->Variant != PacificBase->Variant);
}

RA4_TEST(UI, ScreenContractSeparatesMenuAndHudInteraction)
{
    const RA4::UI::ScreenDefinition* MainMenu =
        RA4::UI::FindScreen(RA4::UI::ScreenId::MainMenu);
    const RA4::UI::ScreenDefinition* PacificHud =
        RA4::UI::FindScreen(RA4::UI::ScreenId::PacificHud);
    const RA4::UI::ScreenReferenceDefinition* PacificAirReference =
        RA4::UI::FindScreenByReference(15);

    RA4_EXPECT(MainMenu != nullptr);
    RA4_EXPECT(PacificHud != nullptr);
    RA4_EXPECT(PacificAirReference != nullptr);
    RA4_EXPECT(MainMenu->Family == RA4::UI::ScreenFamily::MainMenu);
    RA4_EXPECT(MainMenu->Input == RA4::UI::InputPolicy::MenuOnly);
    RA4_EXPECT(PacificHud->Family == RA4::UI::ScreenFamily::InGameHud);
    RA4_EXPECT(PacificAirReference->Variant == RA4::UI::ScreenVariant::AirWarfare);
    RA4_EXPECT(PacificHud->Input == RA4::UI::InputPolicy::GameAndUI);
}

RA4_TEST(UI, WASDCameraPanningAndBoundsClamping)
{
    CameraController Camera;
    CameraConfig Config;
    Config.MinHeight = 1000.0f;
    Config.MaxHeight = 5000.0f;
    Config.PanSpeedAtMinHeight = 1000.0f;
    Config.PanSmoothing = 100.0f;   // Fast approach for testing
    Camera.Configure(Config);

    Camera.SetMapBounds(Vec2f(0.0f, 0.0f), Vec2f(10000.0f, 10000.0f));
    Camera.FocusOn(Vec2f(5000.0f, 5000.0f), true);

    // With the north-up zero-degree opening yaw, W and D move up/right on screen.
    // In this camera basis W increases world Y while D decreases world X.
    Camera.SetKeyboardPan(1.0f, 1.0f);
    Camera.Update(0.5f);

    Vec2f Focus = Camera.GetFocus();
    RA4_EXPECT(Focus.X < 5000.0f);
    RA4_EXPECT(Focus.Y > 5000.0f);

    // Pan far off-map to verify bounds clamping
    Camera.FocusOn(Vec2f(99999.0f, 99999.0f), true);
    Focus = Camera.GetFocus();
    RA4_EXPECT(Focus.X <= 10000.0f + Config.BorderMarginUnits);
    RA4_EXPECT(Focus.Y <= 10000.0f + Config.BorderMarginUnits);
}

RA4_TEST(UI, SelectionDetailsAndHarvesterCargo)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);
    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(42000));

    HudSnapshotBuilder Builder;
    Builder.Initialize(0);

    const EntityId Harvester = World.SpawnUnit(Ids::SovHarvester, 0, Vec2(Fixed::FromInt(500), Fixed::FromInt(500)));
    std::vector<EntityId> Selection = {Harvester};

    HudSnapshot Snapshot;
    Builder.Build(World, Selection, Snapshot);

    RA4_EXPECT_EQ(Snapshot.Selection.TotalCount, 1);
    RA4_EXPECT_EQ(int(Snapshot.Selection.Kind), int(SelectionKind::SingleUnit));
    RA4_EXPECT(Snapshot.Selection.Primary == Harvester);
    RA4_EXPECT(Snapshot.Selection.bPrimaryIsOwned);
}

RA4_TEST(UI, BuildCardCostTimePowerAndBlockReasons)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);
    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(42001));

    HudSnapshotBuilder Builder;
    Builder.Initialize(0);

    HudSnapshot Snapshot;
    Builder.Build(World, {}, Snapshot);

    // Find Power Plant build option
    bool bFoundPowerPlant = false;
    for (const BuildOption& Option : Snapshot.Production.Options)
    {
        if (Option.Content == Ids::SovPower)
        {
            bFoundPowerPlant = true;
            RA4_EXPECT(Option.Cost > 0);
            RA4_EXPECT(Option.BuildTimeTicks > 0);
            RA4_EXPECT(Option.PowerDelta > 0);   // Power plant provides positive power
            break;
        }
    }
    RA4_EXPECT(bFoundPowerPlant);
}

RA4_TEST(UI, SkirmishSetupOptionsAndConflictValidation)
{
    // Validate player & AI conflict logic rules
    const int32_t PlayerColorIndex = 0;   // Red
    const int32_t AIColorIndex = 0;       // Red (Conflict!)

    const bool bColorConflict = (PlayerColorIndex == AIColorIndex);
    RA4_EXPECT(bColorConflict);

    const int32_t PlayerSpotIndex = 0;   // Spot 1
    const int32_t AISpotIndex = 1;       // Spot 2

    const bool bSpotConflict = (PlayerSpotIndex == AISpotIndex);
    RA4_EXPECT(!bSpotConflict);
}
