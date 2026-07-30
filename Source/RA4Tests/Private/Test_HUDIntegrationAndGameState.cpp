// Copyright (c) Red Alert 4 project. Unit tests for HUD Snapshot & ViewModels.
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Presentation/HudSnapshot.h"
#include "RA4Simulation/SimWorld.h"
#include "RA4Content/ContentDatabase.h"

RA4_TEST(HudSnapshot, SnapshotBuilderGeneratesResourcesAndSelection)
{
    RA4::ContentDatabase Content;
    RA4::BuildDefaultContent(Content);
    RA4::SimWorld World;
    RA4::MatchSetup Setup = RA4Test::MakeTestSetup(12345);

    World.Initialize(&Content, Setup);

    RA4::Presentation::HudSnapshotBuilder Builder;
    Builder.Initialize(0);

    RA4::Presentation::HudSnapshot Snapshot;
    std::vector<RA4::EntityId> Selection;

    Builder.Build(World, Selection, Snapshot);

    RA4_EXPECT_EQ(Snapshot.LocalPlayer, 0);
    RA4_EXPECT_EQ(Snapshot.Resources.Credits, 10000);
    RA4_EXPECT_EQ(Snapshot.Resources.PowerRatioPercent, 100);
    RA4_EXPECT(Snapshot.Selection.Kind == RA4::Presentation::SelectionKind::Empty);
}

RA4_TEST(HudSnapshot, PrimarySelectionPopulatesHealthAndName)
{
    RA4::ContentDatabase Content;
    RA4::BuildDefaultContent(Content);
    RA4::SimWorld World;
    RA4::MatchSetup Setup = RA4Test::MakeTestSetup(12345);

    World.Initialize(&Content, Setup);
    RA4Test::SpawnEnemyOutpost(World, 1);

    RA4::EntityId Tank = World.SpawnUnit(RA4Test::Ids::SovHeavyTank, 0, RA4::Vec2(RA4::Fixed::FromInt(500), RA4::Fixed::FromInt(500)));
    RA4_EXPECT(Tank.IsValid());

    RA4::Presentation::HudSnapshotBuilder Builder;
    Builder.Initialize(0);

    RA4::Presentation::HudSnapshot Snapshot;
    std::vector<RA4::EntityId> Selection = { Tank };

    Builder.Build(World, Selection, Snapshot);

    RA4_EXPECT(Snapshot.Selection.Kind == RA4::Presentation::SelectionKind::SingleUnit);
    RA4_EXPECT_EQ(Snapshot.Selection.TotalCount, 1);
    RA4_EXPECT(Snapshot.Selection.PrimaryHealthMax > 0);
    RA4_EXPECT(Snapshot.Selection.bPrimaryIsOwned);
}
