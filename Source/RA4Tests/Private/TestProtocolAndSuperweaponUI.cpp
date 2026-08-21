// Copyright (c) Red Alert 4 project. Tests for Stage 11 (Protocol Tree UI & Superweapon Timers).
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Content/ContentDatabase.h"
#include "RA4Core/Fixed.h"
#include "RA4Core/Vector.h"
#include "RA4Presentation/ProtocolAndSuperweaponUI.h"
#include "RA4Simulation/ProtocolRuntime.h"
#include "RA4Simulation/SimWorld.h"

#include <memory>
#include <vector>

using namespace RA4;
using namespace RA4Test;

namespace
{

std::unique_ptr<ContentDatabase> MakeSuperweaponUIDatabase()
{
    auto Db = std::make_unique<ContentDatabase>();

    // ConYard
    {
        EntityDef E;
        E.Id = MakeContentId("building.sov.conyard");
        E.Name = "building.sov.conyard";
        E.Kind = EntityKind::Building;
        E.MaxHealth = 2000;
        E.Armor = ArmorClass::Building;
        E.Building.FootprintX = 2;
        E.Building.FootprintY = 2;
        E.Building.bIsConstructionYard = true;
        E.Building.PowerProduced = 100;
        Db->AddEntity(E);
    }

    // Superweapon Building
    {
        EntityDef E;
        E.Id = MakeContentId("building.sov.vacuum_imploder");
        E.Name = "building.sov.vacuum_imploder";
        E.Kind = EntityKind::Building;
        E.MaxHealth = 3000;
        E.Armor = ArmorClass::Building;
        E.Building.FootprintX = 3;
        E.Building.FootprintY = 3;
        E.Building.PowerConsumed = 50;
        E.Building.SuperweaponRechargeTicks = 200; // 10s @ 20Hz
        E.Building.SuperweaponDamage = 3000;
        E.Building.SuperweaponRadius = Fixed::FromInt(600);
        E.Building.SuperweaponWarhead = WarheadClass::Siege;
        Db->AddEntity(E);
    }

    return Db;
}

} // namespace

// --- 1. Protocol Tree View Model Binding & Cooldowns ---

RA4_TEST(ProtocolUI, ProtocolTreeViewModelBinding)
{
    ProtocolRuntime Protocols;
    Protocols.AwardExperience(0, 5000);
    Protocols.UnlockProtocol(0, "sov_protocol_production");
    Protocols.UnlockProtocol(0, "sov_protocol_orbital_strike");

    // Put Orbital Strike on cooldown at tick 100 (cooldown is 1200 ticks -> ready at 1300)
    auto& PlayerState = Protocols.GetPlayerStateMutable(0);
    PlayerState.Cooldowns.push_back({"sov_protocol_orbital_strike", 1300});


    ProtocolAndSuperweaponUI UI;
    const auto Nodes = UI.BuildProtocolTreeViewModel(0, Protocols, 100, 20.0f);

    RA4_REQUIRE(!Nodes.empty());

    bool bFoundProduction = false;
    bool bFoundOrbital = false;

    for (const auto& Node : Nodes)
    {
        if (Node.Id == "sov_protocol_production")
        {
            bFoundProduction = true;
            RA4_EXPECT_EQ(Node.bUnlocked, true);
            RA4_EXPECT_EQ(Node.bOnCooldown, false);
        }
        else if (Node.Id == "sov_protocol_orbital_strike")
        {
            bFoundOrbital = true;
            RA4_EXPECT_EQ(Node.bUnlocked, true);
            RA4_EXPECT_EQ(Node.bOnCooldown, true);
            RA4_EXPECT_NEAR(Node.CooldownRemainingSeconds, 60.0f, 0.1f);
            RA4_EXPECT_NEAR(Node.CooldownProgressFraction, 1.0f, 0.01f);
        }
    }

    RA4_EXPECT_EQ(bFoundProduction, true);
    RA4_EXPECT_EQ(bFoundOrbital, true);
}

// --- 2. Superweapon HUD Timers & MM:SS Formatting ---

RA4_TEST(ProtocolUI, SuperweaponHUDTimersAndFormatting)
{
    auto Content = MakeSuperweaponUIDatabase();
    MatchSetup Setup = MakeTestSetup(301);

    SimWorld World;
    World.Initialize(Content.get(), Setup);

    const ContentId ConYard = MakeContentId("building.sov.conyard");
    World.SpawnBuilding(ConYard, 0, TileCoord(2, 2), true);
    World.SpawnBuilding(ConYard, 1, TileCoord(50, 50), true);

    const ContentId SuperDef = MakeContentId("building.sov.vacuum_imploder");
    const EntityId SuperBld = World.SpawnBuilding(SuperDef, 0, TileCoord(10, 10), true);

    // Advance 50 ticks (2.5s out of 10.0s total recharge)
    for (int I = 0; I < 50; ++I)
    {
        World.Tick(nullptr);
    }

    ProtocolRuntime Protocols;
    ProtocolAndSuperweaponUI UI;

    const auto Timers = UI.BuildSuperweaponTimersViewModel(Protocols, World, 20.0f);
    RA4_REQUIRE(Timers.size() == 1u);
    RA4_EXPECT(Timers[0].BuildingEntity == SuperBld);
    RA4_EXPECT_EQ(Timers[0].OwnerPlayer, 0u);
    RA4_EXPECT_EQ(Timers[0].ChargePercent, 25);
    RA4_EXPECT_NEAR(Timers[0].RemainingSeconds, 7.5f, 0.1f);
    RA4_EXPECT_EQ(Timers[0].bReady, false);
    RA4_EXPECT_EQ(Timers[0].bPowered, true);

    // Countdown formatting checks
    RA4_EXPECT(ProtocolAndSuperweaponUI::FormatCountdownMMSS(7.5f) == "00:08");
    RA4_EXPECT(ProtocolAndSuperweaponUI::FormatCountdownMMSS(65.0f) == "01:05");
    RA4_EXPECT(ProtocolAndSuperweaponUI::FormatCountdownMMSS(125.0f) == "02:05");
}
