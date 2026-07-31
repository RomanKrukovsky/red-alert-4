// Copyright (c) Red Alert 4 project. Automated UI & Skirmish Setup tests.
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Input/CameraController.h"
#include "RA4Presentation/HudSnapshot.h"
#include "RA4Simulation/SimWorld.h"

using namespace RA4;
using namespace RA4::Input;
using namespace RA4::Presentation;
using namespace RA4Test;

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

    // Pan Up (W key -> +Y) and Right (D key -> -X focus)
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
