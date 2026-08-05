// Copyright (c) Red Alert 4 project. Data-driven definitions for every game object.
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
    LightInfantry = 0,
    HeavyInfantry,
    LightVehicle,
    HeavyVehicle,
    SiegeVehicle,
    Air,
    Naval,
    Building,
    Shielded,
    Count,
    // Backwards compatibility aliases
    None = LightInfantry,
    Infantry = LightInfantry,
    Defense = Building,
    Aircraft = Air
};

enum class WarheadClass : uint8_t
{
    Ballistic = 0,
    Fragmentation,
    ArmorPiercing,
    Siege,
    Electric,
    Plasma,
    Cryogenic,
    Temporal,
    AntiAir,
    Count,
    // Backwards compatibility aliases
    SmallArms = Ballistic,
    HighExplosive = Fragmentation,
    Rocket = ArmorPiercing,
    Beam = Plasma,
    Flame = Fragmentation,
    EMP = Electric,
    Crush = Siege
};

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

enum class VeterancyRank : uint8_t
{
    Recruit = 0,
    Veteran,
    Elite,
    Heroic,
    Count
};

enum class EntityRole : uint32_t
{
    None         = 0,
    Harvester    = 1u << 0,
    Builder      = 1u << 1,
    Scout        = 1u << 2,
    Combat       = 1u << 3,
    AntiAir      = 1u << 4,
    AntiArmor    = 1u << 5,
    Artillery    = 1u << 6,
    Engineer     = 1u << 7,
    BaseBuilding = 1u << 8,
    Power        = 1u << 9,
    Refinery     = 1u << 10,
    Defense      = 1u << 11,
    Production   = 1u << 12,
};

inline constexpr EntityRole operator|(EntityRole A, EntityRole B)
{
    return static_cast<EntityRole>(static_cast<uint32_t>(A) | static_cast<uint32_t>(B));
}

inline constexpr EntityRole operator&(EntityRole A, EntityRole B)
{
    return static_cast<EntityRole>(static_cast<uint32_t>(A) & static_cast<uint32_t>(B));
}

inline constexpr EntityRole operator~(EntityRole A)
{
    return static_cast<EntityRole>(~static_cast<uint32_t>(A));
}

inline EntityRole& operator|=(EntityRole& A, EntityRole B)
{
    A = A | B;
    return A;
}

inline EntityRole& operator&=(EntityRole& A, EntityRole B)
{
    A = A & B;
    return A;
}

inline bool HasRole(EntityRole Roles, EntityRole Target)
{
    return (static_cast<uint32_t>(Roles) & static_cast<uint32_t>(Target)) != 0;
}

// --- Weapons --------------------------------------------------------------

struct WeaponDef
{
    ContentId Id;
    std::string Name;

    int32_t Damage = 0;
    WarheadClass Warhead = WarheadClass::Ballistic;

    Fixed MinRange = Fixed::Zero();     // artillery cannot fire inside this
    Fixed MaxRange = Fixed::FromInt(500);
    int32_t CooldownTicks = 20;
    int32_t BurstCount = 1;
    int32_t BurstDelayTicks = 0;

    Fixed ProjectileSpeed = Fixed::Zero();
    Fixed SplashRadius = Fixed::Zero();
    int32_t SplashFalloffPercent = 50;  // damage at the splash edge

    bool bCanTargetGround = true;
    bool bCanTargetAir = false;
    bool bRequiresTurretAligned = true;
    Fixed ScatterAtMaxRange = Fixed::Zero();
};

// --- Entities -------------------------------------------------------------

struct PrerequisiteGroup
{
    std::vector<ContentId> AllOf;
    std::vector<ContentId> AnyOf;
    std::vector<ContentId> NoneOf;

    bool IsEmpty() const
    {
        return AllOf.empty() && AnyOf.empty() && NoneOf.empty();
    }
};

// --- Tech Tiers -----------------------------------------------------------

// Declared before ProductionInfo because that struct carries a Tier field. "High
// tech" in the ADR-0013 sense means T2 and above: those are the items a power deficit
// pauses outright rather than merely slowing.
enum class TechTier : uint8_t
{
    T0 = 0,   // starting buildings
    T1,
    T2,
    T3,
    Count
};

struct ProductionInfo
{
    int32_t Cost = 0;
    int32_t BuildTimeTicks = 0;
    int32_t CommandLimit = 1;
    ProductionCategory Category = ProductionCategory::Infantry;
    std::vector<ContentId> ProducedBy;
    std::vector<ContentId> Prerequisites;
    PrerequisiteGroup PrerequisitesGroup;
    int32_t CancelRefundPercent = 100;
    // ADR-0013. T0/T1 keep building through a deficit at reduced speed; T2+ is paused
    // at Severe and Critical. Defaulting to T0 means content that says nothing about
    // tech behaves exactly as it did before this field existed.
    TechTier Tier = TechTier::T0;
};

struct BuildingInfo
{
    int32_t FootprintX = 1;
    int32_t FootprintY = 1;
    int32_t PowerProduced = 0;
    int32_t PowerConsumed = 0;
    int32_t CommandLimitProvided = 0;

    bool bIsConstructionYard = false;
    bool bIsRefinery = false;        // harvesters unload here
    bool bIsPowerPlant = false;
    bool bIsRadar = false;
    bool bProvidesBuildRadius = false;
    Fixed BuildRadius = Fixed::Zero();

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
    int32_t CargoCapacity = 1200;
    int32_t HarvestPerTick = 20;
    int32_t UnloadPerTick = 50;

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
    EntityRole Roles = EntityRole::None;

    int32_t MaxHealth = 100;
    int32_t MaxShield = 0;
    ArmorClass Armor = ArmorClass::LightInfantry;
    Fixed VisionRange = Fixed::FromInt(600);

    ContentId Weapon;                 // primary weapon; invalid means unarmed
    ContentId SecondaryWeapon;

    std::string VoicePackId;          // maps to voice pack stable_id, e.g. "SU_RubezhRifleman"

    ProductionInfo Production;
    BuildingInfo Building;
    UnitInfo Unit;
    
    std::vector<std::string> Abilities;
};

// --- Resources ------------------------------------------------------------

struct ResourceNodeDef
{
    ContentId Id;
    std::string Name;
    int32_t InitialAmount = 45000;  // Standard ore field: 45000, Rich: 75000
    int32_t ValuePerUnit = 1;       // credits per harvested unit
    bool bIsRichOre = false;
    bool bRegrows = false;
    int32_t RegrowPerTick = 0;
    int32_t MaxAmount = 45000;
};

// --- Faction --------------------------------------------------------------

struct FactionDef
{
    FactionId Id = FactionId::None;
    std::string Name;
    std::string DisplayNameKey;
    ContentId StartingUnit;         // typically the MCV
    int32_t StartingCredits = 10000;
    int32_t StartingCommandLimit = 50;
    int32_t MaxCommandLimit = 200;
    std::string UniqueResourceName;
};

// --- Damage Matrix -------------------------------------------------------

// Multiplier[damageType][armorType] — read verbatim from the bible table.
// Stored as fixed-point thousandths (e.g. 1.5 → 1500) to avoid float.
struct DamageMatrixDef
{
    // Multipliers[warhead][armor], in per-mille (1000 = 1.0x).
    int32_t Multipliers[int32_t(WarheadClass::Count)][int32_t(ArmorClass::Count)] = {};

    int32_t GetMultiplier(WarheadClass W, ArmorClass A) const
    {
        const int32_t wi = int32_t(W);
        const int32_t ai = int32_t(A);
        if (wi < 0 || wi >= int32_t(WarheadClass::Count)) return 0;
        if (ai < 0 || ai >= int32_t(ArmorClass::Count)) return 0;
        return Multipliers[wi][ai];
    }

    void SetMultiplier(WarheadClass W, ArmorClass A, int32_t MultiplierPercent)
    {
        const int32_t wi = int32_t(W);
        const int32_t ai = int32_t(A);
        if (wi >= 0 && wi < int32_t(WarheadClass::Count) && ai >= 0 && ai < int32_t(ArmorClass::Count))
        {
            Multipliers[wi][ai] = MultiplierPercent;
        }
    }
};

// --- Veterancy ------------------------------------------------------------

struct VeterancyLevel
{
    int32_t CostThresholdMultiplier = 1;   // x times own cost destroyed
    int32_t DamageBonusPercent = 0;
    int32_t HpBonusPercent = 0;
    int32_t RegenPerTick = 0;
    bool bImprovedAbility = false;
    bool bHeroicPassive = false;
};

struct VeterancyDef
{
    VeterancyLevel Levels[int32_t(VeterancyRank::Count)];
};

// --- Faction Resources ---------------------------------------------------

enum class FactionResourceType : uint8_t
{
    None = 0,
    Mobilization,       // Soviet
    Intelligence,       // Alliance
    Synchronization,    // Eastern Coalition
    TemporalStability, // ChronoLegion
    Count
};

struct FactionResourceDef
{
    FactionResourceType Type = FactionResourceType::None;
    std::string Name;
    FactionId Faction = FactionId::None;
    int32_t Min = 0;
    int32_t Max = 100;
    int32_t NaturalRegenPerTick = 0;
    int32_t LowThreshold = 0;        // penalties below this
    int32_t HighThreshold = 0;       // bonuses at/above this
    std::vector<std::string> AccrualRules;  // textual description from bible
    std::vector<std::string> SpendRules;
    std::vector<std::string> ThresholdBonuses;
};

// --- Voice ---------------------------------------------------------------

struct VoiceLineDef
{
    std::string EventTag;     // Voice.Selected, Voice.Move, etc.
    std::string TextRu;       // canonical Russian line
    std::string SoundWaveRef; // soft reference, may be empty
    int32_t Priority = 0;
    int32_t CooldownSeconds = 0;
    int32_t Weight = 1;
};

struct VoiceSetDef
{
    ContentId UnitId;
    std::string VoiceId;     // same as unit Stable ID
    std::vector<VoiceLineDef> Lines;
};

struct EvaLineDef
{
    std::string EventTag;
    std::string TextRu;
    std::string SoundWaveRef;
    int32_t Priority = 0;
    std::string Faction;
};

} // namespace RA4
