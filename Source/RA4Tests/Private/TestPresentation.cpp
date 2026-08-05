// Copyright (c) Red Alert 4 project. Tests for the HUD data projection.
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Core/SimConfig.h"
#include "RA4Presentation/HudSnapshot.h"
#include "RA4Presentation/RA4ArtMapping.h"

using namespace RA4;
using namespace RA4::Presentation;
using namespace RA4Test;

namespace
{

struct HudFixture
{
    ContentDatabase Content;
    SimWorld World;
    HudSnapshotBuilder Builder;
    HudSnapshot Snapshot;

    HudFixture()
    {
        BuildDefaultContent(Content);
        World.Initialize(&Content, MakeTestSetup(31415));
        Builder.Initialize(0);
    }

    // Steps the world and refreshes the snapshot, mirroring what the UI subsystem
    // does once per simulation tick.
    void Step(int32_t Ticks = 1, const std::vector<EntityId>& Selection = {})
    {
        for (int32_t I = 0; I < Ticks; ++I)
        {
            World.Tick(nullptr);
            Builder.Build(World, Selection, Snapshot);
            World.ClearEvents();
        }
    }

    void Refresh(const std::vector<EntityId>& Selection = {})
    {
        Builder.Build(World, Selection, Snapshot);
    }

    const BuildOption* FindOption(ContentId Id) const
    {
        for (const BuildOption& O : Snapshot.Production.Options)
        {
            if (O.Content == Id)
            {
                return &O;
            }
        }
        return nullptr;
    }

    int32_t CountAlerts(AlertType Type) const
    {
        int32_t Count = 0;
        for (const Alert& A : Snapshot.Alerts)
        {
            if (A.Type == Type)
            {
                ++Count;
            }
        }
        return Count;
    }
};

} // namespace

// ---------------------------------------------------------------------------
// Resources
// ---------------------------------------------------------------------------

RA4_TEST(Hud, ResourceBarReportsRealCreditsAndPower)
{
    HudFixture F;
    SpawnEnemyOutpost(F.World);
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    F.World.SpawnBuilding(Ids::SovPower, 0, TileCoord(14, 10), true);
    F.Step(2);

    RA4_EXPECT_EQ(F.Snapshot.Resources.Credits, 10000);
    RA4_EXPECT_EQ(F.Snapshot.Resources.PowerProduced, 150);
    RA4_EXPECT_EQ(F.Snapshot.Resources.PowerRatioPercent, 100);
    RA4_EXPECT(!F.Snapshot.Resources.bPowerShortage);
}

RA4_TEST(Hud, FirstSnapshotDoesNotReportStartingCreditsAsIncome)
{
    HudFixture F;
    SpawnEnemyOutpost(F.World);
    F.Refresh();
    // Without the guard the HUD would flash "+10000" on the opening frame.
    RA4_EXPECT_EQ(F.Snapshot.Resources.CreditsDelta, 0);
    RA4_EXPECT_EQ(F.Snapshot.Resources.Credits, 10000);
}

RA4_TEST(Hud, CreditDeltaTracksSpending)
{
    HudFixture F;
    SpawnEnemyOutpost(F.World);
    const EntityId Yard = F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    F.Step(1);

    Command Start = MakeCommand(CommandType::StartProduction, 0);
    Start.Primary = Yard;
    Start.Content = Ids::SovPower;   // costs 800
    RA4_REQUIRE(F.World.ApplyCommand(Start).IsAccepted());

    // ADR-0012: the order itself costs nothing; the HUD must show money leaving as
    // the item builds, not a single lump the instant the button is clicked.
    F.Refresh();
    RA4_EXPECT_EQ(F.Snapshot.Resources.Credits, 10000);

    F.Step(SecondsToTicks(9), {Yard});
    RA4_EXPECT_EQ(F.Snapshot.Resources.Credits, 9200);
    // The queue card must be able to show the funding progress, not just the build
    // progress, or the player cannot see where the money went.
    RA4_REQUIRE(F.Snapshot.Production.Queue.size() == 1u);
    RA4_EXPECT_EQ(F.Snapshot.Production.Queue[0].TotalCost, 800);
    RA4_EXPECT_EQ(F.Snapshot.Production.Queue[0].PaidCredits, 800);
    RA4_EXPECT(!F.Snapshot.Production.Queue[0].bStarvedForCredits);
}

RA4_TEST(Hud, PowerShortageIsReportedNotInvented)
{
    HudFixture F;
    SpawnEnemyOutpost(F.World);
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    // A war factory draws 50 with no reactor to feed it.
    F.World.SpawnBuilding(Ids::SovWarFactory, 0, TileCoord(14, 14), true);
    F.Step(2);

    RA4_EXPECT(F.Snapshot.Resources.bPowerShortage);
    RA4_EXPECT(F.Snapshot.Resources.PowerRatioPercent < 100);
    RA4_EXPECT_EQ(F.Snapshot.Resources.PowerProduced, 0);
    RA4_EXPECT(F.Snapshot.Resources.PowerConsumed > 0);
}

RA4_TEST(Hud, SupplyIsMarkedUnmodelledRatherThanFabricated)
{
    HudFixture F;
    SpawnEnemyOutpost(F.World);
    F.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(2000, 2000));
    F.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(2100, 2000));
    F.Step(1);

    // The reference HUD shows "88 / 200". The simulation has no population cap, so
    // the widget must hide the counter instead of showing a made-up limit.
    RA4_EXPECT_EQ(F.Snapshot.Resources.SupplyUsed, 2);
    RA4_EXPECT_EQ(F.Snapshot.Resources.SupplyCap, 0);
    RA4_EXPECT(!F.Snapshot.Resources.bSupplyModelled);
}

// ---------------------------------------------------------------------------
// Selection
// ---------------------------------------------------------------------------

RA4_TEST(Hud, EmptySelectionIsAnExplicitState)
{
    HudFixture F;
    SpawnEnemyOutpost(F.World);
    F.Step(1);

    RA4_EXPECT(F.Snapshot.Selection.Kind == SelectionKind::Empty);
    RA4_EXPECT_EQ(F.Snapshot.Selection.TotalCount, 0);
    RA4_EXPECT(F.Snapshot.Selection.Groups.empty());
    RA4_EXPECT(!F.Snapshot.Selection.Primary.IsValid());
}

RA4_TEST(Hud, SingleUnitFillsThePortraitFromRealData)
{
    HudFixture F;
    SpawnEnemyOutpost(F.World);
    const EntityId Tank = F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2000, 2000));
    RA4_REQUIRE(Tank.IsValid());
    F.Step(1, {Tank});

    const SelectionState& S = F.Snapshot.Selection;
    RA4_EXPECT(S.Kind == SelectionKind::SingleUnit);
    RA4_EXPECT_EQ(S.TotalCount, 1);
    RA4_EXPECT(S.Primary == Tank);
    RA4_EXPECT(S.bPrimaryIsOwned);
    RA4_EXPECT_EQ(S.PrimaryHealthCurrent, 520);
    RA4_EXPECT_EQ(S.PrimaryHealthMax, 520);
    // Player-facing text must arrive as a localization key, never as a literal.
    RA4_EXPECT(S.PrimaryDisplayNameKey == "faction.soviet.unit.main_tank");
}

RA4_TEST(Hud, IdenticalUnitsAreGroupedWithCounts)
{
    HudFixture F;
    SpawnEnemyOutpost(F.World);
    std::vector<EntityId> Selection;
    for (int32_t I = 0; I < 5; ++I)
    {
        Selection.push_back(F.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(2000 + I * 60, 2000)));
    }
    Selection.push_back(F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2400, 2000)));
    F.Step(1, Selection);

    const SelectionState& S = F.Snapshot.Selection;
    RA4_EXPECT(S.Kind == SelectionKind::MultipleUnits);
    RA4_EXPECT_EQ(S.TotalCount, 6);
    RA4_EXPECT_EQ(int32_t(S.Groups.size()), 2);

    // The portrait follows the largest group, not the click order.
    RA4_EXPECT(S.PrimaryContent == Ids::SovConscript);

    for (const SelectionGroup& G : S.Groups)
    {
        if (G.Content == Ids::SovConscript)
        {
            RA4_EXPECT_EQ(G.Count, 5);
            RA4_EXPECT_EQ(G.HealthMax, 500);   // 5 x 100
        }
    }
}

RA4_TEST(Hud, MixedSelectionIsDistinguishedFromUnitsOnly)
{
    HudFixture F;
    SpawnEnemyOutpost(F.World);
    const EntityId Yard = F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    const EntityId Tank = F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2000, 2000));
    F.Step(1, {Yard, Tank});

    RA4_EXPECT(F.Snapshot.Selection.Kind == SelectionKind::Mixed);
    // A producer anywhere in the selection drives the production panel.
    RA4_EXPECT(F.Snapshot.Selection.ProductionSource == Yard);
}

RA4_TEST(Hud, SelectingAProducerSwitchesTheProductionPanelToIt)
{
    HudFixture F;
    SpawnEnemyOutpost(F.World);
    const EntityId Yard = F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    F.World.SpawnBuilding(Ids::SovPower, 0, TileCoord(14, 10), true);
    const EntityId Barracks = F.World.SpawnBuilding(Ids::SovBarracks, 0, TileCoord(10, 14), true);
    F.Step(1, {Barracks});

    RA4_EXPECT(F.Snapshot.Production.Producer == Barracks);
    F.Step(1, {Yard});
    RA4_EXPECT(F.Snapshot.Production.Producer == Yard);
}

// ---------------------------------------------------------------------------
// Production
// ---------------------------------------------------------------------------

RA4_TEST(Hud, BuildCardsReportWhyTheyAreBlocked)
{
    HudFixture F;
    SpawnEnemyOutpost(F.World);
    // A lone soldier keeps the player in the match without granting any tech, so the
    // blocked reasons below are about prerequisites rather than about the match
    // having already ended.
    F.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(4000, 4000));
    F.Step(1);

    // No construction yard yet: nothing has a producer.
    {
        const BuildOption* Power = F.FindOption(Ids::SovPower);
        RA4_REQUIRE(Power != nullptr);
        RA4_EXPECT(!Power->bAvailable);
        RA4_EXPECT(Power->BlockReason == BuildBlockReason::MissingPrerequisite);
    }

    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    F.Step(1);
    {
        const BuildOption* Power = F.FindOption(Ids::SovPower);
        RA4_REQUIRE(Power != nullptr);
        RA4_EXPECT(Power->bAvailable);
        RA4_EXPECT(Power->BlockReason == BuildBlockReason::None);
        // The card knows which building a click would go to.
        RA4_EXPECT(Power->Producer.IsValid());

        // Heavy tanks still need a war factory.
        const BuildOption* Tank = F.FindOption(Ids::SovHeavyTank);
        RA4_REQUIRE(Tank != nullptr);
        RA4_EXPECT(Tank->BlockReason == BuildBlockReason::MissingPrerequisite);
    }
}

// Under ADR-0012 being poor no longer blocks an order: the simulation accepts it and
// funds it as income arrives. A HUD that greyed the card out would forbid a command
// the simulation would take, so affordability must not be a block reason -- while a
// genuinely unbuildable item still is.
RA4_TEST(Hud, PovertyDoesNotBlockTheCardButMissingTechStillDoes)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);
    MatchSetup Setup = MakeTestSetup(7);
    Setup.Players[0].StartingCredits = 100;

    SimWorld World;
    World.Initialize(&Content, Setup);
    World.SpawnBuilding(Ids::AllConYard, 1, TileCoord(58, 58), true);
    World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);

    HudSnapshotBuilder Builder;
    Builder.Initialize(0);
    HudSnapshot Snapshot;
    World.Tick(nullptr);
    Builder.Build(World, {}, Snapshot);

    const BuildOption* Power = nullptr;
    const BuildOption* Tank = nullptr;
    for (const BuildOption& O : Snapshot.Production.Options)
    {
        if (O.Content == Ids::SovPower) { Power = &O; }
        if (O.Content == Ids::SovHeavyTank) { Tank = &O; }
    }

    // 800-cost reactor with 100 credits: tech and producer are fine, so the player
    // is allowed to queue it and watch it fund slowly.
    RA4_REQUIRE(Power != nullptr);
    RA4_EXPECT(Power->bAvailable);
    RA4_EXPECT(Power->BlockReason == BuildBlockReason::None);
    // The simulation must agree, or the HUD is lying about what is possible.
    Command Start = MakeCommand(CommandType::StartProduction, 0);
    Start.Content = Ids::SovPower;
    RA4_EXPECT(World.ApplyCommand(Start).IsAccepted());

    // A heavy tank needs a war factory that does not exist, and that still blocks.
    RA4_REQUIRE(Tank != nullptr);
    RA4_EXPECT(!Tank->bAvailable);
    RA4_EXPECT(Tank->BlockReason != BuildBlockReason::None);
    RA4_EXPECT(Tank->BlockReason != BuildBlockReason::InsufficientCredits);
}

RA4_TEST(Hud, QueueReportsProgressAndRemainingTime)
{
    HudFixture F;
    SpawnEnemyOutpost(F.World);
    const EntityId Yard = F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    F.Step(1, {Yard});

    Command Start = MakeCommand(CommandType::StartProduction, 0);
    Start.Primary = Yard;
    Start.Content = Ids::SovPower;   // 8 s == 160 ticks
    RA4_REQUIRE(F.World.ApplyCommand(Start).IsAccepted());

    F.Step(1, {Yard});
    RA4_REQUIRE(F.Snapshot.Production.Queue.size() == 1u);
    RA4_EXPECT(F.Snapshot.Production.Queue[0].ProgressPercent < 5);
    RA4_EXPECT(F.Snapshot.Production.Queue[0].RemainingTicks > 150);
    RA4_EXPECT(!F.Snapshot.Production.Queue[0].bAwaitingPlacement);

    F.Step(90, {Yard});
    RA4_REQUIRE(F.Snapshot.Production.Queue.size() == 1u);
    const int32_t Mid = F.Snapshot.Production.Queue[0].ProgressPercent;
    RA4_EXPECT(Mid > 40 && Mid < 70);
}

RA4_TEST(Hud, FinishedStructureIsFlaggedAsAwaitingPlacement)
{
    HudFixture F;
    SpawnEnemyOutpost(F.World);
    const EntityId Yard = F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    F.Step(1, {Yard});

    Command Start = MakeCommand(CommandType::StartProduction, 0);
    Start.Primary = Yard;
    Start.Content = Ids::SovPower;
    RA4_REQUIRE(F.World.ApplyCommand(Start).IsAccepted());

    F.Step(SecondsToTicks(10), {Yard});
    RA4_REQUIRE(F.Snapshot.Production.Queue.size() == 1u);
    // The card must say "place me", not "still building".
    RA4_EXPECT(F.Snapshot.Production.Queue[0].bAwaitingPlacement);
    RA4_EXPECT_EQ(F.Snapshot.Production.Queue[0].ProgressPercent, 100);
    RA4_EXPECT_EQ(F.Snapshot.Production.Queue[0].RemainingTicks, 0);
}

RA4_TEST(Hud, OptionsAreGroupedByCategoryForTheTabs)
{
    HudFixture F;
    SpawnEnemyOutpost(F.World);
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    F.Step(1);

    RA4_REQUIRE(!F.Snapshot.Production.Options.empty());
    // Category order must be non-decreasing so the panel can slice by tab without
    // re-sorting on every frame.
    for (size_t I = 1; I < F.Snapshot.Production.Options.size(); ++I)
    {
        const uint8_t Previous = uint8_t(F.Snapshot.Production.Options[I - 1].Category);
        const uint8_t Current = uint8_t(F.Snapshot.Production.Options[I].Category);
        RA4_EXPECT(Previous <= Current);
    }

    // Only the local player's faction appears; Alliance buildings must not leak into
    // a Soviet sidebar.
    for (const BuildOption& O : F.Snapshot.Production.Options)
    {
        RA4_EXPECT(O.Content != Ids::AllConYard);
    }
}

RA4_TEST(Hud, RadarShowsOwnForcesButNotEnemiesHiddenByFog)
{
    HudFixture F;
    const EntityId OwnUnit =
        F.World.SpawnUnit(Ids::SovConscript, 0, Vec2::FromInts(1200, 1200));
    const EntityId HiddenEnemy =
        F.World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(11000, 11000));
    F.Step(1, {OwnUnit});

    RA4_EXPECT_EQ(F.Snapshot.Radar.MapWidthUnits,
                  F.World.GetMap().Width * kTileSizeUnits);
    RA4_EXPECT_EQ(F.Snapshot.Radar.MapHeightUnits,
                  F.World.GetMap().Height * kTileSizeUnits);

    bool bFoundOwn = false;
    bool bFoundHiddenEnemy = false;
    for (const RadarMarker& Marker : F.Snapshot.Radar.Markers)
    {
        if (Marker.Entity == OwnUnit)
        {
            bFoundOwn = true;
            RA4_EXPECT(Marker.bSelected);
            RA4_EXPECT_EQ(Marker.Owner, 0);
        }
        if (Marker.Entity == HiddenEnemy)
        {
            bFoundHiddenEnemy = true;
        }
    }

    RA4_EXPECT(bFoundOwn);
    RA4_EXPECT(!bFoundHiddenEnemy);
}

// ---------------------------------------------------------------------------
// Alerts
// ---------------------------------------------------------------------------

RA4_TEST(Hud, RepeatedDamageMergesIntoOneAlert)
{
    HudFixture F;
    SpawnEnemyOutpost(F.World);
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    // Enemy armour parked next to the base produces a damage event every few ticks.
    for (int32_t I = 0; I < 3; ++I)
    {
        F.World.SpawnUnit(Ids::AllLightTank, 1, Vec2::FromInts(2400 + I * 150, 2600));
    }

    F.Step(SecondsToTicks(6));

    // One barrage must be one line in the feed, not sixty.
    RA4_EXPECT_EQ(F.CountAlerts(AlertType::BaseUnderAttack), 1);
    bool bFound = false;
    for (const Alert& A : F.Snapshot.Alerts)
    {
        if (A.Type == AlertType::BaseUnderAttack)
        {
            bFound = true;
            RA4_EXPECT(A.RepeatCount > 1);
            RA4_EXPECT(A.Severity == AlertSeverity::Critical);
            RA4_EXPECT(A.bHasLocation);
        }
    }
    RA4_EXPECT(bFound);
}

RA4_TEST(Hud, CriticalAlertsOutrankInformationalOnes)
{
    HudFixture F;
    SpawnEnemyOutpost(F.World);
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    F.World.SpawnBuilding(Ids::SovWarFactory, 0, TileCoord(14, 14), true);   // draws power, none supplied
    F.World.SpawnUnit(Ids::AllLightTank, 1, Vec2::FromInts(2400, 2600));
    F.Step(SecondsToTicks(5));

    RA4_REQUIRE(!F.Snapshot.Alerts.empty());
    // An EVA "unit ready" must never push a "base under attack" off the top.
    for (size_t I = 1; I < F.Snapshot.Alerts.size(); ++I)
    {
        RA4_EXPECT(uint8_t(F.Snapshot.Alerts[I - 1].Severity) >= uint8_t(F.Snapshot.Alerts[I].Severity));
    }
    RA4_EXPECT(F.Snapshot.Alerts[0].Severity == AlertSeverity::Critical);
}

RA4_TEST(Hud, AlertsExpireOnceTheConditionStops)
{
    HudFixture F;
    SpawnEnemyOutpost(F.World);
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    F.World.SpawnBuilding(Ids::SovWarFactory, 0, TileCoord(14, 14), true);
    F.Builder.SetAlertLifetimeTicks(40);
    F.Step(SecondsToTicks(3));
    RA4_EXPECT(F.CountAlerts(AlertType::LowPower) > 0);

    // Supply the power and the warning must clear itself.
    F.World.SpawnBuilding(Ids::SovPower, 0, TileCoord(18, 10), true);
    F.Step(SecondsToTicks(5));
    RA4_EXPECT(!F.Snapshot.Resources.bPowerShortage);
    RA4_EXPECT_EQ(F.CountAlerts(AlertType::LowPower), 0);
}

RA4_TEST(Hud, EnemyLossesAreNotLocalAlerts)
{
    HudFixture F;
    SpawnEnemyOutpost(F.World);
    F.World.SpawnUnit(Ids::SovHeavyTank, 0, Vec2::FromInts(2000, 2000));
    // 7 m away: inside the tank's 9 m gun, outside the rifleman's 6 m reach, so the
    // exchange is one-sided and nothing of the player's is ever hit.
    const EntityId Victim = F.World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(2700, 2000));
    RA4_REQUIRE(Victim.IsValid());

    for (int32_t I = 0; I < SecondsToTicks(30) && F.World.IsAlive(Victim); ++I)
    {
        F.Step(1);
    }
    RA4_EXPECT(!F.World.IsAlive(Victim));
    // Killing an enemy must not raise "unit lost" in the player's own feed.
    RA4_EXPECT_EQ(F.CountAlerts(AlertType::UnitLost), 0);
    RA4_EXPECT_EQ(F.CountAlerts(AlertType::UnitsUnderAttack), 0);
}

// ---------------------------------------------------------------------------
// Match state
// ---------------------------------------------------------------------------

RA4_TEST(Hud, MatchStateDrivesVictoryAndDefeatScreens)
{
    HudFixture F;
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    F.World.SpawnBuilding(Ids::AllConYard, 1, TileCoord(40, 40), true);
    F.Step(2);

    RA4_EXPECT(F.Snapshot.Match.Phase == MatchPhase::Running);
    RA4_EXPECT(!F.Snapshot.Match.bLocalPlayerDefeated);
    RA4_EXPECT_EQ(F.Snapshot.Match.ElapsedSeconds, 0);

    Command Surrender = MakeCommand(CommandType::Surrender, 1);
    RA4_REQUIRE(F.World.ApplyCommand(Surrender).IsAccepted());
    F.Step(2);

    RA4_EXPECT(F.Snapshot.Match.Phase == MatchPhase::Finished);
    RA4_EXPECT_EQ(int32_t(F.Snapshot.Match.Winner), 0);
    RA4_EXPECT(!F.Snapshot.Match.bLocalPlayerDefeated);

    // Everything is blocked once the match is over.
    RA4_REQUIRE(!F.Snapshot.Production.Options.empty());
    RA4_EXPECT(F.Snapshot.Production.Options[0].BlockReason == BuildBlockReason::MatchOver);
}

RA4_TEST(Hud, ElapsedTimeTracksTheSimulationClock)
{
    HudFixture F;
    SpawnEnemyOutpost(F.World);
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    F.Step(SecondsToTicks(7));

    RA4_EXPECT_EQ(F.Snapshot.Match.ElapsedSeconds, 7);
    RA4_EXPECT_EQ(int32_t(F.Snapshot.Match.Tick), SecondsToTicks(7));
}

RA4_TEST(ArtMapping, UnitArtDefinitionSupportsAnimationProperties)
{
    FRA4UnitArtDefinition Def;
    Def.UnitId = FName("SU_Conscript");
    Def.MeshScale = FVector(1.0f, 1.0f, 1.0f);
    Def.MeshOffset = FVector(0.0f, 0.0f, -90.0f);
    Def.MeshRotation = FRotator(0.0f, -90.0f, 0.0f);

    RA4_EXPECT(Def.UnitId == FName("SU_Conscript"));
    RA4_EXPECT_EQ(Def.MeshScale.X, 1.0f);
    RA4_EXPECT_EQ(Def.MeshOffset.Z, -90.0f);
    RA4_EXPECT_EQ(Def.MeshRotation.Yaw, -90.0f);
    RA4_EXPECT(Def.IdleAnim.IsNull());
    RA4_EXPECT(Def.RunAnim.IsNull());
}

