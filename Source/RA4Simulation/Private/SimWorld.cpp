// Copyright (c) Red Alert 4 project.
#include "RA4Simulation/SimWorld.h"

#include "RA4Core/Checksum.h"
#include "RA4Core/SimConfig.h"
#include "RA4Navigation/MNavRouter.h"
#include "RA4Navigation/ReservationGrid.h"

#include <algorithm>
#include <cassert>

namespace RA4
{

static_assert(MapDescription::kTileSizeUnitsLocal == kTileSizeUnits,
              "MapDescription tile size must match the simulation tile size");

namespace
{
// Progress is accumulated in hundredths of a tick so that low power can slow
// production proportionally without a floating point rate or a modulo trick.
constexpr int32_t kProgressScale = 100;

// A turret must be within this many angle units of the target before a weapon that
// requires alignment will fire (~5.6 degrees).
constexpr int32_t kTurretAlignTolerance = 64;

// Harvesters dock when this close to a node or refinery centre.
constexpr int64_t kDockDistanceUnits = 260;

// Under-powered bases still make some progress; a full stall makes a lost power
// plant unrecoverable, which is not the intent of the mechanic.
constexpr int32_t kMinPowerRatioPercent = 20;

Fixed SquaredUnits(int64_t Units) { return Fixed::FromInt(Units) * Fixed::FromInt(Units); }
} // namespace

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void SimWorld::Reset()
{
    NavigationGrid.reset();
    FlowFieldCache.clear();
    if (Reservations) Reservations->Expire(0);
    if (Router) Router->InvalidateAll();
    FlowFieldBuildsThisTick = 0;
    MacroPathBuildsThisTick = 0;
    Stats = MovementStats{};
    Core.clear();
    Transforms.clear();
    Healths.clear();
    Movements.clear();
    Combats.clear();
    Buildings.clear();
    Harvesters.clear();
    ResourceNodes.clear();
    Projectiles.clear();
    Orders.clear();
    FreeSlots.clear();
    PendingDestroy.clear();
    Events.clear();
    // Reserve the whole entity budget up front. Systems legitimately spawn entities
    // while iterating (a factory finishing a tank, a gun firing a shell), and a
    // reallocation at that moment would dangle every component reference the
    // system is holding. AllocateEntity refuses to exceed kMaxEntities, so with the
    // capacity reserved here the vectors never move.
    Core.reserve(kMaxEntities);
    Transforms.reserve(kMaxEntities);
    Healths.reserve(kMaxEntities);
    Movements.reserve(kMaxEntities);
    Combats.reserve(kMaxEntities);
    Buildings.reserve(kMaxEntities);
    Harvesters.reserve(kMaxEntities);
    ResourceNodes.reserve(kMaxEntities);
    Projectiles.reserve(kMaxEntities);
    Orders.reserve(kMaxEntities);

    HighWaterMark = 0;
    CurrentTick = 0;
    Phase = MatchPhase::NotStarted;
    Winner = kInvalidPlayer;
    for (PlayerState& P : Players)
    {
        P = PlayerState();
    }
}

void SimWorld::Initialize(const ContentDatabase* InContent, const MatchSetup& Setup)
{
    Reset();
    Content = InContent;
    Map = Setup.Map;
    BuildNavigationGrid();
    Reservations = std::make_unique<Nav::ReservationGrid>(Map.Width, Map.Height);
    Router = std::make_unique<Nav::MNavRouter>(*NavigationGrid);
    Rng.Reset(Setup.Seed);

    for (PlayerId I = 0; I < kMaxPlayers; ++I)
    {
        const MatchSetup::PlayerSlot& Slot = Setup.Players[I];
        PlayerState& P = Players[I];
        P.bActive = Slot.bActive;
        P.bDefeated = false;
        P.Faction = Slot.Faction;
        P.Credits = Slot.StartingCredits;
    }

    Phase = MatchPhase::Running;
}

// ---------------------------------------------------------------------------
// Entity storage
// ---------------------------------------------------------------------------

EntityId SimWorld::AllocateEntity()
{
    uint32_t Index;
    if (!FreeSlots.empty())
    {
        Index = FreeSlots.back();
        FreeSlots.pop_back();
    }
    else
    {
        if (Core.size() >= kMaxEntities)
        {
            return EntityId::Invalid();
        }
        Index = uint32_t(Core.size());
        Core.emplace_back();
        Transforms.emplace_back();
        Healths.emplace_back();
        Movements.emplace_back();
        Combats.emplace_back();
        Buildings.emplace_back();
        Harvesters.emplace_back();
        ResourceNodes.emplace_back();
        Projectiles.emplace_back();
        Orders.emplace_back();
        HighWaterMark = uint32_t(Core.size());
    }

    // Reset every component: a recycled slot must be indistinguishable from a fresh
    // one, or state leaks between unrelated entities and desyncs the match.
    const uint32_t Generation = Core[Index].Generation;
    Core[Index] = EntityCore();
    Core[Index].Generation = Generation;
    Core[Index].bAlive = true;
    Transforms[Index] = TransformComp();
    Healths[Index] = HealthComp();
    Movements[Index] = MovementComp();
    Combats[Index] = CombatComp();
    Buildings[Index] = BuildingComp();
    Harvesters[Index] = HarvesterComp();
    ResourceNodes[Index] = ResourceNodeComp();
    Projectiles[Index] = ProjectileComp();
    Orders[Index].Clear();

    return EntityId(Index, Generation);
}

EntityId SimWorld::MakeId(uint32_t Index) const
{
    if (Index >= Core.size())
    {
        return EntityId::Invalid();
    }
    return EntityId(Index, Core[Index].Generation);
}

bool SimWorld::IsAlive(EntityId Id) const
{
    return Id.IsValid() && Id.Index < Core.size() && Core[Id.Index].bAlive &&
           Core[Id.Index].Generation == Id.Generation;
}

const EntityCore* SimWorld::GetCore(EntityId Id) const { return IsAlive(Id) ? &Core[Id.Index] : nullptr; }
const TransformComp* SimWorld::GetTransform(EntityId Id) const { return IsAlive(Id) ? &Transforms[Id.Index] : nullptr; }
const HealthComp* SimWorld::GetHealth(EntityId Id) const { return IsAlive(Id) ? &Healths[Id.Index] : nullptr; }
const BuildingComp* SimWorld::GetBuilding(EntityId Id) const { return IsAlive(Id) ? &Buildings[Id.Index] : nullptr; }
const HarvesterComp* SimWorld::GetHarvester(EntityId Id) const { return IsAlive(Id) ? &Harvesters[Id.Index] : nullptr; }
const ResourceNodeComp* SimWorld::GetResourceNode(EntityId Id) const { return IsAlive(Id) ? &ResourceNodes[Id.Index] : nullptr; }
const MovementComp* SimWorld::GetMovement(EntityId Id) const { return IsAlive(Id) ? &Movements[Id.Index] : nullptr; }
const CombatComp* SimWorld::GetCombat(EntityId Id) const { return IsAlive(Id) ? &Combats[Id.Index] : nullptr; }
const OrderQueue* SimWorld::GetOrders(EntityId Id) const { return IsAlive(Id) ? &Orders[Id.Index] : nullptr; }

const PlayerState& SimWorld::GetPlayer(PlayerId Id) const
{
    static const PlayerState Empty;
    return Id < kMaxPlayers ? Players[Id] : Empty;
}

// ---------------------------------------------------------------------------
// Spawning
// ---------------------------------------------------------------------------

EntityId SimWorld::SpawnUnit(ContentId Def, PlayerId Owner, const Vec2& Position, int32_t Facing)
{
    const EntityDef* D = Content ? Content->FindEntity(Def) : nullptr;
    if (D == nullptr || D->Kind != EntityKind::Unit)
    {
        return EntityId::Invalid();
    }

    const EntityId Id = AllocateEntity();
    if (!Id.IsValid())
    {
        return Id;
    }

    Core[Id.Index].Def = Def;
    Core[Id.Index].Kind = EntityKind::Unit;
    Core[Id.Index].Owner = Owner;

    Transforms[Id.Index].Position = Position;
    Transforms[Id.Index].Facing = WrapAngle(Facing);
    Transforms[Id.Index].TurretFacing = WrapAngle(Facing);

    Healths[Id.Index].Max = D->MaxHealth;
    Healths[Id.Index].Current = D->MaxHealth;

    Movements[Id.Index].ArriveRadius = FxMax(D->Unit.CollisionRadius, Fixed::FromInt(25));

    if (D->Unit.bIsHarvester)
    {
        Harvesters[Id.Index].State = HarvesterState::Idle;
    }

    if (Owner < kMaxPlayers)
    {
        Players[Owner].UnitsBuilt += 1;
    }

    SimEvent Ev;
    Ev.Type = SimEventType::EntitySpawned;
    Ev.Tick = CurrentTick;
    Ev.Entity = Id;
    Ev.Player = Owner;
    Ev.Content = Def;
    Ev.Location = Position;
    EmitEvent(Ev);

    return Id;
}

EntityId SimWorld::SpawnBuilding(ContentId Def, PlayerId Owner, const TileCoord& OriginTile, bool bInstantComplete)
{
    const EntityDef* D = Content ? Content->FindEntity(Def) : nullptr;
    if (D == nullptr || D->Kind != EntityKind::Building)
    {
        return EntityId::Invalid();
    }

    const EntityId Id = AllocateEntity();
    if (!Id.IsValid())
    {
        return Id;
    }

    Core[Id.Index].Def = Def;
    Core[Id.Index].Kind = EntityKind::Building;
    Core[Id.Index].Owner = Owner;

    BuildingComp& B = Buildings[Id.Index];
    B.OriginTile = OriginTile;
    B.FootprintX = D->Building.FootprintX;
    B.FootprintY = D->Building.FootprintY;
    B.State = bInstantComplete ? ConstructionState::Complete : ConstructionState::UnderConstruction;
    B.ConstructionTotalTicks = std::max(1, D->Production.BuildTimeTicks);
    B.ConstructionProgressTicks = bInstantComplete ? B.ConstructionTotalTicks * kProgressScale : 0;

    // Position is the footprint centre so that range checks against a 3x3 factory
    // behave the same as against a 1x1 turret.
    const Vec2 Corner = Map.TileCenterToWorld(OriginTile);
    Transforms[Id.Index].Position =
        Vec2(Corner.X + Fixed::FromInt((int64_t(B.FootprintX) - 1) * kTileSizeUnits / 2),
             Corner.Y + Fixed::FromInt((int64_t(B.FootprintY) - 1) * kTileSizeUnits / 2));

    Healths[Id.Index].Max = D->MaxHealth;
    // A building under construction starts at 10% health and is finished by the
    // construction system; killing it mid-build is cheap, as it should be.
    Healths[Id.Index].Current = bInstantComplete ? D->MaxHealth : std::max(1, D->MaxHealth / 10);

    OccupyTiles(B, true);

    if (Owner < kMaxPlayers)
    {
        Players[Owner].BuildingsBuilt += 1;
    }

    SimEvent Ev;
    Ev.Type = SimEventType::BuildingPlaced;
    Ev.Tick = CurrentTick;
    Ev.Entity = Id;
    Ev.Player = Owner;
    Ev.Content = Def;
    Ev.Location = Transforms[Id.Index].Position;
    EmitEvent(Ev);

    if (bInstantComplete)
    {
        RefreshPlayerTech(Owner);
        SimEvent Done = Ev;
        Done.Type = SimEventType::BuildingCompleted;
        EmitEvent(Done);

        if (D->Building.BundledUnit.IsValid())
        {
            const Vec2 SpawnAt = FindFreeSpawnPoint(B, D->Building.BundledUnit);
            SpawnUnit(D->Building.BundledUnit, Owner, SpawnAt);
        }
    }

    return Id;
}

EntityId SimWorld::SpawnResourceNode(ContentId Def, const TileCoord& Tile, int32_t Amount)
{
    const ResourceNodeDef* D = Content ? Content->FindResourceNode(Def) : nullptr;
    if (D == nullptr)
    {
        return EntityId::Invalid();
    }

    const EntityId Id = AllocateEntity();
    if (!Id.IsValid())
    {
        return Id;
    }

    Core[Id.Index].Def = Def;
    Core[Id.Index].Kind = EntityKind::ResourceNode;
    Core[Id.Index].Owner = kNeutralPlayer;
    Transforms[Id.Index].Position = Map.TileCenterToWorld(Tile);
    ResourceNodes[Id.Index].Amount = Amount > 0 ? Amount : D->InitialAmount;
    ResourceNodes[Id.Index].Def = Def;
    Healths[Id.Index].Max = 1;
    Healths[Id.Index].Current = 1;
    Healths[Id.Index].bInvulnerable = true;

    Map.SetTileFlag(Tile.X, Tile.Y, Tile_Resource, true);

    return Id;
}

void SimWorld::OccupyTiles(const BuildingComp& B, bool bOccupy)
{
    if (NavigationGrid != nullptr)
    {
        NavigationGrid->BeginTopologyUpdate();
    }
    for (int32_t Y = 0; Y < B.FootprintY; ++Y)
    {
        for (int32_t X = 0; X < B.FootprintX; ++X)
        {
            const TileCoord Tile(B.OriginTile.X + X, B.OriginTile.Y + Y);
            Map.SetTileFlag(Tile.X, Tile.Y, Tile_Occupied, bOccupy);
            if (NavigationGrid != nullptr)
            {
                NavigationGrid->SetPassability(Tile, GetNavigationPassability(Tile));
            }
        }
    }
    if (NavigationGrid != nullptr)
    {
        NavigationGrid->EndTopologyUpdate();
    }
}

void SimWorld::BuildNavigationGrid()
{
    NavigationGrid = std::make_unique<Nav::NavGrid>(Map.Width, Map.Height);
    FlowFieldCache.clear();
    if (Router) Router->InvalidateAll();
    NavigationGrid->BeginTopologyUpdate();
    for (int32_t Y = 0; Y < Map.Height; ++Y)
    {
        for (int32_t X = 0; X < Map.Width; ++X)
        {
            const TileCoord Tile(X, Y);
            NavigationGrid->SetPassability(Tile, GetNavigationPassability(Tile));
        }
    }
    NavigationGrid->EndTopologyUpdate();
}

uint8_t SimWorld::GetNavigationPassability(const TileCoord& Tile) const
{
    if (!Map.IsInBounds(Tile.X, Tile.Y))
    {
        return Nav::NavLayer_None;
    }

    const uint8_t Flags = Map.GetTile(Tile.X, Tile.Y);
    if ((Flags & Tile_Cliff) != 0)
    {
        return Nav::NavLayer_Air;
    }
    if ((Flags & Tile_Water) != 0)
    {
        return Nav::NavLayer_Amphibious | Nav::NavLayer_Naval | Nav::NavLayer_Air;
    }
    if ((Flags & Tile_GroundPassable) == 0)
    {
        return Nav::NavLayer_Air;
    }

    uint8_t Passability = Nav::NavLayer_Infantry | Nav::NavLayer_Wheeled | Nav::NavLayer_Tracked |
                          Nav::NavLayer_Amphibious | Nav::NavLayer_Air;
    if ((Flags & Tile_Occupied) != 0)
    {
        Passability = Nav::NavLayer_Air;
    }
    return Passability;
}

Nav::NavQuery SimWorld::MakeNavigationQuery(const EntityDef& Def) const
{
    Nav::NavQuery Query;
    switch (Def.Unit.Layer)
    {
        case MovementLayer::Infantry: Query.LayerMask = Nav::NavLayer_Infantry; break;
        case MovementLayer::Wheeled: Query.LayerMask = Nav::NavLayer_Wheeled; break;
        case MovementLayer::Tracked: Query.LayerMask = Nav::NavLayer_Tracked; break;
        case MovementLayer::Amphibious: Query.LayerMask = Nav::NavLayer_Amphibious; break;
        case MovementLayer::Naval: Query.LayerMask = Nav::NavLayer_Naval; break;
        case MovementLayer::Air: Query.LayerMask = Nav::NavLayer_Air; break;
        case MovementLayer::None:
        case MovementLayer::Count: Query.LayerMask = Nav::NavLayer_None; break;
    }

    const int64_t Radius = std::max<int64_t>(0, Def.Unit.CollisionRadius.ToIntRound());
    const int64_t Diameter = Radius * 2;
    const int64_t Required = std::max<int64_t>(1, (Diameter + kTileSizeUnits - 1) / kTileSizeUnits);
    Query.RequiredClearance = static_cast<uint8_t>(std::min<int64_t>(Required, 255));
    return Query;
}

TileCoord SimWorld::ResolveNavigationTarget(const TileCoord& Desired, const Nav::NavQuery& Query) const
{
    if (NavigationGrid == nullptr || NavigationGrid->IsTraversable(Desired, Query))
    {
        return Desired;
    }

    const int32_t MaxRadius = std::max(Map.Width, Map.Height);
    for (int32_t Radius = 1; Radius <= MaxRadius; ++Radius)
    {
        const int32_t MinX = Desired.X - Radius;
        const int32_t MaxX = Desired.X + Radius;
        const int32_t MinY = Desired.Y - Radius;
        const int32_t MaxY = Desired.Y + Radius;
        for (int32_t Y = MinY; Y <= MaxY; ++Y)
        {
            for (int32_t X = MinX; X <= MaxX; ++X)
            {
                if (X != MinX && X != MaxX && Y != MinY && Y != MaxY)
                {
                    continue;
                }
                const TileCoord Candidate(X, Y);
                if (NavigationGrid->IsTraversable(Candidate, Query))
                {
                    return Candidate;
                }
            }
        }
    }
    return Desired;
}

const Nav::FlowField* SimWorld::GetFlowField(const TileCoord& Target, const Nav::NavQuery& Query)
{
    for (FlowFieldCacheEntry& Entry : FlowFieldCache)
    {
        if (Entry.Target == Target &&
            Entry.Query.LayerMask == Query.LayerMask &&
            Entry.Query.RequiredClearance == Query.RequiredClearance &&
            Entry.TopologyRevision == NavigationGrid->GetTopologyRevision())
        {
            Entry.LastUsedTick = CurrentTick;
            return Entry.Field.get();
        }
    }
    // Budget: only build if we haven't built too many this tick.
    if (FlowFieldBuildsThisTick >= kMaxFlowFieldBuildsPerTick)
    {
        // Return the most-recent field in the cache as a best-effort stale guide.
        // If the cache is empty, the caller will treat null as "blocked" and retry
        // next tick -- bounded latency, no crash, deterministic.
        if (!FlowFieldCache.empty())
        {
            return FlowFieldCache.back().Field.get();
        }
        return nullptr;
    }
    constexpr size_t kMaxCachedFlowFields = 64;
    if (FlowFieldCache.size() >= kMaxCachedFlowFields)
    {
        size_t EvictionIndex = 0;
        for (size_t I = 1; I < FlowFieldCache.size(); ++I)
        {
            const FlowFieldCacheEntry& Candidate = FlowFieldCache[I];
            const FlowFieldCacheEntry& Best = FlowFieldCache[EvictionIndex];
            if (Candidate.LastUsedTick < Best.LastUsedTick) EvictionIndex = I;
        }
        FlowFieldCache.erase(FlowFieldCache.begin() + static_cast<std::ptrdiff_t>(EvictionIndex));
    }
    FlowFieldCacheEntry Entry;
    Entry.Target = Target;
    Entry.Query = Query;
    Entry.TopologyRevision = NavigationGrid->GetTopologyRevision();
    Entry.LastUsedTick = CurrentTick;
    Entry.Field = std::make_unique<Nav::FlowField>(*NavigationGrid, Query, Target);
    Entry.Field->Rebuild();
    FlowFieldCache.push_back(std::move(Entry));
    ++FlowFieldBuildsThisTick;
    ++Stats.FlowFieldBuilds;
    return FlowFieldCache.back().Field.get();
}

Vec2 SimWorld::FindFreeSpawnPoint(const BuildingComp& Producer, ContentId /*UnitDef*/) const
{
    // Walk the ring of tiles around the footprint and take the first passable one.
    // Deterministic because the scan order is fixed; the navigation milestone will
    // replace this with a proper factory exit lane.
    for (int32_t Ring = 1; Ring <= 4; ++Ring)
    {
        const int32_t MinX = Producer.OriginTile.X - Ring;
        const int32_t MaxX = Producer.OriginTile.X + Producer.FootprintX - 1 + Ring;
        const int32_t MinY = Producer.OriginTile.Y - Ring;
        const int32_t MaxY = Producer.OriginTile.Y + Producer.FootprintY - 1 + Ring;

        for (int32_t Y = MinY; Y <= MaxY; ++Y)
        {
            for (int32_t X = MinX; X <= MaxX; ++X)
            {
                const bool bOnRing = (X == MinX || X == MaxX || Y == MinY || Y == MaxY);
                if (!bOnRing)
                {
                    continue;
                }
                const uint8_t T = Map.GetTile(X, Y);
                if ((T & Tile_GroundPassable) != 0 && (T & Tile_Occupied) == 0)
                {
                    return Map.TileCenterToWorld(TileCoord(X, Y));
                }
            }
        }
    }

    return Map.TileCenterToWorld(Producer.OriginTile);
}

// ---------------------------------------------------------------------------
// Tech and ownership helpers
// ---------------------------------------------------------------------------

void SimWorld::RefreshPlayerTech(PlayerId Owner)
{
    if (Owner >= kMaxPlayers)
    {
        return;
    }
    std::vector<ContentId>& Types = Players[Owner].CompletedBuildingTypes;
    Types.clear();
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Building || Core[I].Owner != Owner)
        {
            continue;
        }
        if (Buildings[I].State != ConstructionState::Complete)
        {
            continue;
        }
        if (std::find(Types.begin(), Types.end(), Core[I].Def) == Types.end())
        {
            Types.push_back(Core[I].Def);
        }
    }
    // Sorted so the checksum does not depend on entity slot order.
    std::sort(Types.begin(), Types.end());
}

bool SimWorld::HasPrerequisites(PlayerId Owner, const EntityDef& Def) const
{
    if (Owner >= kMaxPlayers)
    {
        return false;
    }
    const std::vector<ContentId>& Have = Players[Owner].CompletedBuildingTypes;
    for (const ContentId& Req : Def.Production.Prerequisites)
    {
        if (std::find(Have.begin(), Have.end(), Req) == Have.end())
        {
            return false;
        }
    }
    return true;
}

bool SimWorld::IsHostile(PlayerId A, PlayerId B) const
{
    if (A == kInvalidPlayer || B == kInvalidPlayer) { return false; }
    if (A == B) { return false; }
    if (A == kNeutralPlayer || B == kNeutralPlayer) { return false; }
    // Team support lands with the lobby; every non-neutral player is hostile today.
    return true;
}

bool SimWorld::IsPlacementValid(ContentId BuildingDef, PlayerId Owner, const TileCoord& OriginTile) const
{
    const EntityDef* D = Content ? Content->FindEntity(BuildingDef) : nullptr;
    if (D == nullptr || D->Kind != EntityKind::Building)
    {
        return false;
    }

    for (int32_t Y = 0; Y < D->Building.FootprintY; ++Y)
    {
        for (int32_t X = 0; X < D->Building.FootprintX; ++X)
        {
            const int32_t TX = OriginTile.X + X;
            const int32_t TY = OriginTile.Y + Y;
            if (!Map.IsInBounds(TX, TY))
            {
                return false;
            }
            const uint8_t T = Map.GetTile(TX, TY);
            if ((T & Tile_GroundPassable) == 0) { return false; }
            if ((T & (Tile_Water | Tile_Cliff | Tile_Occupied | Tile_Resource)) != 0) { return false; }
        }
    }

    // Must sit inside the build radius of a completed structure that projects one.
    const Vec2 Centre = Map.TileCenterToWorld(OriginTile);
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Building || Core[I].Owner != Owner)
        {
            continue;
        }
        if (Buildings[I].State != ConstructionState::Complete)
        {
            continue;
        }
        const EntityDef* OtherDef = Content->FindEntity(Core[I].Def);
        if (OtherDef == nullptr || !OtherDef->Building.bProvidesBuildRadius)
        {
            continue;
        }
        const Fixed R = OtherDef->Building.BuildRadius;
        if (DistanceSquared(Centre, Transforms[I].Position) <= R * R)
        {
            return true;
        }
    }

    return false;
}

// ---------------------------------------------------------------------------
// Commands
// ---------------------------------------------------------------------------

CommandResult SimWorld::ApplyCommand(const Command& Cmd)
{
    CommandResult Result;

    auto Reject = [&](CommandReject Reason)
    {
        Result.Reason = Reason;
        SimEvent Ev;
        Ev.Type = SimEventType::CommandRejected;
        Ev.Tick = CurrentTick;
        Ev.Player = Cmd.Issuer;
        Ev.Entity = Cmd.Primary;
        Ev.Content = Cmd.Content;
        Ev.Value = int32_t(Reason);
        EmitEvent(Ev);
        return Result;
    };

    if (Phase != MatchPhase::Running)
    {
        return Reject(CommandReject::MatchOver);
    }
    if (Cmd.Issuer >= kMaxPlayers || !Players[Cmd.Issuer].bActive || Players[Cmd.Issuer].bDefeated)
    {
        return Reject(CommandReject::NotOwner);
    }
    if (CommandsThisTick[Cmd.Issuer] >= kMaxCommandsPerPlayerPerTick)
    {
        return Reject(CommandReject::RateLimited);
    }
    CommandsThisTick[Cmd.Issuer] += 1;

    PlayerState& Player = Players[Cmd.Issuer];

    switch (Cmd.Type)
    {
        case CommandType::Move:
        case CommandType::AttackMove:
        case CommandType::Attack:
        case CommandType::Stop:
        case CommandType::Harvest:
        case CommandType::Guard:
        {
            if (!IsAlive(Cmd.Primary))
            {
                return Reject(CommandReject::NoSuchEntity);
            }
            if (Core[Cmd.Primary.Index].Owner != Cmd.Issuer)
            {
                return Reject(CommandReject::NotOwner);
            }
            if (Core[Cmd.Primary.Index].Kind != EntityKind::Unit)
            {
                return Reject(CommandReject::NoSuchEntity);
            }

            OrderQueue& Q = Orders[Cmd.Primary.Index];
            if (Cmd.Type == CommandType::Stop)
            {
                Q.Clear();
                Movements[Cmd.Primary.Index].bHasDestination = false;
                Movements[Cmd.Primary.Index].CurrentSpeed = Fixed::Zero();
                Combats[Cmd.Primary.Index].Target = EntityId::Invalid();
                Combats[Cmd.Primary.Index].bTargetIsForced = false;
                Harvesters[Cmd.Primary.Index].State = HarvesterState::Idle;
                break;
            }

            Order O;
            switch (Cmd.Type)
            {
                case CommandType::Move: O.Type = OrderType::Move; break;
                case CommandType::AttackMove: O.Type = OrderType::AttackMove; break;
                case CommandType::Attack: O.Type = OrderType::Attack; break;
                case CommandType::Harvest: O.Type = OrderType::Harvest; break;
                default: O.Type = OrderType::Guard; break;
            }
            O.Location = Cmd.Location;
            O.Target = Cmd.Target;

            if (O.Type == OrderType::Attack)
            {
                if (!IsAlive(Cmd.Target))
                {
                    return Reject(CommandReject::TargetInvalid);
                }
                if (!IsHostile(Cmd.Issuer, Core[Cmd.Target.Index].Owner))
                {
                    return Reject(CommandReject::TargetInvalid);
                }
            }

            if (Cmd.Mode == OrderMode::Replace)
            {
                Q.Clear();
                Combats[Cmd.Primary.Index].bTargetIsForced = false;
            }
            if (!Q.Push(O))
            {
                return Reject(CommandReject::QueueFull);
            }
            break;
        }

        case CommandType::SetRallyPoint:
        {
            if (!IsAlive(Cmd.Primary) || Core[Cmd.Primary.Index].Kind != EntityKind::Building)
            {
                return Reject(CommandReject::NoSuchEntity);
            }
            if (Core[Cmd.Primary.Index].Owner != Cmd.Issuer)
            {
                return Reject(CommandReject::NotOwner);
            }
            Buildings[Cmd.Primary.Index].RallyPoint = Cmd.Location;
            Buildings[Cmd.Primary.Index].bHasRallyPoint = true;
            break;
        }

        case CommandType::StartProduction:
        {
            const EntityDef* Item = Content ? Content->FindEntity(Cmd.Content) : nullptr;
            if (Item == nullptr)
            {
                return Reject(CommandReject::UnknownContent);
            }
            if (!HasPrerequisites(Cmd.Issuer, *Item))
            {
                return Reject(CommandReject::TechRequirementsUnmet);
            }

            // Locate the producer: either the building the player clicked, or the
            // first owned completed building of an allowed producer type.
            EntityId ProducerId = EntityId::Invalid();
            if (IsAlive(Cmd.Primary) && Core[Cmd.Primary.Index].Owner == Cmd.Issuer &&
                Core[Cmd.Primary.Index].Kind == EntityKind::Building &&
                Buildings[Cmd.Primary.Index].State == ConstructionState::Complete)
            {
                const ContentId PrimaryDef = Core[Cmd.Primary.Index].Def;
                if (std::find(Item->Production.ProducedBy.begin(), Item->Production.ProducedBy.end(), PrimaryDef) !=
                    Item->Production.ProducedBy.end())
                {
                    ProducerId = Cmd.Primary;
                }
            }
            if (!ProducerId.IsValid())
            {
                for (uint32_t I = 0; I < Core.size() && !ProducerId.IsValid(); ++I)
                {
                    if (!Core[I].bAlive || Core[I].Kind != EntityKind::Building || Core[I].Owner != Cmd.Issuer)
                    {
                        continue;
                    }
                    if (Buildings[I].State != ConstructionState::Complete)
                    {
                        continue;
                    }
                    if (std::find(Item->Production.ProducedBy.begin(), Item->Production.ProducedBy.end(),
                                  Core[I].Def) != Item->Production.ProducedBy.end())
                    {
                        ProducerId = MakeId(I);
                    }
                }
            }
            if (!ProducerId.IsValid())
            {
                return Reject(CommandReject::NoProducer);
            }

            BuildingComp& Producer = Buildings[ProducerId.Index];
            if (int32_t(Producer.Queue.size()) >= kMaxProductionQueueLength)
            {
                return Reject(CommandReject::QueueFull);
            }
            if (Player.Credits < Item->Production.Cost)
            {
                return Reject(CommandReject::InsufficientCredits);
            }

            Player.Credits -= Item->Production.Cost;

            ProductionItem QueueItem;
            QueueItem.Content = Cmd.Content;
            QueueItem.TotalTicks = std::max(1, Item->Production.BuildTimeTicks);
            QueueItem.ProgressTicks = 0;
            QueueItem.PaidCredits = Item->Production.Cost;
            Producer.Queue.push_back(QueueItem);

            SimEvent Ev;
            Ev.Type = SimEventType::ProductionStarted;
            Ev.Tick = CurrentTick;
            Ev.Entity = ProducerId;
            Ev.Player = Cmd.Issuer;
            Ev.Content = Cmd.Content;
            EmitEvent(Ev);
            break;
        }

        case CommandType::CancelProduction:
        {
            if (!IsAlive(Cmd.Primary) || Core[Cmd.Primary.Index].Owner != Cmd.Issuer ||
                Core[Cmd.Primary.Index].Kind != EntityKind::Building)
            {
                return Reject(CommandReject::NoSuchEntity);
            }
            BuildingComp& Producer = Buildings[Cmd.Primary.Index];
            const int32_t Slot = int32_t(Cmd.Slot);
            if (Slot < 0 || Slot >= int32_t(Producer.Queue.size()))
            {
                return Reject(CommandReject::QueueFull);
            }
            const ProductionItem& QueueItem = Producer.Queue[size_t(Slot)];
            const EntityDef* Item = Content->FindEntity(QueueItem.Content);
            const int32_t RefundPercent = Item ? Item->Production.CancelRefundPercent : 100;
            Player.Credits += (QueueItem.PaidCredits * RefundPercent) / 100;

            SimEvent Ev;
            Ev.Type = SimEventType::ProductionCancelled;
            Ev.Tick = CurrentTick;
            Ev.Entity = Cmd.Primary;
            Ev.Player = Cmd.Issuer;
            Ev.Content = QueueItem.Content;
            Ev.Value = (QueueItem.PaidCredits * RefundPercent) / 100;
            EmitEvent(Ev);

            Producer.Queue.erase(Producer.Queue.begin() + Slot);
            break;
        }

        case CommandType::PauseProduction:
        {
            if (!IsAlive(Cmd.Primary) || Core[Cmd.Primary.Index].Owner != Cmd.Issuer ||
                Core[Cmd.Primary.Index].Kind != EntityKind::Building)
            {
                return Reject(CommandReject::NoSuchEntity);
            }
            BuildingComp& Producer = Buildings[Cmd.Primary.Index];
            if (Cmd.Slot < Producer.Queue.size())
            {
                ProductionItem& QueueItem = Producer.Queue[Cmd.Slot];
                QueueItem.bPaused = !QueueItem.bPaused;
            }
            break;
        }

        case CommandType::PlaceBuilding:
        {
            const EntityDef* Item = Content ? Content->FindEntity(Cmd.Content) : nullptr;
            if (Item == nullptr || Item->Kind != EntityKind::Building)
            {
                return Reject(CommandReject::UnknownContent);
            }

            // The structure must already be finished in a construction yard queue.
            // This is the C&C model: you pay and build first, then choose where it
            // lands, and the server owns both halves of that transaction.
            EntityId YardId = EntityId::Invalid();
            int32_t QueueSlot = -1;
            for (uint32_t I = 0; I < Core.size() && QueueSlot < 0; ++I)
            {
                if (!Core[I].bAlive || Core[I].Kind != EntityKind::Building || Core[I].Owner != Cmd.Issuer)
                {
                    continue;
                }
                BuildingComp& Yard = Buildings[I];
                for (size_t S = 0; S < Yard.Queue.size(); ++S)
                {
                    if (Yard.Queue[S].Content == Cmd.Content &&
                        Yard.Queue[S].ProgressTicks >= Yard.Queue[S].TotalTicks * kProgressScale)
                    {
                        YardId = MakeId(I);
                        QueueSlot = int32_t(S);
                        break;
                    }
                }
            }
            if (QueueSlot < 0)
            {
                return Reject(CommandReject::NoProducer);
            }
            if (!IsPlacementValid(Cmd.Content, Cmd.Issuer, Cmd.Tile))
            {
                return Reject(CommandReject::InvalidPlacement);
            }

            Buildings[YardId.Index].Queue.erase(Buildings[YardId.Index].Queue.begin() + QueueSlot);
            SpawnBuilding(Cmd.Content, Cmd.Issuer, Cmd.Tile, /*bInstantComplete*/ false);
            break;
        }

        case CommandType::SellBuilding:
        {
            if (!IsAlive(Cmd.Primary) || Core[Cmd.Primary.Index].Owner != Cmd.Issuer ||
                Core[Cmd.Primary.Index].Kind != EntityKind::Building)
            {
                return Reject(CommandReject::NoSuchEntity);
            }
            const EntityDef* D = Content->FindEntity(Core[Cmd.Primary.Index].Def);
            if (D != nullptr)
            {
                Player.Credits += (D->Production.Cost * D->Building.SellRefundPercent) / 100;
            }
            PendingDestroy.push_back(Cmd.Primary);
            break;
        }

        case CommandType::RepairBuilding:
        {
            if (!IsAlive(Cmd.Primary) || Core[Cmd.Primary.Index].Owner != Cmd.Issuer)
            {
                return Reject(CommandReject::NoSuchEntity);
            }
            // Repair over time lands with the economy milestone; the command is
            // accepted and validated now so the protocol does not change later.
            break;
        }

        case CommandType::Surrender:
        {
            Player.bDefeated = true;
            SimEvent Ev;
            Ev.Type = SimEventType::PlayerDefeated;
            Ev.Tick = CurrentTick;
            Ev.Player = Cmd.Issuer;
            EmitEvent(Ev);
            break;
        }

        default:
            return Reject(CommandReject::UnknownType);
    }

    return Result;
}

// ---------------------------------------------------------------------------
// Damage
// ---------------------------------------------------------------------------

void SimWorld::ApplyDamage(EntityId TargetId, int32_t BaseDamage, WarheadClass Warhead, EntityId Source,
                           PlayerId SourcePlayer)
{
    if (!IsAlive(TargetId) || Healths[TargetId.Index].bInvulnerable)
    {
        return;
    }
    const EntityDef* D = Content->FindEntity(Core[TargetId.Index].Def);
    if (D == nullptr)
    {
        return;
    }

    const int32_t Multiplier = Content->GetDamageMultiplier(Warhead, D->Armor);
    int32_t Damage = (BaseDamage * Multiplier) / 100;
    // A weapon that is not useless against a target should always do at least one
    // point, otherwise rounding produces unkillable stalemates at low damage.
    if (Damage <= 0 && Multiplier > 0 && BaseDamage > 0)
    {
        Damage = 1;
    }
    if (Damage <= 0)
    {
        return;
    }

    HealthComp& H = Healths[TargetId.Index];
    H.Current -= Damage;

    SimEvent Ev;
    Ev.Type = SimEventType::DamageApplied;
    Ev.Tick = CurrentTick;
    Ev.Entity = TargetId;
    Ev.Other = Source;
    Ev.Player = SourcePlayer;
    Ev.Location = Transforms[TargetId.Index].Position;
    Ev.Value = Damage;
    EmitEvent(Ev);

    if (H.Current <= 0)
    {
        H.Current = 0;
        // Deferred: the killer may still be iterating and the target's components
        // must stay readable for the rest of this tick.
        if (std::find(PendingDestroy.begin(), PendingDestroy.end(), TargetId) == PendingDestroy.end())
        {
            PendingDestroy.push_back(TargetId);
        }
    }
}

void SimWorld::ApplySplashDamage(const Vec2& Center, Fixed Radius, int32_t BaseDamage, WarheadClass Warhead,
                                 int32_t FalloffPercent, EntityId Source, PlayerId SourcePlayer)
{
    if (Radius <= Fixed::Zero())
    {
        return;
    }
    const Fixed RadiusSq = Radius * Radius;
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind == EntityKind::Projectile || Core[I].Kind == EntityKind::ResourceNode)
        {
            continue;
        }
        const Fixed DistSq = DistanceSquared(Center, Transforms[I].Position);
        if (DistSq > RadiusSq)
        {
            continue;
        }
        // Linear falloff from full damage at the centre to FalloffPercent at the rim.
        const Fixed Dist = FxSqrt(DistSq);
        const int64_t Ratio = Radius.Raw == 0 ? 0 : (Dist.Raw * 100) / Radius.Raw;
        const int32_t Scale = 100 - int32_t((int64_t(100 - FalloffPercent) * Ratio) / 100);
        // Splash damages everything in the blast, including the attacker's own
        // units. Friendly fire is a design decision, not an oversight.
        ApplyDamage(MakeId(I), (BaseDamage * Scale) / 100, Warhead, Source, SourcePlayer);
    }
}

void SimWorld::DestroyEntity(EntityId Id, EntityId Killer)
{
    if (!IsAlive(Id))
    {
        return;
    }
    const uint32_t Index = Id.Index;
    const PlayerId Owner = Core[Index].Owner;
    const EntityKind Kind = Core[Index].Kind;

    if (Kind == EntityKind::Building)
    {
        OccupyTiles(Buildings[Index], false);
        // Queued items are lost with the factory. Refunding them would make losing a
        // production building consequence-free.
        Buildings[Index].Queue.clear();
        if (Owner < kMaxPlayers)
        {
            Players[Owner].BuildingsLost += 1;
        }
    }
    else if (Kind == EntityKind::Unit)
    {
        if (Owner < kMaxPlayers)
        {
            Players[Owner].UnitsLost += 1;
        }
    }

    SimEvent Ev;
    Ev.Type = SimEventType::EntityDestroyed;
    Ev.Tick = CurrentTick;
    Ev.Entity = Id;
    Ev.Other = Killer;
    Ev.Player = Owner;
    Ev.Content = Core[Index].Def;
    Ev.Location = Transforms[Index].Position;
    EmitEvent(Ev);

    Core[Index].bAlive = false;
    Core[Index].Generation += 1;
    FreeSlots.push_back(Index);

    if (Kind == EntityKind::Building && Owner < kMaxPlayers)
    {
        RefreshPlayerTech(Owner);
    }
}

// ---------------------------------------------------------------------------
// Systems
// ---------------------------------------------------------------------------

void SimWorld::SystemApplyCommands(const CommandFrame* Frame)
{
    for (int32_t& Count : CommandsThisTick)
    {
        Count = 0;
    }
    if (Frame == nullptr)
    {
        return;
    }
    for (const Command& Cmd : Frame->Commands)
    {
        ApplyCommand(Cmd);
    }
}

void SimWorld::SystemPower()
{
    for (PlayerState& P : Players)
    {
        P.PowerProduced = 0;
        P.PowerConsumed = 0;
    }
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Building || Core[I].Owner >= kMaxPlayers)
        {
            continue;
        }
        if (Buildings[I].State != ConstructionState::Complete)
        {
            continue;
        }
        const EntityDef* D = Content->FindEntity(Core[I].Def);
        if (D == nullptr)
        {
            continue;
        }
        PlayerState& P = Players[Core[I].Owner];
        // A damaged power plant produces proportionally less. This is what makes
        // hitting the power grid a real strategy rather than a binary switch.
        const int64_t HealthRatio = Healths[I].Max > 0 ? (int64_t(Healths[I].Current) * 100) / Healths[I].Max : 0;
        P.PowerProduced += int32_t((int64_t(D->Building.PowerProduced) * HealthRatio) / 100);
        P.PowerConsumed += D->Building.PowerConsumed;
    }
}

void SimWorld::SystemConstruction()
{
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Building)
        {
            continue;
        }
        BuildingComp& B = Buildings[I];
        if (B.State != ConstructionState::UnderConstruction)
        {
            continue;
        }

        const PlayerId Owner = Core[I].Owner;
        const int32_t Ratio = Owner < kMaxPlayers
                                  ? std::max(kMinPowerRatioPercent, Players[Owner].GetPowerRatioPercent())
                                  : 100;

        const EntityDef* D = Content->FindEntity(Core[I].Def);
        if (D == nullptr)
        {
            continue;
        }

        const int64_t Total = std::max<int64_t>(1, int64_t(B.ConstructionTotalTicks) * kProgressScale);
        const int64_t PrevProgress = std::min<int64_t>(B.ConstructionProgressTicks, Total);
        B.ConstructionProgressTicks += Ratio;
        const int64_t NewProgress = std::min<int64_t>(B.ConstructionProgressTicks, Total);

        // Health accrues with construction progress, so a half-built structure is
        // half as tough and the presentation layer can drive a build-up effect from
        // the health ratio. Adding the delta rather than assigning the absolute
        // value means damage taken mid-build is not silently repaired next tick.
        const int64_t Gain = (int64_t(Healths[I].Max) * NewProgress) / Total -
                             (int64_t(Healths[I].Max) * PrevProgress) / Total;
        Healths[I].Current = int32_t(std::min<int64_t>(Healths[I].Max, Healths[I].Current + Gain));

        if (B.ConstructionProgressTicks >= B.ConstructionTotalTicks * kProgressScale)
        {
            B.State = ConstructionState::Complete;
            RefreshPlayerTech(Owner);

            SimEvent Ev;
            Ev.Type = SimEventType::BuildingCompleted;
            Ev.Tick = CurrentTick;
            Ev.Entity = MakeId(I);
            Ev.Player = Owner;
            Ev.Content = Core[I].Def;
            Ev.Location = Transforms[I].Position;
            EmitEvent(Ev);

            if (D->Building.BundledUnit.IsValid())
            {
                const Vec2 SpawnAt = FindFreeSpawnPoint(B, D->Building.BundledUnit);
                SpawnUnit(D->Building.BundledUnit, Owner, SpawnAt);
            }
        }
    }
}

void SimWorld::SystemProduction()
{
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Building)
        {
            continue;
        }
        BuildingComp& B = Buildings[I];
        if (B.State != ConstructionState::Complete || B.Queue.empty())
        {
            continue;
        }

        const PlayerId Owner = Core[I].Owner;
        const int32_t Ratio = Owner < kMaxPlayers
                                  ? std::max(kMinPowerRatioPercent, Players[Owner].GetPowerRatioPercent())
                                  : 100;

        // Only the head of the queue advances; parallel queues are per building,
        // matching the original games.
        ProductionItem& QueueItem = B.Queue.front();
        if (QueueItem.bPaused)
        {
            continue;
        }
        const int32_t Complete = QueueItem.TotalTicks * kProgressScale;
        if (QueueItem.ProgressTicks >= Complete)
        {
            // Structures wait here until the player picks a spot; everything else
            // pops out immediately.
            const EntityDef* Item = Content->FindEntity(QueueItem.Content);
            if (Item != nullptr && Item->Kind == EntityKind::Building)
            {
                continue;
            }
        }
        else
        {
            QueueItem.ProgressTicks += Ratio;
            if (QueueItem.ProgressTicks < Complete)
            {
                continue;
            }
        }

        const EntityDef* Item = Content->FindEntity(QueueItem.Content);
        if (Item == nullptr)
        {
            B.Queue.erase(B.Queue.begin());
            continue;
        }
        if (Item->Kind == EntityKind::Building)
        {
            continue;   // awaiting placement
        }

        const Vec2 SpawnAt = FindFreeSpawnPoint(B, QueueItem.Content);
        const EntityId Spawned = SpawnUnit(QueueItem.Content, Owner, SpawnAt);
        if (!Spawned.IsValid())
        {
            continue;   // entity budget exhausted; retry next tick
        }

        if (B.bHasRallyPoint)
        {
            Order O;
            O.Type = OrderType::Move;
            O.Location = B.RallyPoint;
            Orders[Spawned.Index].Push(O);
        }

        SimEvent Ev;
        Ev.Type = SimEventType::ProductionCompleted;
        Ev.Tick = CurrentTick;
        Ev.Entity = Spawned;
        Ev.Other = MakeId(I);
        Ev.Player = Owner;
        Ev.Content = QueueItem.Content;
        Ev.Location = SpawnAt;
        EmitEvent(Ev);

        B.Queue.erase(B.Queue.begin());
    }
}

EntityId SimWorld::FindNearestResourceNode(const Vec2& From, PlayerId /*Owner*/) const
{
    EntityId Best = EntityId::Invalid();
    Fixed BestDistSq = Fixed::Max();
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::ResourceNode || ResourceNodes[I].Amount <= 0)
        {
            continue;
        }
        const Fixed DistSq = DistanceSquared(From, Transforms[I].Position);
        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            Best = MakeId(I);
        }
    }
    return Best;
}

EntityId SimWorld::FindNearestRefinery(const Vec2& From, PlayerId Owner) const
{
    EntityId Best = EntityId::Invalid();
    Fixed BestDistSq = Fixed::Max();
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Building || Core[I].Owner != Owner)
        {
            continue;
        }
        if (Buildings[I].State != ConstructionState::Complete)
        {
            continue;
        }
        const EntityDef* D = Content->FindEntity(Core[I].Def);
        if (D == nullptr || !D->Building.bIsRefinery)
        {
            continue;
        }
        const Fixed DistSq = DistanceSquared(From, Transforms[I].Position);
        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            Best = MakeId(I);
        }
    }
    return Best;
}

void SimWorld::SystemHarvesters()
{
    const Fixed DockDistSq = SquaredUnits(kDockDistanceUnits);

    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Unit)
        {
            continue;
        }
        const EntityDef* D = Content->FindEntity(Core[I].Def);
        if (D == nullptr || !D->Unit.bIsHarvester)
        {
            continue;
        }

        HarvesterComp& H = Harvesters[I];
        const PlayerId Owner = Core[I].Owner;
        const Vec2 Pos = Transforms[I].Position;

        // An explicit Harvest order overrides whatever the automation chose.
        if (!Orders[I].IsEmpty() && Orders[I].Front().Type == OrderType::Harvest)
        {
            const Order& O = Orders[I].Front();
            if (IsAlive(O.Target) && Core[O.Target.Index].Kind == EntityKind::ResourceNode)
            {
                H.AssignedNode = O.Target;
                H.State = HarvesterState::MovingToResource;
            }
            Orders[I].PopFront();
        }
        // A manual move order suspends harvesting until it finishes.
        else if (!Orders[I].IsEmpty())
        {
            continue;
        }

        switch (H.State)
        {
            case HarvesterState::Idle:
            {
                if (H.Cargo > 0)
                {
                    H.State = HarvesterState::MovingToRefinery;
                    break;
                }
                const EntityId Node = FindNearestResourceNode(Pos, Owner);
                if (Node.IsValid())
                {
                    H.AssignedNode = Node;
                    H.State = HarvesterState::MovingToResource;
                }
                break;
            }

            case HarvesterState::MovingToResource:
            {
                if (!IsAlive(H.AssignedNode) || ResourceNodes[H.AssignedNode.Index].Amount <= 0)
                {
                    H.AssignedNode = FindNearestResourceNode(Pos, Owner);
                    if (!H.AssignedNode.IsValid())
                    {
                        H.State = HarvesterState::Idle;
                        Movements[I].bHasDestination = false;
                        break;
                    }
                }
                const Vec2 NodePos = Transforms[H.AssignedNode.Index].Position;
                if (DistanceSquared(Pos, NodePos) <= DockDistSq)
                {
                    H.State = HarvesterState::Harvesting;
                    Movements[I].bHasDestination = false;
                    Movements[I].CurrentSpeed = Fixed::Zero();
                }
                else
                {
                    Movements[I].Destination = NodePos;
                    Movements[I].bHasDestination = true;
                }
                break;
            }

            case HarvesterState::Harvesting:
            {
                if (!IsAlive(H.AssignedNode))
                {
                    H.State = H.Cargo > 0 ? HarvesterState::MovingToRefinery : HarvesterState::Idle;
                    break;
                }
                ResourceNodeComp& Node = ResourceNodes[H.AssignedNode.Index];
                const int32_t Space = D->Unit.CargoCapacity - H.Cargo;
                const int32_t Take = std::min({D->Unit.HarvestPerTick, Space, Node.Amount});
                Node.Amount -= Take;
                H.Cargo += Take;

                if (Node.Amount <= 0)
                {
                    // Exhausted fields stop being targets; the tile flag drives the
                    // minimap and the AI's expansion logic.
                    const TileCoord T = Map.WorldToTile(Transforms[H.AssignedNode.Index].Position);
                    Map.SetTileFlag(T.X, T.Y, Tile_Resource, false);
                    PendingDestroy.push_back(H.AssignedNode);
                }
                if (H.Cargo >= D->Unit.CargoCapacity || Node.Amount <= 0)
                {
                    H.State = HarvesterState::MovingToRefinery;
                }
                break;
            }

            case HarvesterState::MovingToRefinery:
            {
                if (!IsAlive(H.AssignedRefinery))
                {
                    H.AssignedRefinery = FindNearestRefinery(Pos, Owner);
                }
                if (!H.AssignedRefinery.IsValid())
                {
                    // Nowhere to unload: hold position rather than wander, so that
                    // rebuilding a refinery resumes the loop immediately.
                    Movements[I].bHasDestination = false;
                    break;
                }
                const Vec2 RefPos = Transforms[H.AssignedRefinery.Index].Position;
                if (DistanceSquared(Pos, RefPos) <= DockDistSq)
                {
                    H.State = HarvesterState::Unloading;
                    Movements[I].bHasDestination = false;
                    Movements[I].CurrentSpeed = Fixed::Zero();
                }
                else
                {
                    Movements[I].Destination = RefPos;
                    Movements[I].bHasDestination = true;
                }
                break;
            }

            case HarvesterState::Unloading:
            {
                if (!IsAlive(H.AssignedRefinery))
                {
                    H.State = HarvesterState::MovingToRefinery;
                    break;
                }
                const int32_t Amount = std::min(D->Unit.UnloadPerTick, H.Cargo);
                H.Cargo -= Amount;

                const ResourceNodeDef* NodeDef = Content->FindResourceNode(MakeContentId("resource.ore_field"));
                const int32_t Value = NodeDef ? NodeDef->ValuePerUnit : 1;
                if (Owner < kMaxPlayers)
                {
                    Players[Owner].Credits += Amount * Value;
                    Players[Owner].TotalHarvested += Amount * Value;
                }

                if (Amount > 0)
                {
                    SimEvent Ev;
                    Ev.Type = SimEventType::ResourceDelivered;
                    Ev.Tick = CurrentTick;
                    Ev.Entity = MakeId(I);
                    Ev.Other = H.AssignedRefinery;
                    Ev.Player = Owner;
                    Ev.Value = Amount * Value;
                    EmitEvent(Ev);
                }

                if (H.Cargo <= 0)
                {
                    H.State = HarvesterState::MovingToResource;
                }
                break;
            }
        }
    }
}

void SimWorld::SystemOrders()
{
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Unit)
        {
            continue;
        }
        OrderQueue& Q = Orders[I];
        MovementComp& M = Movements[I];
        CombatComp& C = Combats[I];

        if (Q.IsEmpty())
        {
            C.bTargetIsForced = false;
            continue;
        }

        Order& O = Q.Front();
        switch (O.Type)
        {
            case OrderType::Move:
            {
                M.Destination = O.Location;
                M.bHasDestination = true;
                if (DistanceSquared(Transforms[I].Position, O.Location) <= M.ArriveRadius * M.ArriveRadius)
                {
                    M.bHasDestination = false;
                    M.CurrentSpeed = Fixed::Zero();
                    Q.PopFront();
                }
                // Give up rather than grind forever against an obstacle. The
                // navigation milestone replaces this with a repath request.
                else if (M.BlockedTicks > kTicksPerSecond * 3)
                {
                    M.BlockedTicks = 0;
                    M.bHasDestination = false;
                    Q.PopFront();
                }
                break;
            }

            case OrderType::AttackMove:
            {
                const EntityId Acquired = AcquireTarget(MakeId(I));
                if (Acquired.IsValid())
                {
                    C.Target = Acquired;
                    C.bTargetIsForced = false;
                    // Stop and shoot; resume the advance once the area is clear.
                    M.bHasDestination = false;
                    M.CurrentSpeed = Fixed::Zero();
                }
                else
                {
                    M.Destination = O.Location;
                    M.bHasDestination = true;
                    if (DistanceSquared(Transforms[I].Position, O.Location) <= M.ArriveRadius * M.ArriveRadius)
                    {
                        M.bHasDestination = false;
                        Q.PopFront();
                    }
                }
                break;
            }

            case OrderType::Attack:
            {
                if (!IsAlive(O.Target))
                {
                    C.Target = EntityId::Invalid();
                    C.bTargetIsForced = false;
                    M.bHasDestination = false;
                    Q.PopFront();
                    break;
                }
                C.Target = O.Target;
                C.bTargetIsForced = true;

                const EntityDef* D = Content->FindEntity(Core[I].Def);
                const WeaponDef* W = D && D->Weapon.IsValid() ? Content->FindWeapon(D->Weapon) : nullptr;
                if (W == nullptr)
                {
                    Q.PopFront();
                    break;
                }
                const Vec2 TargetPos = Transforms[O.Target.Index].Position;
                const Fixed DistSq = DistanceSquared(Transforms[I].Position, TargetPos);
                // Close to 90% of maximum range before stopping, so that a target
                // edging away does not make the whole group oscillate.
                const Fixed Engage = (W->MaxRange * 9) / 10;
                if (DistSq > Engage * Engage)
                {
                    M.Destination = TargetPos;
                    M.bHasDestination = true;
                }
                else
                {
                    M.bHasDestination = false;
                    M.CurrentSpeed = Fixed::Zero();
                }
                break;
            }

            case OrderType::Guard:
            {
                M.bHasDestination = false;
                const EntityId Acquired = AcquireTarget(MakeId(I));
                C.Target = Acquired;
                C.bTargetIsForced = false;
                break;
            }

            case OrderType::Harvest:
            case OrderType::DeliverToRefinery:
            case OrderType::None:
                Q.PopFront();
                break;
        }
    }
}

void SimWorld::SystemMovement()
{
    if (Reservations) Reservations->Expire(CurrentTick);
    FlowFieldBuildsThisTick = 0;
    MacroPathBuildsThisTick = 0;

    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Unit) continue;
        MovementComp& M = Movements[I];
        TransformComp& T = Transforms[I];
        const EntityDef* D = Content->FindEntity(Core[I].Def);
        if (D == nullptr) continue;

        if (!M.bHasDestination)
        {
            M.CurrentSpeed = Fixed::Zero();
            M.BlockedTicks = 0;
            if (Reservations) Reservations->Release(I);
            continue;
        }

        // 1. Arrived? (existing check, unchanged)
        const Vec2 GoalDelta = M.Destination - T.Position;
        const Fixed GoalDistSq = GoalDelta.LengthSquared();
        if (GoalDistSq <= M.ArriveRadius * M.ArriveRadius)
        {
            M.bHasDestination = false;
            M.CurrentSpeed = Fixed::Zero();
            M.BlockedTicks = 0;
            if (Reservations) Reservations->Release(I);
            continue;
        }

        const Nav::NavQuery Query = MakeNavigationQuery(*D);
        if (NavigationGrid == nullptr || Query.LayerMask == Nav::NavLayer_None)
        {
            continue;   // no navigation for this unit (e.g. air, handled later)
        }

        const TileCoord FromTile = Map.WorldToTile(T.Position);
        const TileCoord ToTile = ResolveNavigationTarget(Map.WorldToTile(M.Destination), Query);

        // 2. Macro path (budgeted, shared, topology-aware).
        if (M.CurrentMacroPath.BuiltTopologyRevision != NavigationGrid->GetTopologyRevision() ||
            M.CurrentMacroPath.Waypoints.empty())
        {
            if (MacroPathBuildsThisTick < kMaxMacroPathBuildsPerTick && Router)
            {
                M.CurrentMacroPath = Router->Find(FromTile, ToTile, Query, /*MaxWaypoints=*/8);
                M.NextWaypointIndex = 0;
                ++MacroPathBuildsThisTick;
                ++Stats.MacroPathBuilds;
            }
            else
            {
                // Budget exhausted this tick: follow the last-known flow (if any)
                // and retry next tick. Fall through to steering with stale sub-goal.
            }
        }

        // 3. Sub-goal: next sector-center waypoint.
        if (!M.CurrentMacroPath.Waypoints.empty() &&
            M.NextWaypointIndex < int32_t(M.CurrentMacroPath.Waypoints.size()))
        {
            M.CurrentSubGoal = M.CurrentMacroPath.Waypoints[size_t(M.NextWaypointIndex)];
            const int32_t Sx = FromTile.X / Nav::NavGrid::kSectorSize;
            const int32_t Sy = FromTile.Y / Nav::NavGrid::kSectorSize;
            const int32_t Gsx = M.CurrentSubGoal.X / Nav::NavGrid::kSectorSize;
            const int32_t Gsy = M.CurrentSubGoal.Y / Nav::NavGrid::kSectorSize;
            if (Sx == Gsx && Sy == Gsy)
            {
                ++M.NextWaypointIndex;
            }
        }
        else if (M.CurrentMacroPath.Waypoints.empty())
        {
            // No macro path yet (unreachable or budget-stalled). Count blocked.
            M.BlockedTicks += 1;
            if (M.BlockedTicks > kRepathBlockedTickThreshold)
            {
                M.CurrentMacroPath = Nav::MacroPath{};
                M.BlockedTicks = 0;
                M.LastRepathTick = CurrentTick;
            }
            M.CurrentSpeed = Fixed::Zero();
            continue;
        }

        // 4. Flow field for the sub-goal (shared across all units heading there).
        const Nav::FlowField* Field = GetFlowField(M.CurrentSubGoal, Query);
        if (Field == nullptr || !Field->IsReachable(FromTile))
        {
            M.BlockedTicks += 1;
            M.CurrentSpeed = Fixed::Zero();
            continue;
        }

        // 5. Steering: flow direction -> desired tile.
        const Nav::FlowDirection Dir = Field->GetDirection(FromTile);
        if (Dir.X == 0 && Dir.Y == 0)
        {
            M.BlockedTicks += 1;
            M.CurrentSpeed = Fixed::Zero();
            continue;
        }
        const TileCoord DesiredTile(FromTile.X + Dir.X, FromTile.Y + Dir.Y);

        // 6. Reservation (soft, slot-order tie-break).
        TileCoord NextTile = DesiredTile;
        if (Reservations && Reservations->TryReserve(DesiredTile, I, CurrentTick, /*HoldTicks=*/2))
        {
            M.BlockedTicks = 0;
        }
        else
        {
            ++Stats.ReservationContests;
            // 6b. Local avoidance: best open neighbor by flow-direction alignment.
            // Fixed neighbor order: N,E,S,W,NE,SE,SW,NW (matches FlowField GDirections).
            static const int8_t Deltas[8][2] = {
                {0, -1}, {1, 0}, {0, 1}, {-1, 0}, {1, -1}, {1, 1}, {-1, 1}, {-1, -1},
            };
            TileCoord Best;
            Fixed BestScore = Fixed::FromInt(-1) * Fixed::FromInt(1000000);
            bool bFound = false;
            for (int32_t N = 0; N < 8; ++N)
            {
                const TileCoord C(DesiredTile.X + Deltas[N][0], DesiredTile.Y + Deltas[N][1]);
                if (!NavigationGrid->IsTraversable(C, Query)) continue;
                if (Reservations && !Reservations->IsFree(C, CurrentTick)) continue;
                // Score = dot of flow dir with (C - FromTile) direction.
                const Vec2 Dn(Fixed::FromInt(C.X - FromTile.X), Fixed::FromInt(C.Y - FromTile.Y));
                const Fixed Score = Fixed::FromInt(Dir.X) * Dn.X + Fixed::FromInt(Dir.Y) * Dn.Y;
                if (!bFound || Score > BestScore)
                {
                    Best = C; BestScore = Score; bFound = true;
                }
            }
            if (bFound && Reservations && Reservations->TryReserve(Best, I, CurrentTick, 2))
            {
                NextTile = Best;
                M.BlockedTicks = 0;
            }
            else
            {
                M.CurrentSpeed = Fixed::Zero();
                M.BlockedTicks += 1;
                continue;
            }
        }

        // 7. STEER toward TileCenter(NextTile). (Existing turn/accel logic preserved.)
        const Vec2 SteeringDestination = Map.TileCenterToWorld(NextTile);
        const Vec2 SteeringDelta = SteeringDestination - T.Position;
        const int32_t DesiredFacing = SteeringDelta.ToAngle();
        const int32_t TurnPerTick = std::max(1, D->Unit.TurnRatePerSecond / kTicksPerSecond);
        const int32_t Diff = AngleDelta(T.Facing, DesiredFacing);
        if (Diff > TurnPerTick) { T.Facing = WrapAngle(T.Facing + TurnPerTick); }
        else if (Diff < -TurnPerTick) { T.Facing = WrapAngle(T.Facing - TurnPerTick); }
        else { T.Facing = DesiredFacing; }

        const Fixed MaxSpeedPerTick = PerSecondToPerTick(D->Unit.MaxSpeed);
        const Fixed AccelPerTick = PerSecondToPerTick(D->Unit.Acceleration);
        const int32_t AlignedThreshold = kAngleTurn / 8;
        if (Diff > -AlignedThreshold && Diff < AlignedThreshold)
        {
            M.CurrentSpeed = FxMin(M.CurrentSpeed + AccelPerTick, MaxSpeedPerTick);
        }
        else
        {
            M.CurrentSpeed = FxMax(M.CurrentSpeed - AccelPerTick, MaxSpeedPerTick / int64_t(4));
        }

        const Vec2 Step = Vec2::FromAngle(T.Facing) * M.CurrentSpeed;
        const Vec2 NextPos = T.Position + Step;
        const TileCoord NextPosTile = Map.WorldToTile(NextPos);
        const bool bPassable = NavigationGrid->IsTraversable(NextPosTile, Query);
        if (bPassable)
        {
            T.Position = NextPos;
            M.BlockedTicks = 0;
        }
        else
        {
            M.CurrentSpeed = Fixed::Zero();
            M.BlockedTicks += 1;
        }

        // 8. Blocked fallback: force a fresh macro path next tick.
        if (M.BlockedTicks > kRepathBlockedTickThreshold)
        {
            M.CurrentMacroPath = Nav::MacroPath{};
            M.BlockedTicks = 0;
            M.LastRepathTick = CurrentTick;
        }
    }
}

EntityId SimWorld::AcquireTarget(EntityId Attacker) const
{
    if (!IsAlive(Attacker))
    {
        return EntityId::Invalid();
    }
    const uint32_t A = Attacker.Index;
    const EntityDef* D = Content->FindEntity(Core[A].Def);
    if (D == nullptr || !D->Weapon.IsValid())
    {
        return EntityId::Invalid();
    }
    const WeaponDef* W = Content->FindWeapon(D->Weapon);
    if (W == nullptr)
    {
        return EntityId::Invalid();
    }

    const Vec2 Pos = Transforms[A].Position;
    // Units engage anything inside their vision, not just inside weapon range, so
    // that a spotted enemy is chased rather than ignored one metre out of reach.
    const Fixed SearchRange = FxMax(D->VisionRange, W->MaxRange);
    const Fixed SearchSq = SearchRange * SearchRange;

    EntityId Best = EntityId::Invalid();
    Fixed BestDistSq = Fixed::Max();
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || I == A)
        {
            continue;
        }
        if (Core[I].Kind == EntityKind::Projectile || Core[I].Kind == EntityKind::ResourceNode)
        {
            continue;
        }
        if (!IsHostile(Core[A].Owner, Core[I].Owner))
        {
            continue;
        }
        const EntityDef* TargetDef = Content->FindEntity(Core[I].Def);
        if (TargetDef == nullptr)
        {
            continue;
        }
        const bool bTargetIsAir = TargetDef->Unit.Layer == MovementLayer::Air;
        if (bTargetIsAir && !W->bCanTargetAir) { continue; }
        if (!bTargetIsAir && !W->bCanTargetGround) { continue; }

        const Fixed DistSq = DistanceSquared(Pos, Transforms[I].Position);
        if (DistSq <= SearchSq && DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            Best = MakeId(I);
        }
    }
    return Best;
}

void SimWorld::FireWeapon(EntityId Attacker, EntityId TargetId, const WeaponDef& Weapon)
{
    const uint32_t A = Attacker.Index;
    const Vec2 From = Transforms[A].Position;
    const Vec2 To = Transforms[TargetId.Index].Position;

    SimEvent Ev;
    Ev.Type = SimEventType::WeaponFired;
    Ev.Tick = CurrentTick;
    Ev.Entity = Attacker;
    Ev.Other = TargetId;
    Ev.Player = Core[A].Owner;
    Ev.Content = Weapon.Id;
    Ev.Location = From;
    EmitEvent(Ev);

    Combats[A].CooldownTicks = Weapon.CooldownTicks;

    if (Weapon.ProjectileSpeed <= Fixed::Zero())
    {
        // Hitscan: resolve immediately.
        ApplyDamage(TargetId, Weapon.Damage, Weapon.Warhead, Attacker, Core[A].Owner);
        if (Weapon.SplashRadius > Fixed::Zero())
        {
            ApplySplashDamage(To, Weapon.SplashRadius, Weapon.Damage, Weapon.Warhead,
                              Weapon.SplashFalloffPercent, Attacker, Core[A].Owner);
        }
        return;
    }

    const EntityId ProjId = AllocateEntity();
    if (!ProjId.IsValid())
    {
        return;
    }
    Core[ProjId.Index].Kind = EntityKind::Projectile;
    Core[ProjId.Index].Owner = Core[A].Owner;
    Core[ProjId.Index].Def = Weapon.Id;
    Transforms[ProjId.Index].Position = From;
    Transforms[ProjId.Index].Facing = (To - From).ToAngle();
    Healths[ProjId.Index].bInvulnerable = true;
    Healths[ProjId.Index].Max = 1;
    Healths[ProjId.Index].Current = 1;

    ProjectileComp& P = Projectiles[ProjId.Index];
    P.Source = Attacker;
    P.Target = TargetId;
    P.Weapon = Weapon.Id;
    P.OwnerPlayer = Core[A].Owner;
    P.Speed = PerSecondToPerTick(Weapon.ProjectileSpeed);

    // Scatter is applied once at launch and scales with range: long shots are less
    // accurate, which is what makes artillery a suppression weapon rather than a
    // sniper rifle.
    P.ImpactPoint = To;
    if (Weapon.ScatterAtMaxRange > Fixed::Zero())
    {
        const Fixed Dist = Distance(From, To);
        const Fixed Scatter = Weapon.MaxRange.Raw > 0
                                  ? Fixed((Weapon.ScatterAtMaxRange.Raw * Dist.Raw) / Weapon.MaxRange.Raw)
                                  : Fixed::Zero();
        if (Scatter > Fixed::Zero())
        {
            const int32_t Angle = int32_t(Rng.NextBelow(uint32_t(kAngleTurn)));
            const Fixed Radius = Fixed((int64_t(Rng.NextUInt32() & 0xFFFF) * Scatter.Raw) >> 16);
            P.ImpactPoint = To + Vec2::FromAngle(Angle) * Radius;
        }
    }
}

void SimWorld::SystemCombat()
{
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive)
        {
            continue;
        }
        const EntityKind Kind = Core[I].Kind;
        if (Kind != EntityKind::Unit && Kind != EntityKind::Building)
        {
            continue;
        }

        CombatComp& C = Combats[I];
        if (C.CooldownTicks > 0)
        {
            C.CooldownTicks -= 1;
        }

        const EntityDef* D = Content->FindEntity(Core[I].Def);
        if (D == nullptr || !D->Weapon.IsValid())
        {
            continue;
        }
        if (Kind == EntityKind::Building && Buildings[I].State != ConstructionState::Complete)
        {
            continue;   // a turret under construction does not shoot
        }
        const WeaponDef* W = Content->FindWeapon(D->Weapon);
        if (W == nullptr)
        {
            continue;
        }

        // Defences and idle units pick their own targets; ordered units keep theirs.
        if (!IsAlive(C.Target))
        {
            C.Target = EntityId::Invalid();
            C.bTargetIsForced = false;
        }
        if (!C.Target.IsValid())
        {
            C.Target = AcquireTarget(MakeId(I));
        }
        if (!C.Target.IsValid())
        {
            continue;
        }

        const Vec2 Pos = Transforms[I].Position;
        const Vec2 TargetPos = Transforms[C.Target.Index].Position;
        const Fixed DistSq = DistanceSquared(Pos, TargetPos);

        if (DistSq > W->MaxRange * W->MaxRange)
        {
            continue;
        }
        if (W->MinRange > Fixed::Zero() && DistSq < W->MinRange * W->MinRange)
        {
            continue;   // inside the artillery dead zone
        }

        // Turret tracking is independent of hull facing.
        const int32_t DesiredAngle = (TargetPos - Pos).ToAngle();
        if (D->Unit.TurretTurnRatePerSecond > 0)
        {
            const int32_t TurnPerTick = std::max(1, D->Unit.TurretTurnRatePerSecond / kTicksPerSecond);
            const int32_t Diff = AngleDelta(Transforms[I].TurretFacing, DesiredAngle);
            if (Diff > TurnPerTick) { Transforms[I].TurretFacing = WrapAngle(Transforms[I].TurretFacing + TurnPerTick); }
            else if (Diff < -TurnPerTick) { Transforms[I].TurretFacing = WrapAngle(Transforms[I].TurretFacing - TurnPerTick); }
            else { Transforms[I].TurretFacing = DesiredAngle; }
        }
        else
        {
            Transforms[I].TurretFacing = DesiredAngle;
        }

        if (W->bRequiresTurretAligned)
        {
            const int32_t Misalignment = AngleDelta(Transforms[I].TurretFacing, DesiredAngle);
            if (Misalignment > kTurretAlignTolerance || Misalignment < -kTurretAlignTolerance)
            {
                continue;
            }
        }

        if (C.CooldownTicks <= 0)
        {
            FireWeapon(MakeId(I), C.Target, *W);
        }
    }
}

void SimWorld::SystemProjectiles()
{
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Projectile)
        {
            continue;
        }
        ProjectileComp& P = Projectiles[I];
        const WeaponDef* W = Content->FindWeapon(P.Weapon);
        if (W == nullptr)
        {
            PendingDestroy.push_back(MakeId(I));
            continue;
        }

        // Guided munitions track a living target; if it dies the shot still lands
        // where the target was, which is what makes dodging meaningful.
        if (IsAlive(P.Target))
        {
            P.ImpactPoint = Transforms[P.Target.Index].Position;
        }

        const Vec2 Pos = Transforms[I].Position;
        const Vec2 Delta = P.ImpactPoint - Pos;
        const Fixed Dist = Delta.Length();

        if (Dist <= P.Speed || Dist <= Fixed::FromInt(10))
        {
            Transforms[I].Position = P.ImpactPoint;

            SimEvent Ev;
            Ev.Type = SimEventType::ProjectileImpact;
            Ev.Tick = CurrentTick;
            Ev.Entity = MakeId(I);
            Ev.Other = P.Target;
            Ev.Player = P.OwnerPlayer;
            Ev.Content = P.Weapon;
            Ev.Location = P.ImpactPoint;
            EmitEvent(Ev);

            if (IsAlive(P.Target))
            {
                ApplyDamage(P.Target, W->Damage, W->Warhead, P.Source, P.OwnerPlayer);
            }
            if (W->SplashRadius > Fixed::Zero())
            {
                ApplySplashDamage(P.ImpactPoint, W->SplashRadius, W->Damage, W->Warhead,
                                  W->SplashFalloffPercent, P.Source, P.OwnerPlayer);
            }
            PendingDestroy.push_back(MakeId(I));
        }
        else
        {
            const Vec2 Dir = Delta / Dist;
            Transforms[I].Position = Pos + Dir * P.Speed;
            Transforms[I].Facing = Dir.ToAngle();
        }
    }
}

void SimWorld::SystemDeaths()
{
    // PendingDestroy can contain duplicates (splash plus a direct hit in the same
    // tick); DestroyEntity is a no-op on an already dead handle because the
    // generation no longer matches.
    for (const EntityId& Id : PendingDestroy)
    {
        DestroyEntity(Id, EntityId::Invalid());
    }
    PendingDestroy.clear();
}

void SimWorld::SystemVictory()
{
    if (Phase != MatchPhase::Running)
    {
        return;
    }

    int32_t AliveCounts[kMaxPlayers] = {};
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Owner >= kMaxPlayers)
        {
            continue;
        }
        if (Core[I].Kind == EntityKind::Unit || Core[I].Kind == EntityKind::Building)
        {
            AliveCounts[Core[I].Owner] += 1;
        }
    }

    int32_t Remaining = 0;
    PlayerId LastStanding = kInvalidPlayer;
    for (PlayerId I = 0; I < kMaxPlayers; ++I)
    {
        PlayerState& P = Players[I];
        if (!P.bActive || P.bDefeated)
        {
            continue;
        }
        if (AliveCounts[I] == 0)
        {
            P.bDefeated = true;
            SimEvent Ev;
            Ev.Type = SimEventType::PlayerDefeated;
            Ev.Tick = CurrentTick;
            Ev.Player = I;
            EmitEvent(Ev);
            continue;
        }
        Remaining += 1;
        LastStanding = I;
    }

    if (Remaining <= 1)
    {
        Phase = MatchPhase::Finished;
        Winner = Remaining == 1 ? LastStanding : kInvalidPlayer;

        SimEvent Ev;
        Ev.Type = SimEventType::MatchEnded;
        Ev.Tick = CurrentTick;
        Ev.Player = Winner;
        EmitEvent(Ev);
    }
}

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void SimWorld::Tick(const CommandFrame* Frame)
{
    if (Phase != MatchPhase::Running)
    {
        return;
    }

    // Fixed system order. Changing it changes simulation results, so it is part of
    // the replay compatibility contract and is versioned in ReplayFormat.
    SystemApplyCommands(Frame);
    SystemPower();
    SystemConstruction();
    SystemProduction();
    SystemHarvesters();
    SystemOrders();
    SystemMovement();
    SystemCombat();
    SystemProjectiles();
    SystemDeaths();
    SystemVictory();

    CurrentTick += 1;
}

// ---------------------------------------------------------------------------
// Checksum
// ---------------------------------------------------------------------------

uint64_t SimWorld::ComputeStateChecksum() const
{
    Hash64 H;
    H.FeedUInt32(CurrentTick);
    H.FeedUInt8(uint8_t(Phase));
    H.FeedUInt8(Winner);
    H.FeedUInt64(Rng.GetState());

    for (PlayerId I = 0; I < kMaxPlayers; ++I)
    {
        const PlayerState& P = Players[I];
        H.FeedBool(P.bActive);
        H.FeedBool(P.bDefeated);
        H.FeedInt32(P.Credits);
        H.FeedInt32(P.PowerProduced);
        H.FeedInt32(P.PowerConsumed);
        H.FeedInt32(P.TotalHarvested);
        for (const ContentId& C : P.CompletedBuildingTypes)
        {
            H.FeedUInt32(C.Value);
        }
    }

    // Slot order is deterministic because allocation and recycling are
    // deterministic, so hashing by index needs no sort.
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        H.FeedBool(Core[I].bAlive);
        if (!Core[I].bAlive)
        {
            continue;
        }
        H.FeedUInt32(I);
        H.FeedUInt32(Core[I].Generation);
        H.FeedUInt32(Core[I].Def.Value);
        H.FeedUInt8(uint8_t(Core[I].Kind));
        H.FeedUInt8(Core[I].Owner);
        H.FeedInt64(Transforms[I].Position.X.Raw);
        H.FeedInt64(Transforms[I].Position.Y.Raw);
        H.FeedInt32(Transforms[I].Facing);
        H.FeedInt32(Transforms[I].TurretFacing);
        H.FeedInt32(Healths[I].Current);
        H.FeedInt32(Combats[I].CooldownTicks);
        H.FeedUInt64(Combats[I].Target.Packed());
        H.FeedInt64(Movements[I].CurrentSpeed.Raw);
        H.FeedBool(Movements[I].bHasDestination);
        H.FeedInt64(Movements[I].Destination.X.Raw);
        H.FeedInt64(Movements[I].Destination.Y.Raw);
        H.FeedInt32(Orders[I].Count);

        if (Core[I].Kind == EntityKind::Building)
        {
            H.FeedUInt8(uint8_t(Buildings[I].State));
            H.FeedInt32(Buildings[I].ConstructionProgressTicks);
            H.FeedInt32(int32_t(Buildings[I].Queue.size()));
            for (const ProductionItem& QueueItem : Buildings[I].Queue)
            {
                H.FeedUInt32(QueueItem.Content.Value);
                H.FeedInt32(QueueItem.ProgressTicks);
                H.FeedBool(QueueItem.bPaused);
            }
        }
        else if (Core[I].Kind == EntityKind::Unit)
        {
            H.FeedUInt8(uint8_t(Harvesters[I].State));
            H.FeedInt32(Harvesters[I].Cargo);
        }
        else if (Core[I].Kind == EntityKind::ResourceNode)
        {
            H.FeedInt32(ResourceNodes[I].Amount);
        }
    }

    return H.Get();
}

} // namespace RA4
