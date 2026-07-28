// Copyright (c) Red Alert 4 project.
//
// Milestone content set: Soviet and Alliance base tech, enough to play the full
// economy -> production -> combat -> victory loop. Eastern Coalition and Chrono
// Legion are not defined yet (see Docs/Roadmap.md).
//
// Every player-facing string is a localization key. No display text is authored in
// C++, which is the requirement that makes the Licensed / Clean-Room content
// profiles a data swap rather than a code change.
#include "RA4Content/ContentDatabase.h"

#include "RA4Core/SimConfig.h"

namespace RA4
{
namespace
{

// --- Weapon ids -----------------------------------------------------------
constexpr ContentId WpnRifle = MakeContentId("weapon.rifle");
constexpr ContentId WpnRocketLauncher = MakeContentId("weapon.rocket_launcher");
constexpr ContentId WpnTankCannonLight = MakeContentId("weapon.tank_cannon_light");
constexpr ContentId WpnTankCannonHeavy = MakeContentId("weapon.tank_cannon_heavy");
constexpr ContentId WpnTurretCannon = MakeContentId("weapon.turret_cannon");
constexpr ContentId WpnTurretMachineGun = MakeContentId("weapon.turret_machinegun");
constexpr ContentId WpnPrismBeam = MakeContentId("weapon.prism_beam");

// --- Building ids ---------------------------------------------------------
constexpr ContentId BldSovConYard = MakeContentId("building.sov.construction_yard");
constexpr ContentId BldSovPower = MakeContentId("building.sov.tesla_reactor");
constexpr ContentId BldSovRefinery = MakeContentId("building.sov.ore_refinery");
constexpr ContentId BldSovBarracks = MakeContentId("building.sov.barracks");
constexpr ContentId BldSovWarFactory = MakeContentId("building.sov.war_factory");
constexpr ContentId BldSovTurret = MakeContentId("building.sov.gun_turret");

constexpr ContentId BldAllConYard = MakeContentId("building.all.construction_yard");
constexpr ContentId BldAllPower = MakeContentId("building.all.power_plant");
constexpr ContentId BldAllRefinery = MakeContentId("building.all.ore_refinery");
constexpr ContentId BldAllBarracks = MakeContentId("building.all.barracks");
constexpr ContentId BldAllWarFactory = MakeContentId("building.all.war_factory");
constexpr ContentId BldAllTurret = MakeContentId("building.all.pillbox");

// --- Unit ids -------------------------------------------------------------
constexpr ContentId UnitSovMcv = MakeContentId("unit.sov.mcv");
constexpr ContentId UnitSovHarvester = MakeContentId("unit.sov.ore_harvester");
constexpr ContentId UnitSovConscript = MakeContentId("unit.sov.conscript");
constexpr ContentId UnitSovRocketeer = MakeContentId("unit.sov.rocket_trooper");
constexpr ContentId UnitSovHeavyTank = MakeContentId("unit.sov.heavy_tank");

constexpr ContentId UnitAllMcv = MakeContentId("unit.all.mcv");
constexpr ContentId UnitAllHarvester = MakeContentId("unit.all.ore_harvester");
constexpr ContentId UnitAllRifleman = MakeContentId("unit.all.rifleman");
constexpr ContentId UnitAllRocketeer = MakeContentId("unit.all.missile_infantry");
constexpr ContentId UnitAllLightTank = MakeContentId("unit.all.light_tank");

constexpr ContentId ResOreField = MakeContentId("resource.ore_field");

Fixed Metres(int64_t M) { return Fixed::FromInt(M * 100); }

void BuildWeapons(ContentDatabase& Db)
{
    {
        WeaponDef W;
        W.Id = WpnRifle;
        W.Name = "weapon.rifle";
        W.Damage = 15;
        W.Warhead = WarheadClass::SmallArms;
        W.MaxRange = Metres(6);
        W.CooldownTicks = 10;
        W.bRequiresTurretAligned = false;
        Db.AddWeapon(W);
    }
    {
        WeaponDef W;
        W.Id = WpnRocketLauncher;
        W.Name = "weapon.rocket_launcher";
        W.Damage = 55;
        W.Warhead = WarheadClass::Rocket;
        W.MaxRange = Metres(9);
        W.CooldownTicks = 40;
        W.ProjectileSpeed = Metres(30);
        W.SplashRadius = Metres(2);
        W.bCanTargetAir = true;
        W.bRequiresTurretAligned = false;
        Db.AddWeapon(W);
    }
    {
        WeaponDef W;
        W.Id = WpnTankCannonLight;
        W.Name = "weapon.tank_cannon_light";
        W.Damage = 45;
        W.Warhead = WarheadClass::ArmorPiercing;
        W.MaxRange = Metres(8);
        W.CooldownTicks = 24;
        W.ProjectileSpeed = Metres(80);
        W.ScatterAtMaxRange = Fixed::FromInt(40);
        Db.AddWeapon(W);
    }
    {
        WeaponDef W;
        W.Id = WpnTankCannonHeavy;
        W.Name = "weapon.tank_cannon_heavy";
        W.Damage = 90;
        W.Warhead = WarheadClass::ArmorPiercing;
        W.MaxRange = Metres(9);
        W.CooldownTicks = 40;
        W.ProjectileSpeed = Metres(70);
        W.SplashRadius = Metres(1);
        W.ScatterAtMaxRange = Fixed::FromInt(60);
        Db.AddWeapon(W);
    }
    {
        WeaponDef W;
        W.Id = WpnTurretCannon;
        W.Name = "weapon.turret_cannon";
        W.Damage = 70;
        W.Warhead = WarheadClass::ArmorPiercing;
        W.MaxRange = Metres(11);
        W.CooldownTicks = 30;
        W.ProjectileSpeed = Metres(90);
        Db.AddWeapon(W);
    }
    {
        WeaponDef W;
        W.Id = WpnTurretMachineGun;
        W.Name = "weapon.turret_machinegun";
        W.Damage = 20;
        W.Warhead = WarheadClass::SmallArms;
        W.MaxRange = Metres(9);
        W.CooldownTicks = 8;
        W.bRequiresTurretAligned = false;
        Db.AddWeapon(W);
    }
    {
        WeaponDef W;
        W.Id = WpnPrismBeam;
        W.Name = "weapon.prism_beam";
        W.Damage = 60;
        W.Warhead = WarheadClass::Beam;
        W.MaxRange = Metres(12);
        W.CooldownTicks = 35;
        Db.AddWeapon(W);
    }
}

// Shared skeleton for the two mirrored base sets. The factions diverge in stats and
// will diverge structurally as faction-unique tech lands; sharing the constructor
// here avoids two copies of the same authoring mistakes in the meantime.
struct FactionSetup
{
    FactionId Faction;
    const char* KeyPrefix;
    ContentId ConYard, Power, Refinery, Barracks, WarFactory, Turret;
    ContentId Mcv, Harvester, BasicInfantry, AntiArmorInfantry, MainTank;
    const char* ConYardName;
    const char* PowerName;
    const char* RefineryName;
    const char* BarracksName;
    const char* WarFactoryName;
    const char* TurretName;
    const char* McvName;
    const char* HarvesterName;
    const char* BasicInfantryName;
    const char* AntiArmorInfantryName;
    const char* MainTankName;

    int32_t PowerOutput;
    int32_t TankHealth;
    ContentId TankWeapon;
    Fixed TankSpeed;
    int32_t TankCost;
    ContentId TurretWeapon;
};

void BuildFactionSet(ContentDatabase& Db, const FactionSetup& S)
{
    const std::string Prefix = S.KeyPrefix;

    // --- Construction yard -------------------------------------------------
    {
        EntityDef E;
        E.Id = S.ConYard;
        E.Name = S.ConYardName;
        E.DisplayNameKey = Prefix + ".building.construction_yard";
        E.Kind = EntityKind::Building;
        E.Faction = S.Faction;
        E.MaxHealth = 1500;
        E.Armor = ArmorClass::Building;
        E.VisionRange = Metres(10);
        E.Building.FootprintX = 3;
        E.Building.FootprintY = 3;
        E.Building.bIsConstructionYard = true;
        E.Building.bProvidesBuildRadius = true;
        E.Building.BuildRadius = Metres(20);
        E.Building.PowerConsumed = 0;
        E.Production.Cost = 2500;
        E.Production.BuildTimeTicks = SecondsToTicks(30);
        E.Production.Category = ProductionCategory::Structure;
        Db.AddEntity(E);
    }

    // --- Power plant -------------------------------------------------------
    {
        EntityDef E;
        E.Id = S.Power;
        E.Name = S.PowerName;
        E.DisplayNameKey = Prefix + ".building.power_plant";
        E.Kind = EntityKind::Building;
        E.Faction = S.Faction;
        E.MaxHealth = 600;
        E.Armor = ArmorClass::Building;
        E.VisionRange = Metres(8);
        E.Building.FootprintX = 2;
        E.Building.FootprintY = 2;
        E.Building.bIsPowerPlant = true;
        E.Building.PowerProduced = S.PowerOutput;
        E.Building.bProvidesBuildRadius = true;
        E.Building.BuildRadius = Metres(12);
        E.Production.Cost = 800;
        E.Production.BuildTimeTicks = SecondsToTicks(8);
        E.Production.Category = ProductionCategory::Structure;
        E.Production.ProducedBy = {S.ConYard};
        E.Production.Prerequisites = {S.ConYard};
        Db.AddEntity(E);
    }

    // --- Refinery ----------------------------------------------------------
    {
        EntityDef E;
        E.Id = S.Refinery;
        E.Name = S.RefineryName;
        E.DisplayNameKey = Prefix + ".building.refinery";
        E.Kind = EntityKind::Building;
        E.Faction = S.Faction;
        E.MaxHealth = 900;
        E.Armor = ArmorClass::Building;
        E.VisionRange = Metres(9);
        E.Building.FootprintX = 3;
        E.Building.FootprintY = 2;
        E.Building.bIsRefinery = true;
        E.Building.PowerConsumed = 30;
        E.Building.bProvidesBuildRadius = true;
        E.Building.BuildRadius = Metres(12);
        E.Building.BundledUnit = S.Harvester;   // C&C convention: refinery ships a harvester
        E.Production.Cost = 2000;
        E.Production.BuildTimeTicks = SecondsToTicks(20);
        E.Production.Category = ProductionCategory::Structure;
        E.Production.ProducedBy = {S.ConYard};
        E.Production.Prerequisites = {S.Power};
        Db.AddEntity(E);
    }

    // --- Barracks ----------------------------------------------------------
    {
        EntityDef E;
        E.Id = S.Barracks;
        E.Name = S.BarracksName;
        E.DisplayNameKey = Prefix + ".building.barracks";
        E.Kind = EntityKind::Building;
        E.Faction = S.Faction;
        E.MaxHealth = 800;
        E.Armor = ArmorClass::Building;
        E.VisionRange = Metres(8);
        E.Building.FootprintX = 2;
        E.Building.FootprintY = 2;
        E.Building.PowerConsumed = 20;
        E.Building.bProvidesBuildRadius = true;
        E.Building.BuildRadius = Metres(12);
        E.Production.Cost = 500;
        E.Production.BuildTimeTicks = SecondsToTicks(10);
        E.Production.Category = ProductionCategory::Structure;
        E.Production.ProducedBy = {S.ConYard};
        E.Production.Prerequisites = {S.Power};
        Db.AddEntity(E);
    }

    // --- War factory -------------------------------------------------------
    {
        EntityDef E;
        E.Id = S.WarFactory;
        E.Name = S.WarFactoryName;
        E.DisplayNameKey = Prefix + ".building.war_factory";
        E.Kind = EntityKind::Building;
        E.Faction = S.Faction;
        E.MaxHealth = 1000;
        E.Armor = ArmorClass::Building;
        E.VisionRange = Metres(8);
        E.Building.FootprintX = 3;
        E.Building.FootprintY = 3;
        E.Building.PowerConsumed = 50;
        E.Building.bProvidesBuildRadius = true;
        E.Building.BuildRadius = Metres(12);
        E.Production.Cost = 2000;
        E.Production.BuildTimeTicks = SecondsToTicks(20);
        E.Production.Category = ProductionCategory::Structure;
        E.Production.ProducedBy = {S.ConYard};
        E.Production.Prerequisites = {S.Refinery};
        Db.AddEntity(E);
    }

    // --- Defensive turret --------------------------------------------------
    {
        EntityDef E;
        E.Id = S.Turret;
        E.Name = S.TurretName;
        E.DisplayNameKey = Prefix + ".building.turret";
        E.Kind = EntityKind::Building;
        E.Faction = S.Faction;
        E.MaxHealth = 500;
        E.Armor = ArmorClass::Defense;
        E.VisionRange = Metres(12);
        E.Weapon = S.TurretWeapon;
        E.Building.FootprintX = 1;
        E.Building.FootprintY = 1;
        E.Building.PowerConsumed = 40;
        E.Production.Cost = 600;
        E.Production.BuildTimeTicks = SecondsToTicks(8);
        E.Production.Category = ProductionCategory::Defense;
        E.Production.ProducedBy = {S.ConYard};
        E.Production.Prerequisites = {S.Barracks};
        Db.AddEntity(E);
    }

    // --- MCV ---------------------------------------------------------------
    {
        EntityDef E;
        E.Id = S.Mcv;
        E.Name = S.McvName;
        E.DisplayNameKey = Prefix + ".unit.mcv";
        E.Kind = EntityKind::Unit;
        E.Faction = S.Faction;
        E.MaxHealth = 1000;
        E.Armor = ArmorClass::HeavyVehicle;
        E.VisionRange = Metres(8);
        E.Unit.Layer = MovementLayer::Wheeled;
        E.Unit.MaxSpeed = Metres(6);
        E.Unit.Acceleration = Metres(12);
        E.Unit.TurnRatePerSecond = 700;
        E.Unit.TurretTurnRatePerSecond = 0;
        E.Unit.CollisionRadius = Fixed::FromInt(120);
        E.Unit.bIsBuilder = true;
        E.Unit.DeploysInto = S.ConYard;
        E.Production.Cost = 2500;
        E.Production.BuildTimeTicks = SecondsToTicks(30);
        E.Production.Category = ProductionCategory::Vehicle;
        E.Production.ProducedBy = {S.WarFactory};
        E.Production.Prerequisites = {S.WarFactory};
        Db.AddEntity(E);
    }

    // --- Harvester ---------------------------------------------------------
    {
        EntityDef E;
        E.Id = S.Harvester;
        E.Name = S.HarvesterName;
        E.DisplayNameKey = Prefix + ".unit.harvester";
        E.Kind = EntityKind::Unit;
        E.Faction = S.Faction;
        E.MaxHealth = 600;
        E.Armor = ArmorClass::HeavyVehicle;
        E.VisionRange = Metres(6);
        E.Unit.Layer = MovementLayer::Tracked;
        E.Unit.MaxSpeed = Metres(7);
        E.Unit.Acceleration = Metres(14);
        E.Unit.TurnRatePerSecond = 900;
        E.Unit.CollisionRadius = Fixed::FromInt(100);
        E.Unit.bIsHarvester = true;
        E.Unit.CargoCapacity = 700;
        E.Unit.HarvestPerTick = 12;
        E.Unit.UnloadPerTick = 40;
        E.Unit.bCanCrushInfantry = true;
        E.Production.Cost = 1400;
        E.Production.BuildTimeTicks = SecondsToTicks(15);
        E.Production.Category = ProductionCategory::Vehicle;
        E.Production.ProducedBy = {S.WarFactory};
        E.Production.Prerequisites = {S.Refinery};
        Db.AddEntity(E);
    }

    // --- Basic infantry ----------------------------------------------------
    {
        EntityDef E;
        E.Id = S.BasicInfantry;
        E.Name = S.BasicInfantryName;
        E.DisplayNameKey = Prefix + ".unit.basic_infantry";
        E.Kind = EntityKind::Unit;
        E.Faction = S.Faction;
        E.MaxHealth = 100;
        E.Armor = ArmorClass::Infantry;
        E.VisionRange = Metres(7);
        E.Weapon = WpnRifle;
        E.Unit.Layer = MovementLayer::Infantry;
        E.Unit.MaxSpeed = Metres(4);
        E.Unit.Acceleration = Metres(20);
        E.Unit.TurnRatePerSecond = 4096;
        E.Unit.TurretTurnRatePerSecond = 0;
        E.Unit.CollisionRadius = Fixed::FromInt(35);
        E.Production.Cost = 100;
        E.Production.BuildTimeTicks = SecondsToTicks(3);
        E.Production.Category = ProductionCategory::Infantry;
        E.Production.ProducedBy = {S.Barracks};
        E.Production.Prerequisites = {S.Barracks};
        Db.AddEntity(E);
    }

    // --- Anti-armour infantry ---------------------------------------------
    {
        EntityDef E;
        E.Id = S.AntiArmorInfantry;
        E.Name = S.AntiArmorInfantryName;
        E.DisplayNameKey = Prefix + ".unit.antiarmor_infantry";
        E.Kind = EntityKind::Unit;
        E.Faction = S.Faction;
        E.MaxHealth = 90;
        E.Armor = ArmorClass::Infantry;
        E.VisionRange = Metres(8);
        E.Weapon = WpnRocketLauncher;
        E.Unit.Layer = MovementLayer::Infantry;
        E.Unit.MaxSpeed = Metres(3);
        E.Unit.Acceleration = Metres(20);
        E.Unit.TurnRatePerSecond = 4096;
        E.Unit.TurretTurnRatePerSecond = 0;
        E.Unit.CollisionRadius = Fixed::FromInt(35);
        E.Production.Cost = 300;
        E.Production.BuildTimeTicks = SecondsToTicks(5);
        E.Production.Category = ProductionCategory::Infantry;
        E.Production.ProducedBy = {S.Barracks};
        E.Production.Prerequisites = {S.Barracks};
        Db.AddEntity(E);
    }

    // --- Main battle tank --------------------------------------------------
    {
        EntityDef E;
        E.Id = S.MainTank;
        E.Name = S.MainTankName;
        E.DisplayNameKey = Prefix + ".unit.main_tank";
        E.Kind = EntityKind::Unit;
        E.Faction = S.Faction;
        E.MaxHealth = S.TankHealth;
        E.Armor = ArmorClass::HeavyVehicle;
        E.VisionRange = Metres(9);
        E.Weapon = S.TankWeapon;
        E.Unit.Layer = MovementLayer::Tracked;
        E.Unit.MaxSpeed = S.TankSpeed;
        E.Unit.Acceleration = Metres(16);
        E.Unit.TurnRatePerSecond = 800;
        E.Unit.TurretTurnRatePerSecond = 1600;
        E.Unit.CollisionRadius = Fixed::FromInt(110);
        E.Unit.bCanCrushInfantry = true;
        E.Production.Cost = S.TankCost;
        E.Production.BuildTimeTicks = SecondsToTicks(12);
        E.Production.Category = ProductionCategory::Vehicle;
        E.Production.ProducedBy = {S.WarFactory};
        E.Production.Prerequisites = {S.WarFactory};
        Db.AddEntity(E);
    }
}

} // namespace

void BuildDefaultContent(ContentDatabase& Db)
{
    Db.Clear();
    BuildWeapons(Db);

    // Soviet doctrine: more hit points, more damage, slower and more expensive, and
    // more power per reactor so the economy can absorb heavy defensive structures.
    FactionSetup Soviet{};
    Soviet.Faction = FactionId::Soviet;
    Soviet.KeyPrefix = "faction.soviet";
    Soviet.ConYard = BldSovConYard;
    Soviet.Power = BldSovPower;
    Soviet.Refinery = BldSovRefinery;
    Soviet.Barracks = BldSovBarracks;
    Soviet.WarFactory = BldSovWarFactory;
    Soviet.Turret = BldSovTurret;
    Soviet.Mcv = UnitSovMcv;
    Soviet.Harvester = UnitSovHarvester;
    Soviet.BasicInfantry = UnitSovConscript;
    Soviet.AntiArmorInfantry = UnitSovRocketeer;
    Soviet.MainTank = UnitSovHeavyTank;
    Soviet.ConYardName = "building.sov.construction_yard";
    Soviet.PowerName = "building.sov.tesla_reactor";
    Soviet.RefineryName = "building.sov.ore_refinery";
    Soviet.BarracksName = "building.sov.barracks";
    Soviet.WarFactoryName = "building.sov.war_factory";
    Soviet.TurretName = "building.sov.gun_turret";
    Soviet.McvName = "unit.sov.mcv";
    Soviet.HarvesterName = "unit.sov.ore_harvester";
    Soviet.BasicInfantryName = "unit.sov.conscript";
    Soviet.AntiArmorInfantryName = "unit.sov.rocket_trooper";
    Soviet.MainTankName = "unit.sov.heavy_tank";
    Soviet.PowerOutput = 150;
    Soviet.TankHealth = 520;
    Soviet.TankWeapon = WpnTankCannonHeavy;
    Soviet.TankSpeed = Metres(5);
    Soviet.TankCost = 1000;
    Soviet.TurretWeapon = WpnTurretCannon;
    BuildFactionSet(Db, Soviet);

    // Alliance doctrine: faster, cheaper, more fragile, weaker reactors, and
    // machine-gun defences that punish infantry rushes instead of armour.
    FactionSetup Alliance{};
    Alliance.Faction = FactionId::Alliance;
    Alliance.KeyPrefix = "faction.alliance";
    Alliance.ConYard = BldAllConYard;
    Alliance.Power = BldAllPower;
    Alliance.Refinery = BldAllRefinery;
    Alliance.Barracks = BldAllBarracks;
    Alliance.WarFactory = BldAllWarFactory;
    Alliance.Turret = BldAllTurret;
    Alliance.Mcv = UnitAllMcv;
    Alliance.Harvester = UnitAllHarvester;
    Alliance.BasicInfantry = UnitAllRifleman;
    Alliance.AntiArmorInfantry = UnitAllRocketeer;
    Alliance.MainTank = UnitAllLightTank;
    Alliance.ConYardName = "building.all.construction_yard";
    Alliance.PowerName = "building.all.power_plant";
    Alliance.RefineryName = "building.all.ore_refinery";
    Alliance.BarracksName = "building.all.barracks";
    Alliance.WarFactoryName = "building.all.war_factory";
    Alliance.TurretName = "building.all.pillbox";
    Alliance.McvName = "unit.all.mcv";
    Alliance.HarvesterName = "unit.all.ore_harvester";
    Alliance.BasicInfantryName = "unit.all.rifleman";
    Alliance.AntiArmorInfantryName = "unit.all.missile_infantry";
    Alliance.MainTankName = "unit.all.light_tank";
    Alliance.PowerOutput = 100;
    Alliance.TankHealth = 380;
    Alliance.TankWeapon = WpnTankCannonLight;
    Alliance.TankSpeed = Metres(8);
    Alliance.TankCost = 700;
    Alliance.TurretWeapon = WpnTurretMachineGun;
    BuildFactionSet(Db, Alliance);

    {
        ResourceNodeDef R;
        R.Id = ResOreField;
        R.Name = "resource.ore_field";
        R.InitialAmount = 8000;
        R.MaxAmount = 8000;
        R.ValuePerUnit = 1;
        R.bRegrows = false;
        Db.AddResourceNode(R);
    }

    {
        FactionDef F;
        F.Id = FactionId::Soviet;
        F.Name = "faction.soviet";
        F.DisplayNameKey = "faction.soviet.name";
        F.StartingUnit = UnitSovMcv;
        F.StartingCredits = 10000;
        Db.AddFaction(F);
    }
    {
        FactionDef F;
        F.Id = FactionId::Alliance;
        F.Name = "faction.alliance";
        F.DisplayNameKey = "faction.alliance.name";
        F.StartingUnit = UnitAllMcv;
        F.StartingCredits = 10000;
        Db.AddFaction(F);
    }
}

} // namespace RA4
