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


// ---------------------------------------------------------------------------
// Minimap (radar panel)
// ---------------------------------------------------------------------------

namespace
{
// A base with an optional radar, an enemy parked well outside every building's sight but
// inside radar reach, and a dial for driving the power tier.
struct MinimapFixture
{
    ContentDatabase Content;
    SimWorld World;
    ContentId RadarType;
    EntityId Radar;

    MinimapFixture(bool bWithRadar, int32_t Turrets)
    {
        BuildDefaultContent(Content);
        // No shipped definition sets bIsRadar, so author one -- exactly how a faction would.
        const EntityDef* Base = Content.FindEntity(Ids::SovPower);
        EntityDef RadarDef = *Base;
        RadarDef.Id = MakeContentId("building.sov.minimap_test_radar");
        RadarDef.Name = "building.sov.minimap_test_radar";
        RadarDef.Building.bIsRadar = true;
        RadarDef.Building.bIsPowerPlant = false;
        RadarDef.Building.PowerProduced = 0;
        RadarDef.Building.PowerConsumed = 40;
        Content.AddEntity(RadarDef);
        RadarType = RadarDef.Id;

        World.Initialize(&Content, MakeTestSetup(6060));
        SpawnEnemyOutpost(World);
        World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
        World.SpawnBuilding(Ids::SovPower, 0, TileCoord(14, 10), true);
        if (bWithRadar)
        {
            Radar = World.SpawnBuilding(RadarType, 0, TileCoord(18, 10), true);
        }
        for (int32_t N = 0; N < Turrets; ++N)
        {
            World.SpawnBuilding(Ids::SovTurret, 0, TileCoord(24 + N * 2, 24), true);
        }
        // 16 tiles from the radar: outside any building's vision, inside radar reach.
        World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(34 * 200, 10 * 200));
        RunTicks(World, 10);
    }

    RadarState Snapshot() const
    {
        HudSnapshotBuilder Builder;
        Builder.Initialize(0);
        HudSnapshot Out;
        Builder.Build(World, {}, Out);
        return Out.Radar;
    }

    // Enemy markers only: the question is whether radar reveals contacts.
    int32_t EnemyMarkers() const
    {
        int32_t Count = 0;
        for (const RadarMarker& M : Snapshot().Markers)
        {
            if (M.Owner != 0 && M.Kind != EntityKind::ResourceNode)
            {
                ++Count;
            }
        }
        return Count;
    }
};
} // namespace

// VisibilityState::RadarDetected existed and was tested for by both the minimap and the AI
// view, but nothing ever set it -- so a radar building contributed nothing to the minimap
// and the "radar" half of the panel was decoration.
RA4_TEST(Minimap, RadarRevealsContactsNoEyesCanSee)
{
    // No radar: the enemy is simply not there as far as the panel is concerned.
    {
        MinimapFixture NoRadar(/*bWithRadar*/ false, /*Turrets*/ 0);
        RA4_REQUIRE(NoRadar.World.GetPlayer(0).GetPowerTier() == PowerTier::Normal);
        RA4_EXPECT_EQ(NoRadar.EnemyMarkers(), 0);
    }
    // With a working radar it appears -- and the only thing that changed is the radar.
    {
        MinimapFixture WithRadar(/*bWithRadar*/ true, /*Turrets*/ 0);
        RA4_REQUIRE(WithRadar.World.GetPlayer(0).GetPowerTier() == PowerTier::Normal);
        RA4_EXPECT(WithRadar.EnemyMarkers() >= 1);
    }
}

RA4_TEST(Minimap, RadarContactsVanishWhenADeficitTakesTheRadar)
{
    // Six turrets against one reactor puts the ratio at 53% -- Moderate, where the
    // Auxiliary band a radar defaults to goes offline.
    MinimapFixture Dark(/*bWithRadar*/ true, /*Turrets*/ 6);
    RA4_REQUIRE(Dark.World.GetPlayer(0).GetPowerTier() >= PowerTier::Moderate);
    RA4_EXPECT_EQ(Dark.EnemyMarkers(), 0);

    // And the whole panel reports itself dark, so the widget can say why rather than
    // looking like a map with nothing on it.
    const RadarState State = Dark.Snapshot();
    RA4_EXPECT(!State.bOnline);
    RA4_EXPECT(State.bOfflineForPower);
    RA4_EXPECT(State.Markers.empty());
}

RA4_TEST(Minimap, PanelStaysOnlineForAPlayerWhoNeverBuiltARadar)
{
    // The effect matrix row is about losing a facility to a deficit, not about gating the
    // basic overview behind tech: a player with no radar must keep their minimap even at
    // Critical, or every early-game blackout blinds them completely.
    MinimapFixture NoRadar(/*bWithRadar*/ false, /*Turrets*/ 13);
    RA4_REQUIRE(NoRadar.World.GetPlayer(0).GetPowerTier() >= PowerTier::Severe);

    const RadarState State = NoRadar.Snapshot();
    RA4_EXPECT(State.bOnline);
    RA4_EXPECT(!State.bOfflineForPower);
    // Own base is still shown.
    bool bSawOwn = false;
    for (const RadarMarker& M : State.Markers)
    {
        if (M.Owner == 0)
        {
            bSawOwn = true;
        }
    }
    RA4_EXPECT(bSawOwn);
}

RA4_TEST(Minimap, RadarStillObeysFogAndIsNotAMaphack)
{
    // Radar grants "something is there", not vision. An enemy outside radar reach as well
    // as outside sight must stay hidden, or the fix would have turned the panel into a
    // maphack.
    MinimapFixture F(/*bWithRadar*/ true, /*Turrets*/ 0);
    const int32_t Before = F.EnemyMarkers();

    // Far corner: well beyond the radar's 24-tile sweep from tile (18,10).
    F.World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(60 * 200, 60 * 200));
    RunTicks(F.World, 10);
    RA4_EXPECT_EQ(F.EnemyMarkers(), Before);
}

// The radar panel is square; maps are not. Drawing a 2:1 map to fill a square panel
// stretched it vertically, so a marker halfway up the map appeared nowhere near the terrain
// it stood on and a click came back with the wrong world position. Letterboxing fixes both,
// and the painter and the click handler share this one mapping so they cannot disagree.
RA4_TEST(Minimap, LetterboxPreservesMapAspectAndCentresIt)
{
    double OffX = 0, OffY = 0, W = 0, H = 0;

    // Square map in a square panel: fills it exactly, no bars.
    ComputeMinimapRect(208, 208, 12800, 12800, OffX, OffY, W, H);
    RA4_EXPECT_EQ(int32_t(OffX), 0);
    RA4_EXPECT_EQ(int32_t(OffY), 0);
    RA4_EXPECT_EQ(int32_t(W), 208);
    RA4_EXPECT_EQ(int32_t(H), 208);

    // Wide 2:1 map: full width, half height, centred vertically. Before the fix this was
    // drawn at full height and every marker's Y was doubled.
    ComputeMinimapRect(208, 208, 25600, 12800, OffX, OffY, W, H);
    RA4_EXPECT_EQ(int32_t(W), 208);
    RA4_EXPECT_EQ(int32_t(H), 104);
    RA4_EXPECT_EQ(int32_t(OffX), 0);
    RA4_EXPECT_EQ(int32_t(OffY), 52);

    // Tall 1:2 map: the mirror image.
    ComputeMinimapRect(208, 208, 12800, 25600, OffX, OffY, W, H);
    RA4_EXPECT_EQ(int32_t(W), 104);
    RA4_EXPECT_EQ(int32_t(H), 208);
    RA4_EXPECT_EQ(int32_t(OffX), 52);
    RA4_EXPECT_EQ(int32_t(OffY), 0);

    // The map rect never exceeds the panel, whatever the shape -- a marker cannot be drawn
    // outside the widget.
    const double Shapes[][2] = {{1, 1}, {3, 1}, {1, 3}, {16, 9}, {9, 16}, {100, 1}, {1, 100}};
    for (const auto& Shape : Shapes)
    {
        ComputeMinimapRect(208, 208, Shape[0] * 1000, Shape[1] * 1000, OffX, OffY, W, H);
        RA4_EXPECT(W <= 208.0 + 0.001);
        RA4_EXPECT(H <= 208.0 + 0.001);
        RA4_EXPECT(OffX >= -0.001 && OffY >= -0.001);
        // And the aspect ratio really is preserved.
        const double Want = Shape[0] / Shape[1];
        const double Got = W / H;
        RA4_EXPECT(Got > Want * 0.999 && Got < Want * 1.001);
    }

    // Degenerate input must not divide by zero or produce a negative rect.
    ComputeMinimapRect(208, 208, 0, 0, OffX, OffY, W, H);
    RA4_EXPECT(W >= 0.0 && H >= 0.0);
    ComputeMinimapRect(0, 0, 12800, 12800, OffX, OffY, W, H);
    RA4_EXPECT(W >= 0.0 && H >= 0.0);
}

// ---------------------------------------------------------------------------
// Minimap background: terrain and shroud (M2)
// ---------------------------------------------------------------------------

namespace
{
// A map with a river, a cliff ridge and an ore patch at known tiles, so the sampler can be
// asked what it produced for each of them rather than only whether it produced something.
struct BackgroundFixture
{
    ContentDatabase Content;
    SimWorld World;
    HudSnapshotBuilder Builder;
    HudSnapshot Snapshot;

    static constexpr int32_t kWaterX = 30;   // a full-height column of water
    static constexpr int32_t kCliffX = 40;   // a full-height column of cliff
    static constexpr int32_t kOreX = 12;     // a 2x2 ore patch next to the base
    static constexpr int32_t kOreY = 14;

    explicit BackgroundFixture(int32_t MapWidth = 64, int32_t MapHeight = 64)
    {
        BuildDefaultContent(Content);

        MatchSetup Setup = MakeTestSetup(9090);
        Setup.Map.Resize(MapWidth, MapHeight, Tile_GroundPassable);
        for (int32_t Y = 0; Y < MapHeight; ++Y)
        {
            if (kWaterX < MapWidth) { Setup.Map.SetTileFlag(kWaterX, Y, Tile_Water, true); }
            if (kCliffX < MapWidth) { Setup.Map.SetTileFlag(kCliffX, Y, Tile_Cliff, true); }
        }
        for (int32_t Y = kOreY; Y < kOreY + 2; ++Y)
        {
            for (int32_t X = kOreX; X < kOreX + 2; ++X)
            {
                Setup.Map.SetTileFlag(X, Y, Tile_Resource, true);
            }
        }

        World.Initialize(&Content, Setup);
        // Without an opponent the match ends on the first tick, which stops the fog from
        // updating and makes every order fail with MatchOver -- so the background would be
        // measured on a world that is not running.
        World.SpawnBuilding(Ids::AllConYard, 1, TileCoord(MapWidth - 6, MapHeight - 6), true);
        Builder.Initialize(0);
        // Every tick, so a test never has to guess when a re-sample lands.
        Builder.SetMinimapRefreshIntervalTicks(1);
    }

    void Step(int32_t Ticks = 1)
    {
        for (int32_t I = 0; I < Ticks; ++I)
        {
            World.Tick(nullptr);
            Builder.Build(World, {}, Snapshot);
            // A real consumer caches the grid and is only handed a new one when it changed, so
            // the fixture does the same. Reading Snapshot.Radar.Background directly every tick
            // would test a payload the shipping UI never receives.
            if (Snapshot.Radar.bBackgroundChanged)
            {
                Delivered = Snapshot.Radar.Background;
            }
            World.ClearEvents();
        }
    }

    // The most recently delivered background, held across the ticks that carried nothing.
    const MinimapBackground& Background() const { return Delivered; }

    MinimapBackground Delivered;

    // The cell a given tile falls into.
    MinimapTerrain TerrainAtTile(int32_t TileX, int32_t TileY) const
    {
        int32_t CellsX = 0, CellsY = 0, StrideX = 1, StrideY = 1;
        ComputeMinimapCellGrid(World.GetMap().Width, World.GetMap().Height,
                               CellsX, CellsY, StrideX, StrideY);
        const int32_t CellX = TileX / StrideX;
        const int32_t CellY = TileY / StrideY;
        const MinimapBackground& B = Background();
        if (CellX < 0 || CellY < 0 || CellX >= B.Width || CellY >= B.Height)
        {
            return MinimapTerrain::Unknown;
        }
        return MinimapTerrain(B.Terrain[size_t(CellY) * size_t(B.Width) + size_t(CellX)]);
    }

    MinimapShroud ShroudAtTile(int32_t TileX, int32_t TileY) const
    {
        int32_t CellsX = 0, CellsY = 0, StrideX = 1, StrideY = 1;
        ComputeMinimapCellGrid(World.GetMap().Width, World.GetMap().Height,
                               CellsX, CellsY, StrideX, StrideY);
        const int32_t CellX = TileX / StrideX;
        const int32_t CellY = TileY / StrideY;
        const MinimapBackground& B = Background();
        if (CellX < 0 || CellY < 0 || CellX >= B.Width || CellY >= B.Height)
        {
            return MinimapShroud::NeverSeen;
        }
        return MinimapShroud(B.Shroud[size_t(CellY) * size_t(B.Width) + size_t(CellX)]);
    }

    int32_t CountTerrain(MinimapTerrain Want) const
    {
        int32_t Count = 0;
        for (uint8_t V : Background().Terrain)
        {
            if (MinimapTerrain(V) == Want) { ++Count; }
        }
        return Count;
    }
};
} // namespace

// The panel drew markers onto an empty grid: a player could see where their units were but
// nothing about the ground under them. The background now carries the terrain the player
// has explored -- and, crucially, only that.
RA4_TEST(MinimapBackground, ExploredTerrainIsReportedAndUnexploredIsNot)
{
    BackgroundFixture F;
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    // The very first sample is new, so it is flagged as changed.
    F.Step(1);
    RA4_EXPECT(F.Snapshot.Radar.bBackgroundChanged);
    F.Step(3);

    RA4_EXPECT(F.Snapshot.Radar.BackgroundRevision > 0);
    RA4_EXPECT(F.Background().Width > 0);
    RA4_EXPECT(F.Background().Height > 0);
    RA4_EXPECT_EQ(F.Background().Terrain.size(),
                  size_t(F.Background().Width) * size_t(F.Background().Height));
    RA4_EXPECT_EQ(F.Background().Shroud.size(), F.Background().Terrain.size());

    // The yard's own cell reads as a structure -- its footprint sets Tile_Occupied, and a
    // building is the most worth-showing thing that can be in a cell.
    RA4_EXPECT_EQ(int32_t(F.TerrainAtTile(10, 10)), int32_t(MinimapTerrain::Structure));
    RA4_EXPECT_EQ(int32_t(F.ShroudAtTile(10, 10)), int32_t(MinimapShroud::Visible));

    // Open ground the yard can see, clear of its footprint but inside its 5-tile vision.
    RA4_EXPECT_EQ(int32_t(F.TerrainAtTile(10, 14)), int32_t(MinimapTerrain::Ground));
    RA4_EXPECT_EQ(int32_t(F.ShroudAtTile(10, 14)), int32_t(MinimapShroud::Visible));

    // The far corner has never been visited: no terrain and no shroud state.
    RA4_EXPECT_EQ(int32_t(F.TerrainAtTile(62, 62)), int32_t(MinimapTerrain::Unknown));
    RA4_EXPECT_EQ(int32_t(F.ShroudAtTile(62, 62)), int32_t(MinimapShroud::NeverSeen));
}

// Sampling terrain regardless of fog would draw the coastline, every cliff and every ore
// patch on a map the player has not set foot on -- the exact maphack the fog exists to
// prevent, delivered by the minimap instead of by the unit vision code.
RA4_TEST(MinimapBackground, UnexploredTerrainIsNotLeaked)
{
    BackgroundFixture F;
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    F.Step(4);

    // The river and the ridge are 20 and 30 tiles away, far outside the yard's sight.
    RA4_EXPECT_EQ(int32_t(F.TerrainAtTile(BackgroundFixture::kWaterX, 40)),
                  int32_t(MinimapTerrain::Unknown));
    RA4_EXPECT_EQ(int32_t(F.TerrainAtTile(BackgroundFixture::kCliffX, 40)),
                  int32_t(MinimapTerrain::Unknown));

    // Nothing anywhere on the map reports water or cliff yet, so the leak cannot be hiding
    // in a cell the two probes above happened to miss.
    RA4_EXPECT_EQ(F.CountTerrain(MinimapTerrain::Water), 0);
    RA4_EXPECT_EQ(F.CountTerrain(MinimapTerrain::Cliff), 0);
}

// Ore is the one thing on the minimap a player actively hunts for, so a cell covering both
// ore and plain ground must read as ore rather than being averaged away.
RA4_TEST(MinimapBackground, OreSurvivesDownsamplingAndIsVisibleWhenExplored)
{
    // A map large enough that one minimap cell covers several tiles. On a 64x64 map the
    // stride is 1 and each cell is a single tile, so the merge rule is never exercised --
    // an earlier version of this test used that size and passed with the rule removed.
    BackgroundFixture F(/*MapWidth*/ 256, /*MapHeight*/ 256);
    int32_t CellsX = 0, CellsY = 0, StrideX = 0, StrideY = 0;
    ComputeMinimapCellGrid(256, 256, CellsX, CellsY, StrideX, StrideY);
    RA4_REQUIRE(StrideX > 1);   // otherwise this test proves nothing
    // Near enough that the patch is inside the yard's vision, but not on top of it -- a
    // footprint would mask the ore with Structure and the test would prove nothing.
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(BackgroundFixture::kOreX + 5,
                                                        BackgroundFixture::kOreY), true);
    F.Step(4);

    RA4_EXPECT_EQ(int32_t(F.TerrainAtTile(BackgroundFixture::kOreX + 1,
                                          BackgroundFixture::kOreY + 1)),
                  int32_t(MinimapTerrain::Ore));
    RA4_EXPECT(F.CountTerrain(MinimapTerrain::Ore) > 0);
}

// Ground the player once walked past but no longer watches must be dimmed rather than
// either erased or left looking live -- that difference is the whole memory of the map.
RA4_TEST(MinimapBackground, GroundOnceSeenBecomesRememberedNotForgotten)
{
    BackgroundFixture F;
    // A lone scout well away from the base, so nothing else keeps its ground lit.
    const EntityId Scout = F.World.SpawnUnit(Ids::AllRifleman, 0, Vec2::FromInts(20 * 200, 45 * 200));
    F.Step(4);
    RA4_EXPECT_EQ(int32_t(F.ShroudAtTile(20, 45)), int32_t(MinimapShroud::Visible));
    const MinimapTerrain SeenTerrain = F.TerrainAtTile(20, 45);
    RA4_EXPECT_EQ(int32_t(SeenTerrain), int32_t(MinimapTerrain::Ground));

    // Walk the eyes away rather than deleting them, so this exercises the same path a real
    // match takes when a scout moves on. The cell must drop to Remembered, not back to
    // NeverSeen, and must keep the terrain the player learned.
    Command Away = MakeCommand(CommandType::Move, 0);
    Away.Primary = Scout;
    Away.Location = Vec2::FromInts(20 * 200, 12 * 200);
    RA4_EXPECT(F.World.ApplyCommand(Away).IsAccepted());
    F.Step(200);
    RA4_EXPECT_EQ(int32_t(F.ShroudAtTile(20, 45)), int32_t(MinimapShroud::Remembered));
    RA4_EXPECT_EQ(int32_t(F.TerrainAtTile(20, 45)), int32_t(MinimapTerrain::Ground));
}

// A background that is copied into the snapshot 20 times a second to say "identical" is the
// single most expensive thing here. It must only be sent when it actually changed.
RA4_TEST(MinimapBackground, IdenticalBackgroundIsNotResentEveryTick)
{
    BackgroundFixture F;
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);

    // First sample: new, so it is sent.
    F.Step(1);
    RA4_EXPECT(F.Snapshot.Radar.bBackgroundChanged);
    const uint32_t FirstRevision = F.Snapshot.Radar.BackgroundRevision;
    RA4_EXPECT(FirstRevision > 0);

    // Let exploration settle, then confirm a static map produces no further traffic.
    F.Step(20);
    const uint32_t Settled = F.Snapshot.Radar.BackgroundRevision;
    int32_t Resends = 0;
    for (int32_t I = 0; I < 20; ++I)
    {
        F.Step(1);
        if (F.Snapshot.Radar.bBackgroundChanged) { ++Resends; }
    }
    RA4_EXPECT_EQ(Resends, 0);
    RA4_EXPECT_EQ(int32_t(F.Snapshot.Radar.BackgroundRevision), int32_t(Settled));
    // The revision is still reported on the quiet ticks, so a consumer that missed one can
    // tell its cached copy is current.
    RA4_EXPECT(F.Snapshot.Radar.BackgroundRevision > 0);

    // Newly explored ground must still get through: a scout walking into the dark is
    // exactly the case a naive "only send once" cache would break.
    F.World.SpawnUnit(Ids::AllRifleman, 0, Vec2::FromInts(50 * 200, 50 * 200));
    F.Step(3);
    RA4_EXPECT(F.Snapshot.Radar.BackgroundRevision > Settled);
}

// A 512x512 map must not turn into a quarter-million-cell upload, and a small map must not
// be blurred by downsampling it when it already fits.
RA4_TEST(MinimapBackground, LargeMapsAreDownsampledAndSmallOnesAreNot)
{
    int32_t CellsX = 0, CellsY = 0, StrideX = 0, StrideY = 0;

    // Comfortably under the cap: sampled tile-for-tile.
    ComputeMinimapCellGrid(64, 64, CellsX, CellsY, StrideX, StrideY);
    RA4_EXPECT_EQ(StrideX, 1);
    RA4_EXPECT_EQ(StrideY, 1);
    RA4_EXPECT_EQ(CellsX, 64);
    RA4_EXPECT_EQ(CellsY, 64);

    // Over the cap: strides up and the cell count stays bounded.
    ComputeMinimapCellGrid(512, 512, CellsX, CellsY, StrideX, StrideY);
    RA4_EXPECT(StrideX > 1);
    RA4_EXPECT(CellsX <= kMinimapMaxCellsPerAxis);
    RA4_EXPECT(CellsY <= kMinimapMaxCellsPerAxis);

    // Every axis length up to a large map stays within the cap and never loses a tile off
    // the end -- the cells must always cover the whole map.
    for (int32_t Size = 1; Size <= 600; ++Size)
    {
        ComputeMinimapCellGrid(Size, Size, CellsX, CellsY, StrideX, StrideY);
        RA4_EXPECT(CellsX >= 1 && CellsX <= kMinimapMaxCellsPerAxis);
        RA4_EXPECT(StrideX >= 1);
        RA4_EXPECT(CellsX * StrideX >= Size);
    }

    // A non-square map keeps independent strides rather than being squashed to one.
    ComputeMinimapCellGrid(256, 64, CellsX, CellsY, StrideX, StrideY);
    RA4_EXPECT(StrideX > StrideY);
    RA4_EXPECT(CellsX > CellsY);

    // Degenerate input yields no grid rather than a divide by zero.
    ComputeMinimapCellGrid(0, 0, CellsX, CellsY, StrideX, StrideY);
    RA4_EXPECT_EQ(CellsX, 0);
    RA4_EXPECT_EQ(CellsY, 0);
}

// A real oversized map must actually produce a bounded, correctly-shaped background, not
// just satisfy the pure helper above.
RA4_TEST(MinimapBackground, OversizedMapProducesABoundedGrid)
{
    BackgroundFixture F(/*MapWidth*/ 256, /*MapHeight*/ 128);
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    F.Step(4);

    RA4_EXPECT(F.Background().Width <= kMinimapMaxCellsPerAxis);
    RA4_EXPECT(F.Background().Height <= kMinimapMaxCellsPerAxis);
    RA4_EXPECT_EQ(F.Background().Terrain.size(),
                  size_t(F.Background().Width) * size_t(F.Background().Height));
    // Aspect is preserved by the cell counts, so the widget's letterbox and the cell grid
    // agree about the shape of the map.
    RA4_EXPECT(F.Background().Width > F.Background().Height);
    // And the base is still findable in the downsampled grid.
    RA4_EXPECT_EQ(int32_t(F.ShroudAtTile(10, 10)), int32_t(MinimapShroud::Visible));
}

// ADR-0013 takes the overview away on a deficit, but it must not take away the map the
// player already explored: losing radar should cost the live picture, not the memory.
RA4_TEST(MinimapBackground, BackgroundSurvivesARadarBlackout)
{
    BackgroundFixture F;
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    F.Step(4);
    const uint32_t Explored = F.Snapshot.Radar.BackgroundRevision;
    RA4_EXPECT(Explored > 0);

    // Drive the tier down hard with unpowered turrets.
    for (int32_t N = 0; N < 12; ++N)
    {
        F.World.SpawnBuilding(Ids::SovTurret, 0, TileCoord(24 + N * 2, 24), true);
    }
    F.Step(6);

    // Whatever the panel decided about being online, the explored ground is still described
    // -- the revision never rewinds and the yard's own cell is still known terrain.
    RA4_EXPECT(F.Snapshot.Radar.BackgroundRevision >= Explored);
    RA4_EXPECT(F.Background().Width > 0);
    RA4_EXPECT_EQ(int32_t(F.TerrainAtTile(10, 10)), int32_t(MinimapTerrain::Structure));
}



// The Blueprint-facing minimap enums in RA4HUDTypes.h are a hand-written copy of these,
// and the sampler writes a raw byte that the painter reads back. RA4UI cannot be compiled
// headlessly, so the copy is guarded by static_assert in RA4SidebarWidget.cpp -- but that
// only fires if these values are the ones it was written against. Pinning them here means
// reordering the presentation enum breaks a test even in a headless run, instead of only
// when someone next builds the editor.
RA4_TEST(MinimapBackground, TerrainAndShroudValuesArePinnedForTheBlueprintMirror)
{
    RA4_EXPECT_EQ(int32_t(MinimapTerrain::Unknown), 0);
    RA4_EXPECT_EQ(int32_t(MinimapTerrain::Ground), 1);
    RA4_EXPECT_EQ(int32_t(MinimapTerrain::Water), 2);
    RA4_EXPECT_EQ(int32_t(MinimapTerrain::Cliff), 3);
    RA4_EXPECT_EQ(int32_t(MinimapTerrain::Ore), 4);
    RA4_EXPECT_EQ(int32_t(MinimapTerrain::Structure), 5);

    RA4_EXPECT_EQ(int32_t(MinimapShroud::NeverSeen), 0);
    RA4_EXPECT_EQ(int32_t(MinimapShroud::Remembered), 1);
    RA4_EXPECT_EQ(int32_t(MinimapShroud::Visible), 2);

    // Brightness ordering is relied on by the sampler, which keeps the brightest state any
    // tile in a cell is in. If Remembered ever outranked Visible, a cell straddling the edge
    // of vision would be drawn dimmed while the player is looking straight at it.
    RA4_EXPECT(MinimapShroud::Visible > MinimapShroud::Remembered);
    RA4_EXPECT(MinimapShroud::Remembered > MinimapShroud::NeverSeen);
}

// ---------------------------------------------------------------------------
// Minimap camera frame (M3)
// ---------------------------------------------------------------------------

// The panel showed where a player's forces were but not where the player was looking, which
// is the difference between an informative minimap and a navigable one.
RA4_TEST(MinimapCamera, FrameTracksTheViewAndFlipsY)
{
    // A square 208px panel showing a square 12800-unit map: the whole panel is the map.
    double OffX = 0, OffY = 0, W = 0, H = 0;
    ComputeMinimapRect(208, 208, 12800, 12800, OffX, OffY, W, H);

    double L = 0, T = 0, R = 0, B = 0;

    // Camera dead centre, looking at a quarter of the map per axis.
    RA4_REQUIRE(ComputeMinimapCameraFrame(OffX, OffY, W, H, 12800, 12800,
                                          6400, 6400, 3200, 3200, L, T, R, B));
    RA4_EXPECT_NEAR(L, 78.0, 0.01);    // (0.5 - 0.125) * 208
    RA4_EXPECT_NEAR(R, 130.0, 0.01);
    RA4_EXPECT_NEAR(T, 78.0, 0.01);
    RA4_EXPECT_NEAR(B, 130.0, 0.01);

    // Looking at the north edge. The simulation's Y grows northward and the panel's grows
    // downward, so a northern view must sit at the TOP of the panel. Without the flip the
    // frame would appear at the bottom -- diametrically wrong, and the sort of error that
    // looks plausible until a player tries to navigate with it.
    RA4_REQUIRE(ComputeMinimapCameraFrame(OffX, OffY, W, H, 12800, 12800,
                                          6400, 12800 - 1600, 3200, 3200, L, T, R, B));
    RA4_EXPECT(T < 52.0);              // in the upper quarter
    RA4_EXPECT_NEAR(B, 52.0, 0.01);

    // Looking at the south edge: the mirror image, at the bottom.
    RA4_REQUIRE(ComputeMinimapCameraFrame(OffX, OffY, W, H, 12800, 12800,
                                          6400, 1600, 3200, 3200, L, T, R, B));
    RA4_EXPECT_NEAR(T, 156.0, 0.01);
    RA4_EXPECT(B > 156.0);

    // And east/west are not flipped: looking east puts the frame on the right.
    RA4_REQUIRE(ComputeMinimapCameraFrame(OffX, OffY, W, H, 12800, 12800,
                                          12800 - 1600, 6400, 3200, 3200, L, T, R, B));
    RA4_EXPECT(R > 156.0);
    RA4_EXPECT_NEAR(L, 156.0, 0.01);
}

// A camera looking past the boundary must produce a frame flush with the edge, not one drawn
// outside the widget over the resource bar beneath it.
RA4_TEST(MinimapCamera, FrameIsClampedToTheMapAndNeverLeavesThePanel)
{
    double OffX = 0, OffY = 0, W = 0, H = 0;
    ComputeMinimapRect(208, 208, 12800, 12800, OffX, OffY, W, H);
    double L = 0, T = 0, R = 0, B = 0;

    // Centred on the corner, so half the footprint is off the map.
    RA4_REQUIRE(ComputeMinimapCameraFrame(OffX, OffY, W, H, 12800, 12800,
                                          0, 0, 4000, 4000, L, T, R, B));
    RA4_EXPECT_NEAR(L, 0.0, 0.01);
    RA4_EXPECT_NEAR(B, 208.0, 0.01);
    RA4_EXPECT(R <= 208.0 && T >= 0.0);

    // A footprint larger than the whole map collapses to the full panel rather than
    // overflowing it.
    RA4_REQUIRE(ComputeMinimapCameraFrame(OffX, OffY, W, H, 12800, 12800,
                                          6400, 6400, 999999, 999999, L, T, R, B));
    RA4_EXPECT_NEAR(L, 0.0, 0.01);
    RA4_EXPECT_NEAR(T, 0.0, 0.01);
    RA4_EXPECT_NEAR(R, 208.0, 0.01);
    RA4_EXPECT_NEAR(B, 208.0, 0.01);

    // Sweeping the camera across and down the map, the frame stays inside the panel and
    // never inverts (right below left, or bottom above top).
    for (int32_t Step = 0; Step <= 20; ++Step)
    {
        const double Along = 12800.0 * double(Step) / 20.0;
        RA4_REQUIRE(ComputeMinimapCameraFrame(OffX, OffY, W, H, 12800, 12800,
                                              Along, Along, 2500, 1800, L, T, R, B));
        RA4_EXPECT(L >= -0.001 && T >= -0.001);
        RA4_EXPECT(R <= 208.001 && B <= 208.001);
        RA4_EXPECT(R >= L);
        RA4_EXPECT(B >= T);
    }
}

// The frame must respect the letterbox, or on a non-square map it would be drawn over the
// bars -- outside the map it claims to describe.
RA4_TEST(MinimapCamera, FrameRespectsTheLetterboxOnANonSquareMap)
{
    // A 2:1 map in a square panel: 208 wide, 104 tall, centred with 52px bars.
    double OffX = 0, OffY = 0, W = 0, H = 0;
    ComputeMinimapRect(208, 208, 25600, 12800, OffX, OffY, W, H);
    RA4_REQUIRE(int32_t(OffY) == 52);

    double L = 0, T = 0, R = 0, B = 0;
    RA4_REQUIRE(ComputeMinimapCameraFrame(OffX, OffY, W, H, 25600, 12800,
                                          12800, 6400, 2560, 1280, L, T, R, B));
    // Vertically inside the map strip, not the panel: never in a bar.
    RA4_EXPECT(T >= 52.0 - 0.001);
    RA4_EXPECT(B <= 156.0 + 0.001);
    // Horizontally centred, since the camera is at the middle of the map.
    RA4_EXPECT_NEAR((L + R) * 0.5, 104.0, 0.01);
}

// An unknown footprint must be reported as "do not draw" rather than as a zero-size frame,
// which would leave a stray dot in the corner implying the player is looking there.
RA4_TEST(MinimapCamera, UnknownOrDegenerateInputDrawsNothing)
{
    double OffX = 0, OffY = 0, W = 0, H = 0;
    ComputeMinimapRect(208, 208, 12800, 12800, OffX, OffY, W, H);
    double L = 1, T = 1, R = 1, B = 1;

    // Zero extent: the controller could not deproject the viewport corners.
    RA4_EXPECT(!ComputeMinimapCameraFrame(OffX, OffY, W, H, 12800, 12800,
                                          6400, 6400, 0, 0, L, T, R, B));
    // Negative extent is nonsense and must be refused rather than mirrored.
    RA4_EXPECT(!ComputeMinimapCameraFrame(OffX, OffY, W, H, 12800, 12800,
                                          6400, 6400, -100, -100, L, T, R, B));
    // No map yet.
    RA4_EXPECT(!ComputeMinimapCameraFrame(OffX, OffY, W, H, 0, 0,
                                          6400, 6400, 3200, 3200, L, T, R, B));
    // No panel rect yet.
    RA4_EXPECT(!ComputeMinimapCameraFrame(0, 0, 0, 0, 12800, 12800,
                                          6400, 6400, 3200, 3200, L, T, R, B));
}

// ---------------------------------------------------------------------------
// Minimap alert pings (M4)
// ---------------------------------------------------------------------------

// The alert feed said "base under attack" but a line of text does not tell a player where to
// look, which is the whole reason a minimap matters during an attack.
RA4_TEST(MinimapPing, BaseUnderAttackRaisesAPingWhereItHappened)
{
    HudFixture F;
    SpawnEnemyOutpost(F.World);
    const EntityId Yard = F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    RA4_REQUIRE(Yard.IsValid());
    // Placing the yard is itself a located event, so it pings as a Construction. Wait for
    // that to expire, so what follows can only have come from the attack.
    F.Step(kRadarPingLifetimeTicks + 2);
    for (const RadarPing& Existing : F.Snapshot.Radar.Pings)
    {
        RA4_EXPECT(Existing.Kind != RadarPingKind::Attack);
    }

    // Park an enemy next to the yard and let it open fire, which is what raises the alert.
    F.World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(11 * 200, 10 * 200));
    const RadarPing* Attack = nullptr;
    for (int32_t I = 0; I < 400 && Attack == nullptr; ++I)
    {
        F.Step(1);
        for (const RadarPing& Candidate : F.Snapshot.Radar.Pings)
        {
            if (Candidate.Kind == RadarPingKind::Attack) { Attack = &Candidate; break; }
        }
    }

    RA4_REQUIRE(Attack != nullptr);
    RA4_EXPECT(Attack->IntensityPercent > 0 && Attack->IntensityPercent <= 100);
    // Near the yard, not at the origin: a ping in the wrong place is worse than none.
    const int32_t PingTileX = int32_t(Attack->Position.X.ToIntFloor() / 200);
    const int32_t PingTileY = int32_t(Attack->Position.Y.ToIntFloor() / 200);
    RA4_EXPECT(PingTileX >= 8 && PingTileX <= 14);
    RA4_EXPECT(PingTileY >= 8 && PingTileY <= 14);
}

// A ping that never expires is a permanent dot the player learns to ignore.
RA4_TEST(MinimapPing, PingsFadeAndThenDisappear)
{
    // Fresh: full intensity.
    RA4_EXPECT_EQ(RadarPingIntensityPercent(/*Raised*/ 100, /*Now*/ 100, 60), 100);
    // Half way through its life: roughly half.
    RA4_EXPECT_EQ(RadarPingIntensityPercent(100, 130, 60), 50);
    // One tick before expiry: still visible.
    RA4_EXPECT(RadarPingIntensityPercent(100, 159, 60) > 0);
    // At and past expiry: gone.
    RA4_EXPECT_EQ(RadarPingIntensityPercent(100, 160, 60), 0);
    RA4_EXPECT_EQ(RadarPingIntensityPercent(100, 5000, 60), 0);

    // Monotonically decreasing, so a ping never brightens as it ages.
    int32_t Previous = 101;
    for (TickIndex Now = 100; Now <= 160; ++Now)
    {
        const int32_t Current = RadarPingIntensityPercent(100, Now, 60);
        RA4_EXPECT(Current <= Previous);
        Previous = Current;
    }

    // Degenerate lifetimes must not divide by zero or produce something above full.
    RA4_EXPECT_EQ(RadarPingIntensityPercent(100, 120, 0), 0);
    RA4_EXPECT_EQ(RadarPingIntensityPercent(100, 120, -5), 0);
    // A "future" tick is a caller ordering mistake; clamped to full rather than overflowing,
    // which would scale a marker past its own cell.
    RA4_EXPECT_EQ(RadarPingIntensityPercent(200, 100, 60), 100);
}

// A ping must mean "look here". Pinging the map for a condition with no location would put a
// marker somewhere arbitrary and teach the player that pings mean nothing.
RA4_TEST(MinimapPing, ConditionsWithNoPlaceOnTheMapDoNotPing)
{
    RadarPingKind Kind = RadarPingKind::Attack;

    // Located events do ping, and are categorised so the widget can colour them.
    RA4_EXPECT(RadarPingKindForAlert(AlertType::BaseUnderAttack, Kind));
    RA4_EXPECT(Kind == RadarPingKind::Attack);
    RA4_EXPECT(RadarPingKindForAlert(AlertType::UnitsUnderAttack, Kind));
    RA4_EXPECT(Kind == RadarPingKind::Attack);
    RA4_EXPECT(RadarPingKindForAlert(AlertType::BuildingLost, Kind));
    RA4_EXPECT(Kind == RadarPingKind::Loss);
    RA4_EXPECT(RadarPingKindForAlert(AlertType::UnitLost, Kind));
    RA4_EXPECT(Kind == RadarPingKind::Loss);
    RA4_EXPECT(RadarPingKindForAlert(AlertType::ConstructionComplete, Kind));
    RA4_EXPECT(Kind == RadarPingKind::Construction);
    RA4_EXPECT(RadarPingKindForAlert(AlertType::UnitReady, Kind));
    RA4_EXPECT(Kind == RadarPingKind::Construction);

    // Conditions do not.
    RA4_EXPECT(!RadarPingKindForAlert(AlertType::LowPower, Kind));
    RA4_EXPECT(!RadarPingKindForAlert(AlertType::InsufficientFunds, Kind));
    RA4_EXPECT(!RadarPingKindForAlert(AlertType::ResourcesDepleted, Kind));
    RA4_EXPECT(!RadarPingKindForAlert(AlertType::None, Kind));
}

// Conditions with no place on the map produce no ping. Verified through a real match, which
// is the case that actually occurs today: every alert type that can ping currently sets its
// location, so the builder's bHasLocation guard is unreachable from here.
//
// The guard is kept regardless -- it is one line, and the alternative failure mode is a
// cluster of pings stacked in the map corner if a future alert type ever forgets its
// coordinates. It is deliberately not claimed as tested: an assertion that cannot fail is
// worse than none, because it reads as coverage.
RA4_TEST(MinimapPing, ConditionOnlyMatchProducesNoPings)
{
    HudFixture F;
    SpawnEnemyOutpost(F.World);
    // A war factory with no reactor raises LowPower, which is a condition, not a place.
    F.World.SpawnBuilding(Ids::SovWarFactory, 0, TileCoord(14, 14), true);
    F.Step(6);

    RA4_REQUIRE(F.CountAlerts(AlertType::LowPower) > 0);   // the alert really did fire
    for (const RadarPing& Ping : F.Snapshot.Radar.Pings)
    {
        // Nothing is stacked in the corner, and nothing pinged for the power condition.
        RA4_EXPECT(Ping.Kind != RadarPingKind::Attack);
        RA4_EXPECT(!(Ping.Position.X.ToIntFloor() == 0 && Ping.Position.Y.ToIntFloor() == 0));
    }
}

// A base still being shelled must stay lit. Measuring the fade from when the alert first
// fired would let the ping expire mid-attack, which is precisely when it is needed.
RA4_TEST(MinimapPing, AnOngoingAttackKeepsItsPingLit)
{
    HudFixture F;
    SpawnEnemyOutpost(F.World);
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    F.Step(2);

    // Two riflemen, so the fire keeps coming for a long time.
    F.World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(11 * 200, 10 * 200));
    F.World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(11 * 200, 11 * 200));
    for (int32_t I = 0; I < 200 && F.Snapshot.Radar.Pings.empty(); ++I)
    {
        F.Step(1);
    }
    RA4_REQUIRE(!F.Snapshot.Radar.Pings.empty());

    // Well past a single ping's lifetime, the attack is still in progress, so a ping must
    // still be there.
    F.Step(kRadarPingLifetimeTicks * 2);
    bool bStillLit = false;
    for (const RadarPing& Ping : F.Snapshot.Radar.Pings)
    {
        if (Ping.Kind == RadarPingKind::Attack) { bStillLit = true; }
    }
    RA4_EXPECT(bStillLit);
}


// A widget that draws only the top few pings must get the ones that matter. Without an
// ordering it would get whatever the alert feed happened to hold, so a construction ping
// could hide an attack the player needs to see.
// A widget that draws only the top few pings must get the ones that matter. The alert feed is
// already ordered by severity, so an attack that has begun to fade still sits above a fresh
// construction there; the ping list has to agree, or the two views of the same event would
// disagree about which is urgent.
RA4_TEST(MinimapPing, UrgentPingsOutrankFresherHarmlessOnes)
{
    HudFixture F;
    SpawnEnemyOutpost(F.World);
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    F.Step(2);

    // Start an attack, then let it age a little.
    F.World.SpawnUnit(Ids::AllRifleman, 1, Vec2::FromInts(11 * 200, 10 * 200));
    bool bSawAttack = false;
    for (int32_t I = 0; I < 400 && !bSawAttack; ++I)
    {
        F.Step(1);
        for (const RadarPing& P : F.Snapshot.Radar.Pings)
        {
            if (P.Kind == RadarPingKind::Attack) { bSawAttack = true; }
        }
    }
    RA4_REQUIRE(bSawAttack);

    // Now finish a building far away, which is newer and therefore brighter than the attack.
    F.World.SpawnBuilding(Ids::SovPower, 0, TileCoord(40, 40), true);
    F.Step(1);

    const RadarPing* Attack = nullptr;
    const RadarPing* Construction = nullptr;
    int32_t AttackIndex = -1;
    int32_t ConstructionIndex = -1;
    for (int32_t I = 0; I < int32_t(F.Snapshot.Radar.Pings.size()); ++I)
    {
        const RadarPing& P = F.Snapshot.Radar.Pings[size_t(I)];
        if (P.Kind == RadarPingKind::Attack && Attack == nullptr) { Attack = &P; AttackIndex = I; }
        if (P.Kind == RadarPingKind::Construction && Construction == nullptr)
        {
            Construction = &P;
            ConstructionIndex = I;
        }
    }
    RA4_REQUIRE(Attack != nullptr);
    RA4_REQUIRE(Construction != nullptr);

    // The construction really is the fresher of the two, so this is the case that
    // distinguishes "most urgent first" from "newest first".
    RA4_REQUIRE(Construction->IntensityPercent >= Attack->IntensityPercent);
    // And the attack still comes first.
    RA4_EXPECT(AttackIndex < ConstructionIndex);
}

// Within one kind the freshest comes first, so among several attacks the widget shows the one
// that is happening right now.
RA4_TEST(MinimapPing, WithinAKindTheFreshestComesFirst)
{
    HudFixture F;
    SpawnEnemyOutpost(F.World);
    // Three separate buildings, so their construction pings are distinct alert rows of
    // differing ages rather than one merged row.
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    F.Step(1);
    F.World.SpawnBuilding(Ids::SovPower, 0, TileCoord(30, 30), true);
    F.Step(1);
    F.World.SpawnBuilding(Ids::SovBarracks, 0, TileCoord(40, 40), true);
    F.Step(1);
    RA4_REQUIRE(F.Snapshot.Radar.Pings.size() >= 3u);   // otherwise this proves nothing

    int32_t Previous = 101;
    bool bSawDifferingIntensities = false;
    for (const RadarPing& P : F.Snapshot.Radar.Pings)
    {
        RA4_EXPECT(P.Kind == RadarPingKind::Construction);   // all one kind, so age decides
        if (Previous <= 100 && P.IntensityPercent != Previous)
        {
            bSawDifferingIntensities = true;
        }
        RA4_EXPECT(P.IntensityPercent <= Previous);
        Previous = P.IntensityPercent;
    }
    // The pings really do differ in age, so the ordering was exercised rather than trivially
    // satisfied by three identical values.
    RA4_EXPECT(bSawDifferingIntensities);

    RA4_EXPECT(uint8_t(RadarPingKind::Attack) < uint8_t(RadarPingKind::Loss));
    RA4_EXPECT(uint8_t(RadarPingKind::Loss) < uint8_t(RadarPingKind::Construction));
}

// Every radar test above authored its own definition at runtime, because no shipped entity set
// Building.bIsRadar -- so the whole mechanic was reachable only from tests and a real match had
// no radar to build. This asserts the shipped content, so deleting the definition breaks a test
// rather than silently reverting the feature.
RA4_TEST(MinimapContent, ShippedFactionsHaveABuildableRadar)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);

    const EntityDef* Radar = Content.FindEntity(Ids::SovRadar);
    RA4_REQUIRE(Radar != nullptr);
    RA4_EXPECT(Radar->Building.bIsRadar);
    RA4_EXPECT(Radar->Kind == EntityKind::Building);
    // Reachable: it must be produced by something and cost something, or it exists on paper
    // only. A prerequisite chain no player can satisfy is the same as no radar.
    RA4_EXPECT(!Radar->Production.ProducedBy.empty());
    RA4_EXPECT(Radar->Production.Cost > 0);
    RA4_EXPECT(Radar->Production.BuildTimeTicks > 0);
    // Draws enough power to matter: ADR-0013's minimap row only fires under a real deficit,
    // and a radar that consumed almost nothing would never cause one.
    RA4_EXPECT(Radar->Building.PowerConsumed > 0);
    // T1, so a blackout does not make the radar unrebuildable -- T2+ production is throttled
    // from Severe, which would turn a power crisis into a permanent one.
    RA4_EXPECT(Radar->Production.Tier <= TechTier::T1);

    // Both playable factions, not just the Soviets: an asymmetry here would be a silent
    // balance bug rather than a design decision.
    const EntityDef* AllianceRadar = Content.FindEntity(MakeContentId("building.all.radar_complex"));
    RA4_REQUIRE(AllianceRadar != nullptr);
    RA4_EXPECT(AllianceRadar->Building.bIsRadar);
    RA4_EXPECT(!AllianceRadar->Production.ProducedBy.empty());
}

// The radar's power priority decides whether a deficit takes it, which is the entire ADR-0013
// row. Derived rather than hardcoded, so this pins the derivation against the shipped content.
RA4_TEST(MinimapContent, ShippedRadarIsTheFirstThingADeficitTakes)
{
    ContentDatabase Content;
    BuildDefaultContent(Content);
    SimWorld World;
    World.Initialize(&Content, MakeTestSetup(4242));
    SpawnEnemyOutpost(World);
    World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    World.SpawnBuilding(Ids::SovPower, 0, TileCoord(14, 10), true);
    const EntityId Radar = World.SpawnBuilding(Ids::SovRadar, 0, TileCoord(18, 10), true);
    RA4_REQUIRE(Radar.IsValid());
    RunTicks(World, 4);

    const BuildingComp* B = World.GetBuilding(Radar);
    RA4_REQUIRE(B != nullptr);
    // Auxiliary: the radar is what a deficit is supposed to take first, ahead of production
    // and defence.
    RA4_EXPECT(B->Priority == PowerPriority::Auxiliary);
    RA4_EXPECT(uint8_t(PowerPriority::Auxiliary) > uint8_t(PowerPriority::Production));
    RA4_EXPECT(uint8_t(PowerPriority::Auxiliary) > uint8_t(PowerPriority::Defense));

    // With power intact the panel is online; the shipped radar therefore actually works.
    HudSnapshotBuilder Builder;
    Builder.Initialize(0);
    HudSnapshot Snapshot;
    Builder.Build(World, {}, Snapshot);
    RA4_EXPECT(Snapshot.Radar.bOnline);
    RA4_EXPECT(!Snapshot.Radar.bOfflineForPower);

    // Now overload the grid. The radar's band goes offline before anything else, and the
    // panel reports itself dark rather than pretending to still have coverage.
    for (int32_t N = 0; N < 12; ++N)
    {
        World.SpawnBuilding(Ids::SovTurret, 0, TileCoord(24 + N * 2, 24), true);
    }
    RunTicks(World, 6);
    RA4_REQUIRE(World.GetPlayer(0).GetPowerTier() >= PowerTier::Moderate);
    Builder.Build(World, {}, Snapshot);
    RA4_EXPECT(!Snapshot.Radar.bOnline);
    RA4_EXPECT(Snapshot.Radar.bOfflineForPower);
}


// The contract says false when there is nothing to draw. Guarding only the input extent was
// not enough: a camera entirely off the map clamps every corner to the same bound, so the rect
// had zero area but the function still returned true -- and the widget drew a degenerate line
// against the map edge, implying the player was looking somewhere they were not.
//
// Found by an independent reviewer's probe, which reported ok=1 with w=0 h=0.
RA4_TEST(MinimapCamera, ACameraEntirelyOffTheMapDrawsNothing)
{
    double OffX = 0, OffY = 0, W = 0, H = 0;
    ComputeMinimapRect(208, 208, 12800, 12800, OffX, OffY, W, H);
    double L = 1, T = 1, R = 1, B = 1;

    // Far off every edge in turn. Each must refuse rather than return a zero-area rect.
    const double FarOff[][2] = {{-99999, -99999}, {99999, 99999}, {-99999, 6400},
                                {99999, 6400}, {6400, -99999}, {6400, 99999}};
    for (const auto& Centre : FarOff)
    {
        RA4_EXPECT(!ComputeMinimapCameraFrame(OffX, OffY, W, H, 12800, 12800,
                                              Centre[0], Centre[1], 2000, 2000, L, T, R, B));
    }

    // A camera that merely straddles an edge still has area, and must still be drawn --
    // otherwise the frame would vanish the moment the player panned to the map border.
    RA4_REQUIRE(ComputeMinimapCameraFrame(OffX, OffY, W, H, 12800, 12800,
                                          0, 6400, 4000, 4000, L, T, R, B));
    RA4_EXPECT(R > L);
    RA4_EXPECT(B > T);
}

// The whole point of the revision counter is that an unchanged map costs nothing. An earlier
// version copied the full grid into every snapshot regardless -- 14792 bytes on a 256x256 map,
// about 296 KB/s at 20 Hz, purely to say "identical". Measured and reported by an independent
// reviewer; the claimed saving did not exist.
RA4_TEST(MinimapBackground, AnUnchangedBackgroundCarriesNoPayload)
{
    BackgroundFixture F(/*MapWidth*/ 256, /*MapHeight*/ 256);
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);

    // Let exploration settle so the grid really is static.
    F.Step(40);
    RA4_REQUIRE(F.Background().Width > 0);   // something was delivered at least once

    int32_t TicksCarryingPayload = 0;
    for (int32_t I = 0; I < 40; ++I)
    {
        F.Step(1);
        if (!F.Snapshot.Radar.Background.Terrain.empty() ||
            !F.Snapshot.Radar.Background.Shroud.empty())
        {
            ++TicksCarryingPayload;
        }
    }
    RA4_EXPECT_EQ(TicksCarryingPayload, 0);
    // The revision is still reported, so a consumer can tell its cached copy is current.
    RA4_EXPECT(F.Snapshot.Radar.BackgroundRevision > 0);

    // Newly explored ground still gets through: the cache must not silence real changes.
    F.World.SpawnUnit(Ids::AllRifleman, 0, Vec2::FromInts(200 * 200, 200 * 200));
    bool bDelivered = false;
    for (int32_t I = 0; I < 40 && !bDelivered; ++I)
    {
        F.Step(1);
        if (F.Snapshot.Radar.bBackgroundChanged) { bDelivered = true; }
    }
    RA4_EXPECT(bDelivered);
}

// A consumer created mid-match has no cached grid, and the payload is no longer sent
// unconditionally -- so it needs a way to ask. Without this it would sit blank until the next
// scout happened to reveal something, which on a settled map could be the rest of the game.
RA4_TEST(MinimapBackground, ALateConsumerCanAskForTheGridToBeResent)
{
    BackgroundFixture F(/*MapWidth*/ 128, /*MapHeight*/ 128);
    F.World.SpawnBuilding(Ids::SovConYard, 0, TileCoord(10, 10), true);
    F.Step(40);

    // Settled: nothing is being sent.
    F.Step(1);
    RA4_REQUIRE(F.Snapshot.Radar.Background.Terrain.empty());

    // Ask, and the very next snapshot carries the full grid.
    F.Builder.RequestBackgroundResend();
    F.Step(1);
    RA4_EXPECT(F.Snapshot.Radar.bBackgroundChanged);
    RA4_EXPECT(!F.Snapshot.Radar.Background.Terrain.empty());
    RA4_EXPECT_EQ(F.Snapshot.Radar.Background.Terrain.size(),
                  size_t(F.Snapshot.Radar.Background.Width) *
                  size_t(F.Snapshot.Radar.Background.Height));

    // And it is a one-shot request, not a mode: the tick after is quiet again.
    F.Step(1);
    RA4_EXPECT(F.Snapshot.Radar.Background.Terrain.empty());
}
