// Copyright (c) Red Alert 4 project. Data-driven definitions for every game object.
//
// These are plain structs with no engine dependency. In the editor build they are
// populated from Primary Data Assets; in tests and on the dedicated server they are
// populated from the same serialized content pack. The simulation only ever sees
// these structs, which is what allows a full rebrand (Licensed vs Clean-Room
// profile) without touching a line of C++.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "RA4Core/Fixed.h"
#include "RA4Core/Ids.h"
#include "RA4Core/Vector.h"

namespace RA4
{

// --- Taxonomy -------------------------------------------------------------

enum class ArmorClass : uint8_t
{
    None = 0,
    Infantry,
    LightVehicle,
    HeavyVehicle,
    Building,
    Defense,
    Aircraft,
    Naval,
    Count,
};

enum class WarheadClass : uint8_t
{
    SmallArms = 0,
    ArmorPiercing,
    HighExplosive,
    Rocket,
    Beam,
    Flame,
    EMP,
    Crush,
    Count,
};

// Which navigation layer an entity occupies. Separate layers let a submarine and a
// tank share a tile without interacting, and let bridges be passable only to the
// ground layers.
enum class MovementLayer : uint8_t
{
    None = 0,     // buildings, resource nodes
    Infantry,
    Wheeled,
    Tracked,
    Amphibious,
    Naval,
    Air,
    Count,
};

enum class EntityKind : uint8_t
{
    Unit = 0,
    Building,
    ResourceNode,
    Projectile,
    Count,
};

// Right-hand production tabs, in display order.
enum class ProductionCategory : uint8_t
{
    Structure = 0,
    Defense,
    Infantry,
    Vehicle,
    Aircraft,
    Naval,
    Ability,
    Count,
};

enum class FactionId : uint8_t
{
    None = 0,
    Soviet,
    Alliance,
    EasternCoalition,
    ChronoLegion,
    Count,
};

// --- Weapons --------------------------------------------------------------

struct WeaponDef
{
    ContentId Id;
    std::string Name;

    int32_t Damage = 0;
    WarheadClass Warhead = WarheadClass::SmallArms;

    Fixed MinRange = Fixed::Zero();     // artillery cannot fire inside this
    Fixed MaxRange = Fixed::FromInt(500);
    int32_t CooldownTicks = 20;
    int32_t BurstCount = 1;
    int32_t BurstDelayTicks = 0;

    // Zero speed means hitscan: damage is applied in the same tick the shot is
    // resolved. Non-zero spawns a simulated projectile entity.
    Fixed ProjectileSpeed = Fixed::Zero();
    Fixed SplashRadius = Fixed::Zero();
    int32_t SplashFalloffPercent = 50;  // damage at the splash edge

    bool bCanTargetGround = true;
    bool bCanTargetAir = false;
    bool bRequiresTurretAligned = true;
    // Accuracy is expressed as a scatter radius at max range rather than a hit
    // roll, so that misses land somewhere and can still splash.
    Fixed ScatterAtMaxRange = Fixed::Zero();
};

// --- Entities -------------------------------------------------------------

struct ProductionInfo
{
    int32_t Cost = 0;
    int32_t BuildTimeTicks = 0;
    ProductionCategory Category = ProductionCategory::Infantry;
    // Producer building types that can queue this item. Empty means "placed
    // directly by the player" (used by structures).
    std::vector<ContentId> ProducedBy;
    std::vector<ContentId> Prerequisites;
    // Refund fraction when a queued item is cancelled, in percent.
    int32_t CancelRefundPercent = 100;
};

struct BuildingInfo
{
    int32_t FootprintX = 1;
    int32_t FootprintY = 1;
    int32_t PowerProduced = 0;
    int32_t PowerConsumed = 0;

    bool bIsConstructionYard = false;
    bool bIsRefinery = false;        // harvesters unload here
    bool bIsPowerPlant = false;
    bool bIsRadar = false;
    bool bProvidesBuildRadius = false;
    Fixed BuildRadius = Fixed::Zero();

    // Unit that spawns with the building on completion (refinery -> free harvester,
    // matching the C&C convention).
    ContentId BundledUnit;

    int32_t SellRefundPercent = 50;
};

struct UnitInfo
{
    MovementLayer Layer = MovementLayer::Infantry;
    Fixed MaxSpeed = Fixed::FromInt(100);   // world units per second
    Fixed Acceleration = Fixed::FromInt(400);
    int32_t TurnRatePerSecond = 1024;       // angle units/s; hull rotation
    int32_t TurretTurnRatePerSecond = 2048; // 0 means no independent turret

    Fixed CollisionRadius = Fixed::FromInt(20);

    bool bIsHarvester = false;
    int32_t CargoCapacity = 0;
    int32_t HarvestPerTick = 0;
    int32_t UnloadPerTick = 0;

    bool bCanCrushInfantry = false;
    bool bIsBuilder = false;                // deployable MCV
    ContentId DeploysInto;
};

struct EntityDef
{
    ContentId Id;
    std::string Name;                 // stable key, e.g. "unit.sov.conscript"
    std::string DisplayNameKey;       // localization key -- never a literal
    EntityKind Kind = EntityKind::Unit;
    FactionId Faction = FactionId::None;

    int32_t MaxHealth = 100;
    ArmorClass Armor = ArmorClass::Infantry;
    Fixed VisionRange = Fixed::FromInt(600);

    ContentId Weapon;                 // primary weapon; invalid means unarmed
    ContentId SecondaryWeapon;

    ProductionInfo Production;
    BuildingInfo Building;
    UnitInfo Unit;
};

// --- Resources ------------------------------------------------------------

struct ResourceNodeDef
{
    ContentId Id;
    std::string Name;
    int32_t InitialAmount = 5000;
    int32_t ValuePerUnit = 1;       // credits per harvested unit
    bool bRegrows = false;
    int32_t RegrowPerTick = 0;
    int32_t MaxAmount = 5000;
};

// --- Faction --------------------------------------------------------------

struct FactionDef
{
    FactionId Id = FactionId::None;
    std::string Name;
    std::string DisplayNameKey;
    ContentId StartingUnit;         // typically the MCV
    int32_t StartingCredits = 10000;
};

} // namespace RA4
