// Copyright (c) Red Alert 4 project. Simulation-side component data.
//
// Storage is structure-of-arrays indexed by EntityId::Index. Systems walk one or
// two arrays linearly rather than chasing an object graph, and adding a component
// to the game does not grow every entity. The layout is also what lets the Mass
// migration happen later without touching system logic: systems already take
// spans of components, not actors.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "RA4Content/ContentTypes.h"
#include "RA4Core/Fixed.h"
#include "RA4Core/Ids.h"
#include "RA4Core/Vector.h"
#include "RA4Navigation/MNavRouter.h"

namespace RA4
{

// --- Map ------------------------------------------------------------------

enum TileFlags : uint8_t
{
    Tile_None = 0,
    Tile_GroundPassable = 1 << 0,
    Tile_Water = 1 << 1,
    Tile_Cliff = 1 << 2,
    Tile_Occupied = 1 << 3,   // a building footprint sits here
    Tile_Resource = 1 << 4,
    Tile_Bridge = 1 << 5,
};

struct MapDescription
{
    int32_t Width = 0;
    int32_t Height = 0;
    std::vector<uint8_t> Tiles;
    std::vector<Vec2> StartPositions;
    std::string Name;

    void Resize(int32_t InWidth, int32_t InHeight, uint8_t Fill = Tile_GroundPassable)
    {
        Width = InWidth;
        Height = InHeight;
        Tiles.assign(size_t(Width) * size_t(Height), Fill);
    }

    bool IsInBounds(int32_t X, int32_t Y) const { return X >= 0 && Y >= 0 && X < Width && Y < Height; }
    size_t TileIndex(int32_t X, int32_t Y) const { return size_t(Y) * size_t(Width) + size_t(X); }

    uint8_t GetTile(int32_t X, int32_t Y) const
    {
        return IsInBounds(X, Y) ? Tiles[TileIndex(X, Y)] : uint8_t(Tile_None);
    }
    void SetTileFlag(int32_t X, int32_t Y, uint8_t Flag, bool bSet)
    {
        if (!IsInBounds(X, Y)) { return; }
        uint8_t& T = Tiles[TileIndex(X, Y)];
        T = bSet ? uint8_t(T | Flag) : uint8_t(T & ~Flag);
    }

    TileCoord WorldToTile(const Vec2& P) const
    {
        return TileCoord(int32_t(P.X.ToIntFloor() / kTileSizeUnitsLocal), int32_t(P.Y.ToIntFloor() / kTileSizeUnitsLocal));
    }
    Vec2 TileCenterToWorld(const TileCoord& T) const
    {
        return Vec2(Fixed::FromInt(int64_t(T.X) * kTileSizeUnitsLocal + kTileSizeUnitsLocal / 2),
                    Fixed::FromInt(int64_t(T.Y) * kTileSizeUnitsLocal + kTileSizeUnitsLocal / 2));
    }

    // Mirrors RA4Core/SimConfig.h kTileSizeUnits; kept local so this header does not
    // force SimConfig on every consumer. Asserted equal in SimWorld.cpp.
    static constexpr int64_t kTileSizeUnitsLocal = 200;
};

// --- Orders ---------------------------------------------------------------

enum class OrderType : uint8_t
{
    None = 0,
    Move,
    AttackMove,
    Attack,
    Harvest,
    Guard,
    // Internal steps the harvester state machine issues to itself; never produced
    // by player input, but they live in the same queue so cancellation works.
    DeliverToRefinery,
};

struct Order
{
    OrderType Type = OrderType::None;
    Vec2 Location;
    EntityId Target;
};

struct OrderQueue
{
    // Fixed capacity: an unbounded queue is a griefing vector (a client can spam
    // waypoints until the server allocates itself to death) and 16 waypoints is
    // more than any real order chain uses.
    static constexpr int32_t kCapacity = 16;
    Order Orders[kCapacity];
    int32_t Count = 0;

    void Clear() { Count = 0; }
    bool IsEmpty() const { return Count == 0; }
    const Order& Front() const { return Orders[0]; }
    Order& Front() { return Orders[0]; }

    bool Push(const Order& O)
    {
        if (Count >= kCapacity) { return false; }
        Orders[Count++] = O;
        return true;
    }
    bool PushFront(const Order& O)
    {
        if (Count >= kCapacity) { return false; }
        for (int32_t I = Count; I > 0; --I) { Orders[I] = Orders[I - 1]; }
        Orders[0] = O;
        ++Count;
        return true;
    }
    void PopFront()
    {
        if (Count == 0) { return; }
        for (int32_t I = 1; I < Count; ++I) { Orders[I - 1] = Orders[I]; }
        --Count;
    }
};

// --- Energy (ADR-0013) ----------------------------------------------------

// Power is not a stored resource but a ratio recomputed every tick, and the ratio
// alone is a poor thing to scatter comparisons against: "< 40" appearing in six
// systems is six chances to write a different bound. Naming the bands once means a
// balance change moves one table, and the UI, the AI and the tests all agree on
// where "Moderate" begins.
enum class PowerTier : uint8_t
{
    Normal = 0,    // >= 100%: no penalties
    Mild,          // 70-99%:  build and production speed scale with the ratio
    Moderate,      // 40-69%:  radar off, repair halved
    Severe,        // 10-39%:  repair off, high-tech paused, defences slowed
    Critical,      // < 10%:   only barracks and harvesters, at half speed
};

// Band boundaries, expressed as the lowest ratio still inside each band.
constexpr int32_t kPowerTierMildMinPercent = 70;
constexpr int32_t kPowerTierModerateMinPercent = 40;
constexpr int32_t kPowerTierSevereMinPercent = 10;

inline constexpr PowerTier PowerTierForRatio(int32_t RatioPercent)
{
    if (RatioPercent >= 100) { return PowerTier::Normal; }
    if (RatioPercent >= kPowerTierMildMinPercent) { return PowerTier::Mild; }
    if (RatioPercent >= kPowerTierModerateMinPercent) { return PowerTier::Moderate; }
    if (RatioPercent >= kPowerTierSevereMinPercent) { return PowerTier::Severe; }
    return PowerTier::Critical;
}

// Speed floor inside Severe. A deficit should hurt, but a base that can never
// rebuild its power plant is a dead match rather than a hard one.
constexpr int32_t kPowerSevereFloorPercent = 10;
// At Critical the few things still running do so at a flat rate rather than at the
// ratio, which by then is near zero and would mean "stopped" in all but name.
constexpr int32_t kPowerCriticalSpeedPercent = 50;

// Speed multiplier (percent) that construction and production run at in a tier.
// Critical returns the flat rate; whether a given producer is allowed to run at all
// at Critical is a separate question the caller answers, because it depends on what
// the building is.
inline constexpr int32_t PowerSpeedPercentForTier(PowerTier Tier, int32_t RatioPercent)
{
    switch (Tier)
    {
        case PowerTier::Normal:   return 100;
        case PowerTier::Mild:     return RatioPercent;
        case PowerTier::Moderate: return RatioPercent;
        case PowerTier::Severe:   return RatioPercent > kPowerSevereFloorPercent
                                             ? RatioPercent : kPowerSevereFloorPercent;
        case PowerTier::Critical: return kPowerCriticalSpeedPercent;
    }
    return 100;
}

const char* ToString(PowerTier Tier);

// ADR-0013. Which buildings a deficit reaches first. The point is that a shortage is a
// choice rather than an opaque uniform slowdown: the player decides that the radar can
// go dark so the factory keeps running, and the ordering here is what makes that
// decision expressible.
enum class PowerPriority : uint8_t
{
    Vital = 0,      // HQ, barracks, refinery -- last to degrade, never fully offline
    Production = 1, // factories and docks -- degrade at the tier thresholds
    Defense = 2,    // turrets and walls -- degrade at thresholds, offline at Critical
    Auxiliary = 3,  // radar, repair, tech, superweapon -- first to go, offline at Moderate
};

const char* ToString(PowerPriority Priority);

// --- ADR-0013 per-system deficit effects ----------------------------------

// Radar (and with it the anonymous-contact blips the recon layer derives from it) goes
// dark from Moderate. Named rather than inlined so the simulation, the HUD and the tests
// cannot disagree about when the minimap stops being trustworthy.
inline constexpr bool IsRadarOnlineAtTier(PowerTier Tier)
{
    return Tier < PowerTier::Moderate;
}

// Repair runs at full speed until Moderate, at half through Moderate, and stops from
// Severe. Returned as a percentage so the caller scales rather than branching.
constexpr int32_t kRepairModerateSpeedPercent = 50;

inline constexpr int32_t RepairSpeedPercentForTier(PowerTier Tier)
{
    switch (Tier)
    {
        case PowerTier::Normal:
        case PowerTier::Mild:     return 100;
        case PowerTier::Moderate: return kRepairModerateSpeedPercent;
        case PowerTier::Severe:
        case PowerTier::Critical: return 0;
    }
    return 100;
}

// Static defence fires at half rate through Severe (cooldowns doubled) and not at all at
// Critical. A multiplier on the cooldown rather than a speed, because that is the value
// the combat system actually holds.
constexpr int32_t kDefenceSevereCooldownMultiplier = 2;

inline constexpr int32_t StaticDefenceCooldownMultiplierForTier(PowerTier Tier)
{
    return Tier == PowerTier::Severe ? kDefenceSevereCooldownMultiplier : 1;
}

inline constexpr bool IsStaticDefenceOnlineAtTier(PowerTier Tier)
{
    return Tier != PowerTier::Critical;
}

// --- Repair (ADR-0013) ----------------------------------------------------

// Health restored per tick at full speed, and what that costs. Repair is deliberately
// slower and dearer per point than building the structure was: it is a way to save a
// damaged building, not a cheaper substitute for defending it.
constexpr int32_t kRepairHealthPerTick = 4;
// Credits per health point, in hundredths, so the price can be below one credit per
// point without floating-point maths. 25 = a quarter of a credit per hitpoint.
constexpr int32_t kRepairCostPerHealthCenti = 25;
constexpr int32_t kRepairCostScale = 100;

// The tier at which a priority band goes fully offline. Vital never does, which is what
// stops a deficit from being unrecoverable: the buildings needed to rebuild power keep
// working no matter how deep the hole.
inline constexpr bool IsPowerPriorityOffline(PowerPriority Priority, PowerTier Tier)
{
    switch (Priority)
    {
        case PowerPriority::Vital:      return false;
        case PowerPriority::Production: return Tier == PowerTier::Critical;
        case PowerPriority::Defense:    return Tier == PowerTier::Critical;
        case PowerPriority::Auxiliary:  return Tier >= PowerTier::Moderate;
    }
    return false;
}

// --- Components -----------------------------------------------------------

struct EntityCore
{
    uint32_t Generation = 0;
    ContentId Def;
    EntityKind Kind = EntityKind::Unit;
    PlayerId Owner = kInvalidPlayer;
    bool bAlive = false;
};

struct TransformComp
{
    Vec2 Position;
    int32_t Facing = 0;
    int32_t TurretFacing = 0;
};

struct HealthComp
{
    int32_t Current = 0;
    int32_t Max = 0;
    bool bInvulnerable = false;
    // Veterancy
    VeterancyRank Rank = VeterancyRank::Recruit;
    int32_t DamageDealt = 0;   // accumulated damage dealt (for veterancy tracking)
    int32_t KillsValue = 0;    // accumulated value of destroyed targets
};

struct MovementComp
{
    Vec2 Destination;
    bool bHasDestination = false;
    Fixed CurrentSpeed = Fixed::Zero();
    // Distance at which the unit considers itself arrived. Scaled by group size in
    // the navigation milestone so a hundred units do not pile onto one point.
    Fixed ArriveRadius = Fixed::FromInt(30);
    // Ticks spent unable to make progress; the navigation system uses this to give
    // up rather than grind against an obstacle forever.
    int32_t BlockedTicks = 0;
    // --- navigation milestone ---
    Nav::MacroPath CurrentMacroPath;
    int32_t NextWaypointIndex = 0;
    TileCoord CurrentSubGoal;
    ContentId FormationId;          // ContentId() == no formation
    int32_t FormationSlot = -1;     // -1 == leader or unassigned
    TickIndex LastRepathTick = 0;
};

struct MovementStats
{
    uint32_t FlowFieldBuilds = 0;
    uint32_t MacroPathBuilds = 0;
    uint32_t ReservationContests = 0;
};

struct CombatComp
{
    EntityId Target;
    int32_t CooldownTicks = 0;
    // Set when the entity is executing an explicit Attack order, so that acquiring a
    // closer target does not override what the player told it to kill.
    bool bTargetIsForced = false;
};

enum class ConstructionState : uint8_t
{
    Complete = 0,
    UnderConstruction,
};

// ADR-0012: every way a queued item can be interrupted gets its own state, so the
// UI can say *why* something stopped instead of showing an unexplained paused bar,
// and so the AI can tell "I cannot afford this" from "I have no power" from "the
// player paused it". Credits are charged incrementally across the build, not as a
// lump sum at queue time.
enum class FlowPaymentState : uint8_t
{
    Queued            = 0,  // in the queue, not yet drawing credits
    Funding           = 1,  // credits being deducted incrementally
    Paying            = 2,  // fully funded; production ticks advance
    Starved           = 3,  // funding interrupted because the treasury hit zero
    EnergyThrottled   = 4,  // funded, but a power deficit halts this category
    ManuallyPaused    = 5,  // the player paused this item
    Completed         = 6,  // finished; a building waits here for placement
    Cancelled         = 7,  // player cancelled; refund issued
    ProducerDestroyed = 8,  // the producing building died
    PrerequisiteLost  = 9,  // a required tech building died
    OwnershipChanged  = 10, // the producer was sold or captured
};

// Credits charged per tick. Ceiling division so the last tick never leaves a
// remainder unpaid: an item that reaches full progress is always fully paid for.
inline constexpr int32_t FlowPaymentCostPerTick(int32_t TotalCost, int32_t TotalTicks)
{
    return (TotalTicks > 0) ? (TotalCost + TotalTicks - 1) / TotalTicks : TotalCost;
}

struct ProductionItem
{
    ContentId Content;
    // ADR-0012 progression: Queued -> Funding -> Paying -> Completed, with
    // Starved / EnergyThrottled / ManuallyPaused as resumable detours. Progress
    // never regresses; an interrupted item freezes and later continues.
    FlowPaymentState State = FlowPaymentState::Queued;
    // Captured at queue time so a mid-build content hot-reload or a price change
    // cannot alter what the player already agreed to pay.
    int32_t TotalCost = 0;
    int32_t PaidCredits = 0;
    int32_t ProgressTicks = 0;
    int32_t TotalTicks = 0;
    int32_t Priority = 0;      // higher is funded first when credits are scarce
    // Retained so existing pause UI keeps working, but State is authoritative:
    // it distinguishes a player pause from starvation, which this flag cannot.
    bool bPaused = false;

    int32_t CostPerTick() const { return FlowPaymentCostPerTick(TotalCost, TotalTicks); }
    int32_t CreditsRemaining() const { return TotalCost > PaidCredits ? TotalCost - PaidCredits : 0; }
    bool IsFullyFunded() const { return PaidCredits >= TotalCost; }
};

struct BuildingComp
{
    TileCoord OriginTile;
    int32_t FootprintX = 1;
    int32_t FootprintY = 1;
    ConstructionState State = ConstructionState::Complete;
    int32_t ConstructionProgressTicks = 0;
    int32_t ConstructionTotalTicks = 0;

    Vec2 RallyPoint;
    bool bHasRallyPoint = false;

    std::vector<ProductionItem> Queue;

    // ADR-0013 power priority. Seeded from the definition when the building is created
    // and then owned by the player: an explicit override has to persist, so this is
    // stored per building rather than re-derived from content each tick.
    PowerPriority Priority = PowerPriority::Production;

    // Repair is a sustained, paid activity the player switches on, not a one-shot: it
    // draws credits every tick while it runs, so it needs a persistent flag rather than
    // a queued order. A power deficit slows it and eventually stops it (ADR-0013), and
    // it switches itself off once the building is whole again.
    bool bRepairing = false;
    // Fractional-credit accumulator. Repair costs less than one credit per tick at
    // sensible rates, and rounding that up every tick would make repair wildly more
    // expensive than intended; rounding it down would make it free.
    int32_t RepairCreditAccumulator = 0;

    EntityId DockedHarvester;
    std::vector<EntityId> UnloadingQueue;
};

enum class HarvesterState : uint8_t
{
    Idle = 0,
    MovingToResource,
    Harvesting,
    MovingToRefinery,
    Unloading,
};

struct HarvesterComp
{
    HarvesterState State = HarvesterState::Idle;
    int32_t Cargo = 0;
    EntityId AssignedNode;
    EntityId AssignedRefinery;
};

struct ResourceNodeComp
{
    int32_t Amount = 0;
    ContentId Def;
};

struct ProjectileComp
{
    EntityId Source;
    EntityId Target;
    Vec2 ImpactPoint;      // used when the target dies mid-flight
    ContentId Weapon;
    PlayerId OwnerPlayer = kInvalidPlayer;
    Fixed Speed = Fixed::Zero();
};

// --- Direct vehicle control ------------------------------------------------
//
// State for a vehicle currently under first-person command. Lives next to the
// other components and is authoritative: clients send DirectControlDrive/Fire
// commands; the simulation re-validates ownership, weapon cooldown and ammo,
// then advances this struct. The presentation layer reads it for the camera
// and HUD but never writes it.
enum class DirectControlPhase : uint8_t
{
    Inactive = 0,
    Entering,      // server accepted Enter; transition window before first Drive
    Active,        // receiving Drive commands
    Exiting,       // server accepted Exit; transition window back to RTS
    VehicleDestroyed,
};

struct DirectControlComp
{
    DirectControlPhase Phase = DirectControlPhase::Inactive;
    PlayerId Controller = kInvalidPlayer;
    TickIndex PhaseUntilTick = 0;   // when Entering/Exiting window ends
    int32_t TurretYawCentiDeg = 0;  // accumulated turret yaw, 1/100 deg units
    int32_t TurretPitchCentiDeg = 0;
    int32_t CooldownTicksPrimary = 0;
    int32_t CooldownTicksSecondary = 0;
    bool bOpticsZoomed = false;
};

// --- Player ---------------------------------------------------------------

struct PlayerState
{
    bool bActive = false;
    bool bDefeated = false;
    FactionId Faction = FactionId::None;
    int32_t Credits = 0;

    int32_t PowerProduced = 0;
    int32_t PowerConsumed = 0;

    int32_t CommandLimitMax = 50;
    int32_t CommandLimitUsed = 0;
    int32_t FactionResource = 0;
    FactionResourceType FactionResourceType = FactionResourceType::None;
    int32_t FactionResourceCooldown = 0;  // ticks until active ability is ready

    // Running totals for the post-match screen and for AI self-evaluation.
    int32_t TotalHarvested = 0;
    int32_t UnitsBuilt = 0;
    int32_t UnitsLost = 0;
    int32_t BuildingsBuilt = 0;
    int32_t BuildingsLost = 0;

    // Tech is derived from completed buildings each tick rather than stored, so
    // losing a war factory immediately removes access to tanks with no bookkeeping.
    std::vector<ContentId> CompletedBuildingTypes;

    // ADR-0013: the tier as of the end of last tick. Unlike the tier itself this is
    // NOT derivable -- it is the memory that makes the warning edge-triggered, so it
    // is serialized and hashed. Without it a reloaded save would either re-announce
    // a deficit the player already knows about or stay silent about one they do not.
    PowerTier LastPowerTier = PowerTier::Normal;

    int32_t GetPowerRatioPercent() const
    {
        if (PowerConsumed <= 0) { return 100; }
        if (PowerProduced >= PowerConsumed) { return 100; }
        return int32_t((int64_t(PowerProduced) * 100) / PowerConsumed);
    }

    // ADR-0013. Derived from the ratio every time it is asked for rather than stored,
    // so it cannot go stale against PowerProduced/PowerConsumed and does not need to
    // be serialized or hashed.
    PowerTier GetPowerTier() const { return PowerTierForRatio(GetPowerRatioPercent()); }
};

// --- Events ---------------------------------------------------------------

// The simulation never touches Actors, Niagara or audio. It emits events and the
// presentation layer decides what to spawn. This is the boundary that keeps the
// core runnable headless.
enum class SimEventType : uint8_t
{
    EntitySpawned = 0,
    EntityDestroyed,
    WeaponFired,
    ProjectileImpact,
    DamageApplied,
    BuildingPlaced,
    BuildingCompleted,
    ProductionStarted,
    ProductionCompleted,
    ProductionCancelled,
    // ADR-0012: emitted once when a queued item runs out of money, and not again
    // until it resumes. Under flow payment an unaffordable order is accepted rather
    // than rejected, so without this the only cue that a player is broke -- the
    // CommandRejected/InsufficientCredits path -- would simply go silent.
    ProductionStarved,
    ResourceDelivered,
    PlayerDefeated,
    MatchEnded,
    CommandRejected,
    EntityVeterancyPromoted,
    // ADR-0013: emitted when a player's power tier changes, with SimEvent::Value
    // carrying the new PowerTier. Started means the tier got worse, Ended means it
    // improved; both are edge-triggered on the crossing, so a base sitting at 45%
    // power produces one event, not one per tick.
    PowerShortageStarted,
    PowerShortageEnded,
    FactionResourceChanged,
    DirectControlEntered,
    DirectControlExited,
    DirectControlFireRejected,
};

struct SimEvent
{
    SimEventType Type = SimEventType::EntitySpawned;
    TickIndex Tick = 0;
    EntityId Entity;
    EntityId Other;
    PlayerId Player = kInvalidPlayer;
    ContentId Content;
    Vec2 Location;
    int32_t Value = 0;      // damage amount, credits, reject reason, ...
};

} // namespace RA4
