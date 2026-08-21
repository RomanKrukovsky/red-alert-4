// Copyright (c) Red Alert 4 project. Tests for Stage 17 (Garrison & Urban Warfare).
#include "TestFramework.h"
#include "TestHelpers.h"

#include "RA4Content/ContentDatabase.h"
#include "RA4Core/Fixed.h"
#include "RA4Core/Vector.h"
#include "RA4Simulation/GarrisonUrbanCombat.h"
#include "RA4Simulation/SimWorld.h"

#include <memory>

using namespace RA4;
using namespace RA4Test;

namespace
{

std::unique_ptr<ContentDatabase> MakeGarrisonContentDb()
{
    auto Db = std::make_unique<ContentDatabase>();
    Db->ResetDamageTableToDefaults();

    // Neutral Civilian Bank / Apartment
    {
        EntityDef E;
        E.Id = MakeContentId("building.neutral.bank");
        E.Name = "building.neutral.bank";
        E.Kind = EntityKind::Building;
        E.MaxHealth = 2500;
        E.Armor = ArmorClass::Building;
        E.Building.FootprintX = 3;
        E.Building.FootprintY = 3;
        Db->AddEntity(E);
    }

    // Soviet Conscript
    {
        EntityDef E;
        E.Id = MakeContentId("unit.sov.conscript");
        E.Name = "unit.sov.conscript";
        E.Kind = EntityKind::Unit;
        E.MaxHealth = 100;
        E.Armor = ArmorClass::LightInfantry;
        E.Production.Cost = 100;
        E.Unit.Layer = MovementLayer::Infantry;
        E.Unit.MaxSpeed = Fixed::FromInt(100);
        E.Unit.Acceleration = Fixed::FromInt(400);
        E.Unit.TurnRatePerSecond = 2048;
        E.Unit.CollisionRadius = Fixed::FromInt(10);
        Db->AddEntity(E);
    }

    return Db;
}

} // namespace

// --- 1. Garrison Entry, Capacity Limits & Evacuation ---

RA4_TEST(GarrisonUrbanCombat, InfantryEnterCapacityAndEvacuation)
{
    auto Content = MakeGarrisonContentDb();
    MatchSetup Setup = MakeTestSetup(701);

    SimWorld World;
    World.Initialize(Content.get(), Setup);

    const ContentId BankDef = MakeContentId("building.neutral.bank");
    const ContentId ConscriptDef = MakeContentId("unit.sov.conscript");

    const EntityId Bank = World.SpawnBuilding(BankDef, kInvalidPlayer, TileCoord(10, 10), true);

    const EntityId Squad1 = World.SpawnUnit(ConscriptDef, 0, Vec2(Fixed::FromInt(1000), Fixed::FromInt(1000)));
    const EntityId Squad2 = World.SpawnUnit(ConscriptDef, 0, Vec2(Fixed::FromInt(1010), Fixed::FromInt(1000)));
    const EntityId Squad3 = World.SpawnUnit(ConscriptDef, 0, Vec2(Fixed::FromInt(1020), Fixed::FromInt(1000)));
    const EntityId EnemySquad = World.SpawnUnit(ConscriptDef, 1, Vec2(Fixed::FromInt(1030), Fixed::FromInt(1000)));

    GarrisonUrbanCombat GarrisonSys;
    GarrisonSys.RegisterBuilding(Bank, 2); // Max capacity 2 squads

    // Enter first two squads -> success
    RA4_EXPECT_EQ(GarrisonSys.EnterGarrison(Bank, Squad1, World), true);
    RA4_EXPECT_EQ(GarrisonSys.EnterGarrison(Bank, Squad2, World), true);

    // Enter third squad -> capacity exceeded
    RA4_EXPECT_EQ(GarrisonSys.EnterGarrison(Bank, Squad3, World), false);

    // Enemy squad tries to enter occupied garrison -> blocked
    RA4_EXPECT_EQ(GarrisonSys.EnterGarrison(Bank, EnemySquad, World), false);

    const auto* G = GarrisonSys.FindGarrison(Bank);
    RA4_EXPECT(G != nullptr);
    RA4_EXPECT_EQ(G->Occupants.size(), size_t(2));
    RA4_EXPECT_EQ(G->Controller, 0);

    // Evacuate
    GarrisonSys.EvacuateGarrison(Bank, World);
    RA4_EXPECT_EQ(G->Occupants.empty(), true);
    RA4_EXPECT_EQ(G->Controller, kInvalidPlayer);
}

// --- 2. Anti-Garrison Incendiary Attack ---

RA4_TEST(GarrisonUrbanCombat, AntiGarrisonFlameWeaponClearing)
{
    auto Content = MakeGarrisonContentDb();
    MatchSetup Setup = MakeTestSetup(702);

    SimWorld World;
    World.Initialize(Content.get(), Setup);

    const ContentId BankDef = MakeContentId("building.neutral.bank");
    const ContentId ConscriptDef = MakeContentId("unit.sov.conscript");

    const EntityId Bank = World.SpawnBuilding(BankDef, kInvalidPlayer, TileCoord(10, 10), true);

    const EntityId Squad1 = World.SpawnUnit(ConscriptDef, 1, Vec2(Fixed::FromInt(1000), Fixed::FromInt(1000)));
    const EntityId Squad2 = World.SpawnUnit(ConscriptDef, 1, Vec2(Fixed::FromInt(1010), Fixed::FromInt(1000)));

    GarrisonUrbanCombat GarrisonSys;
    GarrisonSys.RegisterBuilding(Bank, 5);

    GarrisonSys.EnterGarrison(Bank, Squad1, World);
    GarrisonSys.EnterGarrison(Bank, Squad2, World);

    // Apply Flamethrower flame stream to clear garrison
    const bool bCleared = GarrisonSys.ApplyAntiGarrisonAttack(Bank, WarheadClass::Flame, 200, World);

    RA4_EXPECT_EQ(bCleared, true);

    // Occupants burned out
    RA4_EXPECT_EQ(World.IsAlive(Squad1), false);
    RA4_EXPECT_EQ(World.IsAlive(Squad2), false);

    const auto* G = GarrisonSys.FindGarrison(Bank);
    RA4_EXPECT(G != nullptr);
    RA4_EXPECT_EQ(G->Occupants.empty(), true);
}
