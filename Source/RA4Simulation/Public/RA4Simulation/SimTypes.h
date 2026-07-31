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

struct ProductionItem
{
    ContentId Content;
    int32_t ProgressTicks = 0;
    int32_t TotalTicks = 0;
    int32_t PaidCredits = 0;
    bool bPaused = false;
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
    bool bSelling = false;

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

    int32_t GetPowerRatioPercent() const
    {
        if (PowerConsumed <= 0) { return 100; }
        if (PowerProduced >= PowerConsumed) { return 100; }
        return int32_t((int64_t(PowerProduced) * 100) / PowerConsumed);
    }
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
    ResourceDelivered,
    PlayerDefeated,
    MatchEnded,
    CommandRejected,
    EntityVeterancyPromoted,
    PowerShortageStarted,
    PowerShortageEnded,
    FactionResourceChanged,
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
