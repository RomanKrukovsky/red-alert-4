// Copyright (c) Red Alert 4 project.
#include "RA4Simulation/SimWorld.h"

#include "RA4Core/Checksum.h"
#include "RA4Core/SimConfig.h"

#include "RA4Recon/DistortionPipeline.h"
#include "FogOfWarGrid.h"
#include "RA4Navigation/Formation.h"
#include "RA4Navigation/MNavRouter.h"
#include "RA4Navigation/ReservationGrid.h"

#include <algorithm>
#include <cassert>

namespace RA4
{

namespace
{
// Transition window (Entering/Exiting) before direct control is fully active.
constexpr TickIndex kDirectControlEnterExitTicks = 4; // 200ms at 20Hz
}

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

// How close a harvester must get to the *edge* of its target to dock.
constexpr int64_t kDockDistanceUnits = 260;

// Under-powered bases still make some progress; a full stall makes a lost power
// plant unrecoverable, which is not the intent of the mechanic.
constexpr int32_t kMinPowerRatioPercent = 20;

// Docking is measured to the target's centre, but a building occupies tiles that no
// unit can enter. A harvester pressed against the wall of a 3x2 refinery is already
// ~400 units from its centre, so a flat 260-unit radius made docking geometrically
// impossible: the harvester ground against the building, repathed every 60 ticks and
// never unloaded. The radius therefore grows with the target's footprint.
Fixed DockRadiusFor(const EntityDef* Def)
{
    Fixed Radius = Fixed::FromInt(kDockDistanceUnits);
    if (Def != nullptr && Def->Kind == EntityKind::Building)
    {
        const int64_t SpanTiles = std::max(Def->Building.FootprintX, Def->Building.FootprintY);
        Radius += Fixed::FromInt((SpanTiles * kTileSizeUnits) / 2);
    }
    return Radius;
}

// Widens an arrive radius by the size of the group heading for the same point
// (SimTypes.h:172: "Scaled by group size ... so a hundred units do not pile onto one
// point"). SpawnUnit set ArriveRadius once from CollisionRadius and nothing ever
// scaled it.
//
// Growth is sub-linear: the radius bounds an AREA, so widening by the square root of
// the crowd keeps roughly a constant amount of room per unit. Linear growth would let
// a 200-unit order declare arrival most of a screen from the target. The result is
// clamped so a very large group cannot swallow a whole sector.
//
// Declared up here because it is used by BOTH SystemOrders and SystemMovement, and
// that sharing is load-bearing rather than tidiness: SystemOrders decides when a Move
// order is POPPED and SystemMovement decides when the unit STOPS. If the two used
// different radii, a unit would stop moving at the wide radius while its order was
// never popped at the narrow one, and SystemOrders -- which runs immediately before
// SystemMovement and re-sets bHasDestination from the queue every tick -- would keep
// it moving forever. That is a permanent stall with a full order queue.
Fixed ScaleArriveRadiusForGroup(Fixed BaseRadius, int64_t GroupSize)
{
    if (GroupSize <= 1)
    {
        return BaseRadius;
    }
    const Fixed Scaled = BaseRadius * FxSqrt(Fixed::FromInt(GroupSize));
    return FxMin(Scaled, Fixed::FromInt(MapDescription::kTileSizeUnitsLocal * 3));
}

// Packs a tile into a sortable key. The bias keeps negative coordinates from
// colliding with positive ones.
uint64_t PackDestTileKey(const TileCoord& Tile)
{
    return (uint64_t(uint32_t(Tile.X + (1 << 20))) << 32) | uint64_t(uint32_t(Tile.Y + (1 << 20)));
}

// Counts living units whose destination tile matches DestTile, from a pre-sorted key
// list. Returns at least 1 so a caller never scales by zero.
//
// A sorted vector plus binary search rather than a hash map: sorting integers is
// deterministic, whereas a hash container would bring bucket layout and iteration
// order -- both implementation-defined -- into authoritative simulation state.
int64_t GroupSizeAtDestTile(const std::vector<uint64_t>& SortedKeys, const TileCoord& DestTile)
{
    const uint64_t Key = PackDestTileKey(DestTile);
    const auto Lo = std::lower_bound(SortedKeys.begin(), SortedKeys.end(), Key);
    const auto Hi = std::upper_bound(SortedKeys.begin(), SortedKeys.end(), Key);
    const int64_t Count = int64_t(Hi - Lo);
    return Count > 0 ? Count : 1;
}
} // namespace

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void SimWorld::Reset()
{
    NavigationGrid.reset();
    FogGrid.reset();
    FlowFieldCache.clear();

    if (Reservations) Reservations->Expire(0);
    if (Router) Router->InvalidateAll();
    FlowFieldBuildsThisTick = 0;
    MacroPathBuildsThisTick = 0;
    Stats = MovementStats{};
    ReconLayer.Reset();
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
    DirectControls.clear();
    Morales.clear();
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
    DirectControls.reserve(kMaxEntities);

    HighWaterMark = 0;
    CurrentTick = 0;
    Phase = MatchPhase::NotStarted;
    Winner = kInvalidPlayer;
    for (PlayerState& P : Players)
    {
        P = PlayerState();
    }
}

void SimWorld::Initialize(const ContentDatabase* InContent, const MatchSetup& Setup,
                          const Recon::ReconSettings* InReconSettings)
{
    Reset();
    Content = InContent;
    SetupConfig = Setup;
    Map = Setup.Map;
    BuildNavigationGrid();
    FogGrid = std::make_unique<FFogOfWarGrid>(Map.Width, Map.Height, kMaxPlayers);
    Reservations = std::make_unique<Nav::ReservationGrid>(Map.Width, Map.Height);

    Router = std::make_unique<Nav::MNavRouter>(*NavigationGrid);
    Rng.Reset(Setup.Seed);
    // Distinct sequence constant gives the intel layer an independent stream from
    // the same match seed (see SimWorld.h for why isolation matters).
    ReconRng.Reset(Setup.Seed, 0x496e74656cULL /* "Recon" */);
    ReconSettingsRef = InReconSettings;
    ReconLayer.Initialize(InReconSettings, Map.Width, Map.Height);

    for (PlayerId I = 0; I < kMaxPlayers; ++I)
    {
        const MatchSetup::PlayerSlot& Slot = Setup.Players[I];
        PlayerState& P = Players[I];
        P.bActive = Slot.bActive;
        P.bDefeated = false;
        P.Faction = Slot.Faction;
        P.Credits = Slot.StartingCredits;
        switch (P.Faction)
        {
        case FactionId::Soviet: P.FactionResourceType = FactionResourceType::Mobilization; break;
        case FactionId::Alliance: P.FactionResourceType = FactionResourceType::Intelligence; break;
        case FactionId::EasternCoalition: P.FactionResourceType = FactionResourceType::Synchronization; break;
        case FactionId::ChronoLegion: P.FactionResourceType = FactionResourceType::TemporalStability; break;
        default: P.FactionResourceType = FactionResourceType::None; break;
        }
    }

    Phase = MatchPhase::Running;
}

void SimWorld::Restart()
{
    const ContentDatabase* SavedContent = Content;
    const MatchSetup SavedSetup = SetupConfig;
    const Recon::ReconSettings* SavedRecon = ReconSettingsRef;
    Initialize(SavedContent, SavedSetup, SavedRecon);
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
        DirectControls.emplace_back();
        Morales.emplace_back();
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
    DirectControls[Index] = DirectControlComp();
    Morales[Index] = Recon::MoraleComp();
 
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
const DirectControlComp* SimWorld::GetDirectControl(EntityId Id) const { return IsAlive(Id) ? &DirectControls[Id.Index] : nullptr; }

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

    // Legacy direct list / AllOf check
    for (const ContentId& Req : Def.Production.Prerequisites)
    {
        if (std::find(Have.begin(), Have.end(), Req) == Have.end())
        {
            return false;
        }
    }

    // Group-based AllOf check
    for (const ContentId& Req : Def.Production.PrerequisitesGroup.AllOf)
    {
        if (std::find(Have.begin(), Have.end(), Req) == Have.end())
        {
            return false;
        }
    }

    // Group-based AnyOf check
    if (!Def.Production.PrerequisitesGroup.AnyOf.empty())
    {
        bool bAnyMet = false;
        for (const ContentId& Req : Def.Production.PrerequisitesGroup.AnyOf)
        {
            if (std::find(Have.begin(), Have.end(), Req) != Have.end())
            {
                bAnyMet = true;
                break;
            }
        }
        if (!bAnyMet)
        {
            return false;
        }
    }

    // Group-based NoneOf check
    for (const ContentId& Forbidden : Def.Production.PrerequisitesGroup.NoneOf)
    {
        if (std::find(Have.begin(), Have.end(), Forbidden) != Have.end())
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
            if (Item->Kind == EntityKind::Unit && Player.CommandLimitUsed + Item->Production.CommandLimit > Player.CommandLimitMax)
            {
                return Reject(CommandReject::CommandCapExceeded);
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

        // --- Superweapon -----------------------------------------------
        // Validated like any other order: ownership, liveness, and the building
        // actually being a charged superweapon. There is no separate "cheat"
        // path -- the AI issues this exact command through the same bus.
        case CommandType::FireSuperweapon:
        {
            if (!IsAlive(Cmd.Primary))
            {
                return Reject(CommandReject::NoSuchEntity);
            }
            if (Core[Cmd.Primary.Index].Owner != Cmd.Issuer)
            {
                return Reject(CommandReject::NotOwner);
            }
            const EntityDef* Def = Content ? Content->FindEntity(Core[Cmd.Primary.Index].Def) : nullptr;
            if (Def == nullptr || Def->Building.SuperweaponRechargeTicks <= 0)
            {
                return Reject(CommandReject::UnknownContent);
            }
            BuildingComp& B = Buildings[Cmd.Primary.Index];
            if (B.State != ConstructionState::Complete)
            {
                return Reject(CommandReject::SuperweaponNotReady);
            }
            if (B.SuperweaponChargeTicks < Def->Building.SuperweaponRechargeTicks)
            {
                return Reject(CommandReject::SuperweaponNotReady);
            }
            if (Player.PowerConsumed > Player.PowerProduced)
            {
                return Reject(CommandReject::SuperweaponUnpowered);
            }
            if (!Map.IsInBounds(Cmd.Tile.X, Cmd.Tile.Y))
            {
                return Reject(CommandReject::TargetInvalid);
            }

            // Spend the charge before applying damage: a rejected-after-fire path
            // would let a player fire twice if damage resolution ever throws.
            B.SuperweaponChargeTicks = 0;

            const Vec2 Impact = Map.TileCenterToWorld(Cmd.Tile);
            // Reuses the ordinary splash path, so the armour table, fog and event
            // emission behave exactly as they do for a shell.
            ApplySplashDamage(Impact, Def->Building.SuperweaponRadius,
                              Def->Building.SuperweaponDamage,
                              Def->Building.SuperweaponWarhead,
                              /*FalloffPercent*/ 50, Cmd.Primary, Cmd.Issuer);

            SimEvent Ev;
            Ev.Type = SimEventType::WeaponFired;
            Ev.Tick = CurrentTick;
            Ev.Entity = Cmd.Primary;
            Ev.Player = Cmd.Issuer;
            Ev.Content = Core[Cmd.Primary.Index].Def;
            Ev.Location = Impact;
            Ev.Value = Def->Building.SuperweaponDamage;
            EmitEvent(Ev);
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

        // --- Direct vehicle control ----------------------------------------
        // All four commands go through the same ownership/alive checks as
        // ordinary orders. Enter/Exit mutate the DirectControlComp; Drive
        // mutates movement/turret via the profile; Fire re-uses FireWeapon so
        // damage, cooldown and ammo are identical to RTS fire.
        case CommandType::DirectControlEnter:
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
                return Reject(CommandReject::DirectIneligibleUnit);
            }
            // Only one driver per entity.
            const DirectControlComp& Existing = DirectControls[Cmd.Primary.Index];
            if (Existing.Phase == DirectControlPhase::Active ||
                Existing.Phase == DirectControlPhase::Entering)
            {
                if (Existing.Controller == Cmd.Issuer)
                {
                    // Idempotent re-entry from same player: accept silently.
                    break;
                }
                return Reject(CommandReject::DirectAlreadyControlled);
            }
            // Reject if the entity has no weapon and no turret (eligible check).
            // We treat any armed vehicle or any vehicle with TurretTurnRate>0 as
            // direct-controllable. The presentation profile gates which units the
            // client will *offer*; the simulation only enforces safety.
            const EntityDef* Def = Content ? Content->FindEntity(Core[Cmd.Primary.Index].Def) : nullptr;
            if (Def == nullptr)
            {
                return Reject(CommandReject::UnknownContent);
            }
            if (!Def->Weapon.IsValid() && Def->Unit.TurretTurnRatePerSecond == 0)
            {
                return Reject(CommandReject::DirectIneligibleUnit);
            }
            DirectControlComp& Dc = DirectControls[Cmd.Primary.Index];
            Dc.Phase = DirectControlPhase::Entering;
            Dc.Controller = Cmd.Issuer;
            Dc.PhaseUntilTick = CurrentTick + kDirectControlEnterExitTicks;
            Dc.CooldownTicksPrimary = 0;
            Dc.CooldownTicksSecondary = 0;
            Dc.bOpticsZoomed = false;
            // Clear current orders; the driver is now responsible for movement.
            Orders[Cmd.Primary.Index].Clear();
            Combats[Cmd.Primary.Index].Target = EntityId::Invalid();
            Combats[Cmd.Primary.Index].bTargetIsForced = false;
            {
                SimEvent Ev;
                Ev.Type = SimEventType::DirectControlEntered;
                Ev.Tick = CurrentTick;
                Ev.Entity = Cmd.Primary;
                Ev.Player = Cmd.Issuer;
                EmitEvent(Ev);
            }
            break;
        }

        case CommandType::DirectControlExit:
        {
            if (!IsAlive(Cmd.Primary))
            {
                return Reject(CommandReject::NoSuchEntity);
            }
            const DirectControlComp& Dc = DirectControls[Cmd.Primary.Index];
            if (Dc.Phase != DirectControlPhase::Active &&
                Dc.Phase != DirectControlPhase::Entering)
            {
                return Reject(CommandReject::DirectNotControlling);
            }
            if (Dc.Controller != Cmd.Issuer)
            {
                return Reject(CommandReject::NotOwner);
            }
            DirectControlComp& DcMut = DirectControls[Cmd.Primary.Index];
            DcMut.Phase = DirectControlPhase::Exiting;
            DcMut.PhaseUntilTick = CurrentTick + kDirectControlEnterExitTicks;
            // Hand back to AI: leave a Stop order so the vehicle does not drift.
            Orders[Cmd.Primary.Index].Clear();
            Movements[Cmd.Primary.Index].bHasDestination = false;
            Movements[Cmd.Primary.Index].CurrentSpeed = Fixed::Zero();
            {
                SimEvent Ev;
                Ev.Type = SimEventType::DirectControlExited;
                Ev.Tick = CurrentTick;
                Ev.Entity = Cmd.Primary;
                Ev.Player = Cmd.Issuer;
                EmitEvent(Ev);
            }
            break;
        }

        case CommandType::DirectControlDrive:
        {
            if (!IsAlive(Cmd.Primary))
            {
                return Reject(CommandReject::NoSuchEntity);
            }
            DirectControlComp& Dc = DirectControls[Cmd.Primary.Index];
            if (Dc.Phase != DirectControlPhase::Active &&
                Dc.Phase != DirectControlPhase::Entering)
            {
                return Reject(CommandReject::DirectNotControlling);
            }
            if (Dc.Controller != Cmd.Issuer)
            {
                return Reject(CommandReject::NotOwner);
            }
            const EntityDef* Def = Content ? Content->FindEntity(Core[Cmd.Primary.Index].Def) : nullptr;
            if (Def == nullptr)
            {
                return Reject(CommandReject::UnknownContent);
            }
            // Throttle -> forward movement. We use the same MovementComp the
            // ordinary SystemMovement will integrate, so speed limits stay
            // identical to RTS control.
            const int32_t Throttle = std::clamp(int32_t(Cmd.DirectAxes.Throttle), -127, 127);
            const int32_t Steering = std::clamp(int32_t(Cmd.DirectAxes.Steering), -127, 127);
            if (Throttle > 0)
            {
                const Fixed Fwd = Def->Unit.MaxSpeed * Fixed::FromRatio(Throttle, 127);
                // Reuse the order queue with a small forward waypoint, refreshed
                // every tick. The arrive radius keeps the vehicle from fighting
                // itself.
                const Vec2 Pos = Transforms[Cmd.Primary.Index].Position;
                const int32_t Facing = Transforms[Cmd.Primary.Index].Facing;
                const Vec2 Dir = Vec2::FromAngle(Facing);
                const Vec2 Dest = Pos + Dir * Fwd * Fixed::FromInt(2);
                Orders[Cmd.Primary.Index].Clear();
                Order O;
                O.Type = OrderType::Move;
                O.Location = Dest;
                Orders[Cmd.Primary.Index].Push(O);
            }
            else if (Throttle < 0)
            {
                const Fixed Rev = Def->Unit.MaxSpeed * Fixed::FromRatio(-Throttle, 127) * Fixed::FromRatio(1, 2);
                const Vec2 Pos = Transforms[Cmd.Primary.Index].Position;
                const int32_t Facing = Transforms[Cmd.Primary.Index].Facing;
                const Vec2 Dir = Vec2::FromAngle(Facing);
                const Vec2 Dest = Pos - Dir * Rev * Fixed::FromInt(2);
                Orders[Cmd.Primary.Index].Clear();
                Order O;
                O.Type = OrderType::Move;
                O.Location = Dest;
                Orders[Cmd.Primary.Index].Push(O);
            }
            else
            {
                // No throttle: stop.
                Orders[Cmd.Primary.Index].Clear();
                Movements[Cmd.Primary.Index].CurrentSpeed = Fixed::Zero();
                Movements[Cmd.Primary.Index].bHasDestination = false;
            }
            // Steering -> hull facing delta (centi-degrees per tick).
            if (Steering != 0 && Def->Unit.TurnRatePerSecond > 0)
            {
                const int32_t TurnPerTickCenti =
                    (int32_t(Def->Unit.TurnRatePerSecond) * 100) / int32_t(kTicksPerSecond);
                Transforms[Cmd.Primary.Index].Facing +=
                    (Steering * TurnPerTickCenti) / 127;
            }
            // Turret yaw/pitch -> quantized; full integration in SystemDirectControl.
            Dc.TurretYawCentiDeg += int32_t(Cmd.DirectAxes.TurretYaw) * 16; // scale applied here
            Dc.TurretPitchCentiDeg = std::clamp(
                Dc.TurretPitchCentiDeg + int32_t(Cmd.DirectAxes.TurretPitch) * 16,
                -8000, 8000); // +/- 80 deg clamp
            // Flags handled by Fire command in the same frame, not here.
            const uint8_t Flags = Cmd.DirectAxes.Flags;
            if (Flags & 0x08) { Dc.bOpticsZoomed = !Dc.bOpticsZoomed; }
            // Promote Entering -> Active on first Drive.
            if (Dc.Phase == DirectControlPhase::Entering)
            {
                Dc.Phase = DirectControlPhase::Active;
                Dc.PhaseUntilTick = 0;
            }
            break;
        }

        case CommandType::DirectControlFire:
        {
            if (!IsAlive(Cmd.Primary))
            {
                return Reject(CommandReject::NoSuchEntity);
            }
            const DirectControlComp& Dc = DirectControls[Cmd.Primary.Index];
            if (Dc.Phase != DirectControlPhase::Active &&
                Dc.Phase != DirectControlPhase::Entering)
            {
                return Reject(CommandReject::DirectNotControlling);
            }
            if (Dc.Controller != Cmd.Issuer)
            {
                return Reject(CommandReject::NotOwner);
            }
            const EntityDef* Def = Content ? Content->FindEntity(Core[Cmd.Primary.Index].Def) : nullptr;
            if (Def == nullptr || !Def->Weapon.IsValid())
            {
                return Reject(CommandReject::DirectWeaponEmpty);
            }
            const WeaponDef* Wpn = Content ? Content->FindWeapon(Def->Weapon) : nullptr;
            if (Wpn == nullptr)
            {
                return Reject(CommandReject::DirectWeaponEmpty);
            }
            const uint8_t Flags = Cmd.DirectAxes.Flags;
            const bool bSecondary = (Flags & 0x02) != 0;
            const ContentId WpnId = bSecondary && Def->SecondaryWeapon.IsValid()
                ? Def->SecondaryWeapon
                : Def->Weapon;
            const WeaponDef* WpnToFire = (WpnId == Def->Weapon) ? Wpn
                : (Content ? Content->FindWeapon(WpnId) : nullptr);
            if (WpnToFire == nullptr)
            {
                return Reject(CommandReject::DirectWeaponEmpty);
            }
            DirectControlComp& DcMut = DirectControls[Cmd.Primary.Index];
            int32_t& Cooldown = (WpnId == Def->Weapon)
                ? DcMut.CooldownTicksPrimary
                : DcMut.CooldownTicksSecondary;
            if (Cooldown > 0)
            {
                return Reject(CommandReject::DirectWeaponCooldown);
            }
            // Target: use forced target from Cmd.Target if alive & hostile;
            // otherwise acquire via the existing system so first-person fire
            // can never shoot through fog of war.
            EntityId Target = Cmd.Target;
            if (!IsAlive(Target) || !IsHostile(Cmd.Issuer, Core[Target.Index].Owner))
            {
                Target = AcquireTarget(Cmd.Primary);
            }
            if (!Target.IsValid())
            {
                return Reject(CommandReject::TargetInvalid);
            }
            // Same fire path as RTS: same damage, same cooldown, same projectile.
            FireWeapon(Cmd.Primary, Target, *WpnToFire);
            Cooldown = WpnToFire->CooldownTicks;
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

void SimWorld::DebugDamage(EntityId TargetId, int32_t DamageAmount)
{
    if (IsAlive(TargetId))
    {
        HealthComp& H = Healths[TargetId.Index];
        H.Current = (DamageAmount >= H.Current) ? 0 : (H.Current - DamageAmount);
        if (H.Current == 0)
        {
            PendingDestroy.push_back(TargetId);
        }
    }
}

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
        Buildings[Index].Queue.clear();
        Buildings[Index].DockedHarvester = EntityId::Invalid();
        Buildings[Index].UnloadingQueue.clear();
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

    // If this entity was under direct control, emit the exit event and mark
    // the phase so the presentation layer knows the player was ejected by
    // destruction rather than by an explicit Exit command.
    if (DirectControls[Index].Phase == DirectControlPhase::Active ||
        DirectControls[Index].Phase == DirectControlPhase::Entering)
    {
        SimEvent DcEv;
        DcEv.Type = SimEventType::DirectControlExited;
        DcEv.Tick = CurrentTick;
        DcEv.Entity = Id;
        DcEv.Player = DirectControls[Index].Controller;
        EmitEvent(DcEv);
        DirectControls[Index].Phase = DirectControlPhase::VehicleDestroyed;
        DirectControls[Index].PhaseUntilTick = 0;
        DirectControls[Index].Controller = kInvalidPlayer;
    }

    Core[Index].bAlive = false;
    Core[Index].Generation += 1;
    FreeSlots.push_back(Index);

    if (Kind == EntityKind::Building && Owner < kMaxPlayers)
    {
        RefreshPlayerTech(Owner);
    }
}

void SimWorld::CheatGrantCredits(PlayerId Owner, int32_t Amount)
{
    if (Owner < kMaxPlayers)
    {
        Players[Owner].Credits += Amount;
    }
}

void SimWorld::CheatGrantPower(PlayerId Owner, int32_t PowerAmount)
{
    if (Owner < kMaxPlayers)
    {
        Players[Owner].PowerProduced += PowerAmount;
    }
}

void SimWorld::CheatInstantBuild(PlayerId Owner)
{
    for (size_t Index = 0; Index < Core.size(); ++Index)
    {
        if (Core[Index].bAlive && Core[Index].Owner == Owner)
        {
            if (Index < Buildings.size())
            {
                Buildings[Index].State = ConstructionState::Complete;
                Buildings[Index].ConstructionProgressTicks = Buildings[Index].ConstructionTotalTicks;
                if (!Buildings[Index].Queue.empty())
                {
                    Buildings[Index].Queue.front().ProgressTicks = Buildings[Index].Queue.front().TotalTicks;
                }
            }
        }
    }
}

void SimWorld::CheatToggleGodMode(PlayerId Owner)
{
    for (size_t Index = 0; Index < Core.size(); ++Index)
    {
        if (Core[Index].bAlive && Core[Index].Owner == Owner)
        {
            if (Index < Healths.size())
            {
                Healths[Index].bInvulnerable = !Healths[Index].bInvulnerable;
                Healths[Index].Current = 999999;
                Healths[Index].Max = 999999;
            }
        }
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
        P.CommandLimitMax = 50;
        P.CommandLimitUsed = 0;
    }
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Owner >= kMaxPlayers)
        {
            continue;
        }
        const EntityDef* D = Content ? Content->FindEntity(Core[I].Def) : nullptr;
        if (D == nullptr)
        {
            continue;
        }
        PlayerState& P = Players[Core[I].Owner];
        if (Core[I].Kind == EntityKind::Building && Buildings[I].State == ConstructionState::Complete)
        {
            const int64_t HealthRatio = Healths[I].Max > 0 ? (int64_t(Healths[I].Current) * 100) / Healths[I].Max : 0;
            P.PowerProduced += int32_t((int64_t(D->Building.PowerProduced) * HealthRatio) / 100);
            P.PowerConsumed += D->Building.PowerConsumed;
            P.CommandLimitMax += D->Building.CommandLimitProvided;
        }
        else if (Core[I].Kind == EntityKind::Unit)
        {
            P.CommandLimitUsed += D->Production.CommandLimit;
        }
    }
    for (PlayerState& P : Players)
    {
        P.CommandLimitMax = std::min(200, P.CommandLimitMax);
        if (P.Faction == FactionId::ChronoLegion && (CurrentTick % 40) == 0)
        {
            P.FactionResource = std::min(100, P.FactionResource + 1);
        }
    }

    // Superweapon charge, in a second pass because it depends on the power totals
    // the loop above has just finished computing. Charging requires a power
    // surplus, so cutting an opponent's power stalls their superweapon -- the
    // clock is a consequence of holding a working base, not a wall-clock timer.
    // Placed inside SystemPower rather than as a new system so the fixed system
    // order, which is part of the replay compatibility contract, is unchanged.
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Building ||
            Core[I].Owner >= kMaxPlayers)
        {
            continue;
        }
        if (Buildings[I].State != ConstructionState::Complete)
        {
            continue;
        }
        const EntityDef* D = Content ? Content->FindEntity(Core[I].Def) : nullptr;
        if (D == nullptr || D->Building.SuperweaponRechargeTicks <= 0)
        {
            continue;
        }
        const PlayerState& Owner = Players[Core[I].Owner];
        if (Owner.PowerConsumed > Owner.PowerProduced)
        {
            continue;   // brownout: no charge this tick
        }
        BuildingComp& B = Buildings[I];
        if (B.SuperweaponChargeTicks < D->Building.SuperweaponRechargeTicks)
        {
            B.SuperweaponChargeTicks += 1;
        }
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
                const Fixed NodeDock = DockRadiusFor(Content->FindEntity(Core[H.AssignedNode.Index].Def));
                if (DistanceSquared(Pos, NodePos) <= NodeDock * NodeDock)
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
                if (!IsAlive(H.AssignedRefinery) || Buildings[H.AssignedRefinery.Index].State != ConstructionState::Complete)
                {
                    H.AssignedRefinery = FindNearestRefinery(Pos, Owner);
                }
                if (!H.AssignedRefinery.IsValid())
                {
                    Movements[I].bHasDestination = false;
                    break;
                }
                BuildingComp& RefComp = Buildings[H.AssignedRefinery.Index];
                const Vec2 RefPos = Transforms[H.AssignedRefinery.Index].Position;
                const Fixed RefDock = DockRadiusFor(Content->FindEntity(Core[H.AssignedRefinery.Index].Def));
                const EntityId SelfId = MakeId(I);

                if (DistanceSquared(Pos, RefPos) <= RefDock * RefDock)
                {
                    if (!RefComp.DockedHarvester.IsValid() || !IsAlive(RefComp.DockedHarvester) || RefComp.DockedHarvester == SelfId)
                    {
                        RefComp.DockedHarvester = SelfId;
                        H.State = HarvesterState::Unloading;
                        Movements[I].bHasDestination = false;
                        Movements[I].CurrentSpeed = Fixed::Zero();
                    }
                    else
                    {
                        if (std::find(RefComp.UnloadingQueue.begin(), RefComp.UnloadingQueue.end(), SelfId) == RefComp.UnloadingQueue.end())
                        {
                            RefComp.UnloadingQueue.push_back(SelfId);
                        }
                        Movements[I].bHasDestination = false;
                        Movements[I].CurrentSpeed = Fixed::Zero();
                    }
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
                const EntityId SelfId = MakeId(I);
                if (!IsAlive(H.AssignedRefinery))
                {
                    H.State = HarvesterState::MovingToRefinery;
                    break;
                }
                BuildingComp& RefComp = Buildings[H.AssignedRefinery.Index];
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
                    Ev.Entity = SelfId;
                    Ev.Other = H.AssignedRefinery;
                    Ev.Player = Owner;
                    Ev.Value = Amount * Value;
                    EmitEvent(Ev);
                }

                if (H.Cargo <= 0)
                {
                    if (RefComp.DockedHarvester == SelfId)
                    {
                        RefComp.DockedHarvester = EntityId::Invalid();
                        while (!RefComp.UnloadingQueue.empty())
                        {
                            EntityId NextId = RefComp.UnloadingQueue.front();
                            RefComp.UnloadingQueue.erase(RefComp.UnloadingQueue.begin());
                            if (IsAlive(NextId))
                            {
                                RefComp.DockedHarvester = NextId;
                                Harvesters[NextId.Index].State = HarvesterState::Unloading;
                                break;
                            }
                        }
                    }
                    H.State = HarvesterState::MovingToResource;
                }
                break;
            }
        }
    }
}

void SimWorld::SystemOrders()
{
    // Group-size tally for the arrive radius, built once per tick. SystemMovement
    // builds the same tally from the same state and both call
    // ScaleArriveRadiusForGroup, so the radius that pops an order here is identical
    // to the radius that stops the unit there. They must not diverge: if this system
    // demanded a tighter radius than SystemMovement, the unit would stop moving while
    // its Move order stayed queued, and the code below would re-set bHasDestination
    // every tick forever.
    //
    // Rebuilt here rather than shared through a member so this change stays inside
    // SimWorld.cpp. The two passes read the same Movements array in the same tick
    // with no intervening writes to bHasDestination or Destination -- SystemOrders is
    // the first of the pair -- so both see the same set. Note the ordering subtlety:
    // this tally is taken BEFORE this system updates destinations, so it reflects
    // last tick's assignments. That is deliberate and harmless: group size is used
    // only to widen a tolerance, and a one-tick-stale crowd count changes the radius
    // by at most one unit's worth of contribution.
    std::vector<uint64_t> DestTileKeys;
    DestTileKeys.reserve(Core.size());
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Unit) continue;
        if (!Movements[I].bHasDestination) continue;
        DestTileKeys.push_back(PackDestTileKey(Map.WorldToTile(Movements[I].Destination)));
    }
    std::sort(DestTileKeys.begin(), DestTileKeys.end());

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
                const Fixed D2 = DistanceSquared(Transforms[I].Position, O.Location);
                const Fixed OrderArrive = ScaleArriveRadiusForGroup(
                    M.ArriveRadius, GroupSizeAtDestTile(DestTileKeys, Map.WorldToTile(O.Location)));
                if (D2 <= OrderArrive * OrderArrive)
                {
                    M.bHasDestination = false;
                    M.CurrentSpeed = Fixed::Zero();
                    Q.PopFront();
                }
                // Give up rather than grind forever against an obstacle -- but only
                // for a unit that is genuinely stuck, never for one that is merely
                // waiting its turn in a crowd.
                //
                // WHY THIS IS NOT A PLAIN BlockedTicks THRESHOLD ANY MORE. The
                // reservation tie-break at ReservationGrid.cpp:50 is a static
                // priority: a tile is only displaced by a *strictly lower* entity
                // slot. In a dense crowd converging on one area, a high-index unit
                // therefore loses every contest to every lower-index neighbour and
                // takes the `BlockedTicks += 1` branch in SystemMovement step 7b for
                // as long as the jam lasts. That is ordinary, self-clearing
                // congestion: the units ahead do move on, and the jam drains.
                //
                // Because SystemOrders runs immediately BEFORE SystemMovement
                // (SimWorld.cpp:3628-3629), the old unconditional test here fired
                // first and cancelled the Move order at the same threshold that
                // SystemMovement uses to trigger a repath -- so the order was
                // destroyed on precisely the tick the navigation layer was about to
                // retry. Measured on 200 units sent to one rally point: 73 of them
                // had their order cancelled here between ticks 60 and 63 while still
                // 17-63 tiles away, having never been blocked by any terrain. Only
                // 127 of 200 arrived. Nothing recovers from this, because popping the
                // order is permanent while the congestion was temporary.
                //
                // The condition now requires BOTH of:
                //   (a) the unit has been blocked for the threshold, AND
                //   (b) the navigation layer has already tried to repath it and still
                //       cannot make progress -- i.e. this is a routing failure, not a
                //       queueing delay.
                // SystemMovement sets LastRepathTick only on the paths that clear a
                // dead macro path (no route found, or blocked long enough to force a
                // rebuild). A unit losing reservation contests never sets it, so
                // congestion alone can no longer pop an order.
                //
                // The extra grace window is deliberate and is what makes (b) a real
                // second opinion rather than a restatement of (a): after a repath is
                // requested the unit is given another full threshold to act on the
                // new route before its order is abandoned. A unit that repaths and
                // then moves resets BlockedTicks and never reaches this branch at all.
                //
                // Deterministic: integer tick arithmetic only, and both operands are
                // authoritative serialized state (BlockedTicks, LastRepathTick), so a
                // save/load round-trip resumes with the identical give-up decision.
                // TickIndex is unsigned, so the elapsed-time comparison is written as
                // an addition rather than a subtraction to avoid wrapping when
                // LastRepathTick is still its initial 0.
                else if (M.BlockedTicks > kTicksPerSecond * 3 &&
                         M.LastRepathTick > 0 &&
                         CurrentTick >= M.LastRepathTick +
                             static_cast<TickIndex>(kRepathBlockedTickThreshold))
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
                    const Fixed AmArrive = ScaleArriveRadiusForGroup(
                        M.ArriveRadius, GroupSizeAtDestTile(DestTileKeys, Map.WorldToTile(O.Location)));
                    if (DistanceSquared(Transforms[I].Position, O.Location) <= AmArrive * AmArrive)
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

namespace
{
// Sentinel for "this formation has no living leader this tick".
constexpr uint32_t kNoFormationLeader = 0xFFFFFFFFu;

// Soft separation (see SystemMovement). Two units closer than this many world units
// push apart. The tile is 200 units and the default CollisionRadius is 20, so this
// keeps roughly three bodies per tile edge rather than forcing one unit per tile --
// formations and choke points still work.
constexpr int64_t kSeparationRadiusUnits = 56;

// The separation nudge is deliberately smaller than the deadband below, so a pair
// that has just been pushed apart cannot be pushed back on the following tick. That
// asymmetry is what makes the fixed point stable instead of a two-tick oscillation.
constexpr int64_t kSeparationStepUnits = 6;

// Overlap below this is left alone. Without a deadband a pair sitting exactly at the
// separation radius would jitter across it forever, reading as "arrived" to a
// distance check while never actually settling.
constexpr int64_t kSeparationDeadbandUnits = 10;

// Returns the entity slot leading Formation, or kNoFormationLeader.
//
// A file-local free function rather than a SimWorld method: adding a member would
// mean declaring it in SimWorld.h, and this change is confined to SimWorld.cpp.
//
// The leader is not stored anywhere. MovementComp carries only FormationId and
// FormationSlot, and there is no leader handle on the component or on SimWorld. The
// leader is therefore identified by its slot, and BOTH documented spellings of
// "leader" are accepted: SimTypes.h:183 defines -1 as "leader or unassigned", while
// Formation.h:81-83 defines slot 0 as the leader's own zero-offset slot. Accepting
// only one of the two would leave a formation assembled under the other convention
// permanently leaderless, and every member would then silently fall back to its own
// destination -- the formation would look like it simply did not work.
//
// Where several units qualify -- a malformed assignment, or a leader killed and its
// formation id reused -- the lowest entity slot wins. That is the same deterministic
// tie-break ReservationGrid uses, so every peer selects the same leader.
//
// COST: linear in the entity count, per member, per tick. That is acceptable only
// because formations are rare and small relative to kMaxEntities; if formations
// become common this wants a leader handle on the component or a per-tick index,
// which is a SimWorld.h change and therefore out of scope here.
uint32_t FindFormationLeader(const std::vector<EntityCore>& Core,
                            const std::vector<MovementComp>& Movements,
                            ContentId Formation)
{
    if (!Formation.IsValid())
    {
        return kNoFormationLeader;
    }
    for (uint32_t I = 0; I < uint32_t(Core.size()); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Unit)
        {
            continue;
        }
        if (Movements[I].FormationId == Formation && Movements[I].FormationSlot <= 0)
        {
            return I;
        }
    }
    return kNoFormationLeader;
}
} // namespace

void SimWorld::SystemMovement()
{
    if (Reservations) Reservations->Expire(CurrentTick);
    FlowFieldBuildsThisTick = 0;
    MacroPathBuildsThisTick = 0;

    // Shared turn / accelerate / advance step. Extracted so the final approach and
    // the flow-field path steer through identical code -- if they diverged, a unit
    // would behave differently in the last tile than it did on the way there.
    auto SteerToward = [this](uint32_t Index, const EntityDef& Def, const Vec2& Target,
                              const Nav::NavQuery& Query)
    {
        MovementComp& M = Movements[Index];
        TransformComp& T = Transforms[Index];

        const Vec2 SteeringDelta = Target - T.Position;
        if (SteeringDelta.LengthSquared().Raw == 0)
        {
            return;
        }

        const int32_t DesiredFacing = SteeringDelta.ToAngle();
        const int32_t TurnPerTick = std::max(1, Def.Unit.TurnRatePerSecond / kTicksPerSecond);
        const int32_t Diff = AngleDelta(T.Facing, DesiredFacing);
        if (Diff > TurnPerTick) { T.Facing = WrapAngle(T.Facing + TurnPerTick); }
        else if (Diff < -TurnPerTick) { T.Facing = WrapAngle(T.Facing - TurnPerTick); }
        else { T.Facing = DesiredFacing; }

        const Fixed MaxSpeedPerTick = PerSecondToPerTick(Def.Unit.MaxSpeed);
        const Fixed AccelPerTick = PerSecondToPerTick(Def.Unit.Acceleration);
        const int32_t AlignedThreshold = kAngleTurn / 8;
        if (Diff > -AlignedThreshold && Diff < AlignedThreshold)
        {
            M.CurrentSpeed = FxMin(M.CurrentSpeed + AccelPerTick, MaxSpeedPerTick);
        }
        else
        {
            // Decelerate toward zero when not aligned with the target heading.
            // The old floor of MaxSpeedPerTick/4 made units slide at quarter speed
            // in whatever direction they happened to be facing -- including away
            // from the destination -- which is why harvesters drove past their
            // docking point and never arrived.
            M.CurrentSpeed = FxMax(M.CurrentSpeed - AccelPerTick, Fixed::Zero());
        }

        // Never overshoot the target inside one tick: at 25 units per tick a unit
        // would otherwise oscillate around a point it can never land on.
        Fixed StepLength = M.CurrentSpeed;
        const Fixed Remaining = SteeringDelta.Length();
        if (StepLength > Remaining)
        {
            StepLength = Remaining;
        }

        const Vec2 NextPos = T.Position + Vec2::FromAngle(T.Facing) * StepLength;
        const TileCoord NextPosTile = Map.WorldToTile(NextPos);
        if (NavigationGrid->IsTraversable(NextPosTile, Query))
        {
            T.Position = NextPos;
            M.BlockedTicks = 0;
        }
        else
        {
            M.CurrentSpeed = Fixed::Zero();
            M.BlockedTicks += 1;
        }
    };

    // --- Group-size arrive radius (SimTypes.h:172) --------------------------
    //
    // SimTypes.h documents ArriveRadius as "Scaled by group size in the navigation
    // milestone so a hundred units do not pile onto one point", but SpawnUnit set it
    // once from CollisionRadius and nothing ever scaled it. This tallies how many
    // living units share each destination tile this tick, so the radius used below can
    // widen with the crowd.
    //
    // Keyed on the destination TILE, not the exact Vec2. Two units sent to one spot by
    // a single order carry bit-identical destinations, but a unit whose destination was
    // derived (a harvester's node approach, a rally point) lands a few units off and
    // would hash to a different key while still contending for the same ground. The
    // tile is the granularity the reservation grid actually fights over, so it is the
    // granularity that matters here.
    //
    // Recomputed every tick rather than cached: group membership changes as units
    // arrive and orders are reissued, and a stale tally would leave a lone straggler
    // using a radius sized for the crowd it used to belong to.
    //
    // This is a MINOR contributor, as briefed. The dominant loss is reservation
    // priority starvation, addressed by the separation pass at the end of this
    // function. A wider arrival tolerance only stops the last few units of a genuine
    // pile-up from grinding; it cannot unfreeze a starved unit.
    std::vector<uint64_t> DestTileKeys;
    DestTileKeys.reserve(Core.size());
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Unit) continue;
        if (!Movements[I].bHasDestination) continue;
        DestTileKeys.push_back(PackDestTileKey(Map.WorldToTile(Movements[I].Destination)));
    }
    std::sort(DestTileKeys.begin(), DestTileKeys.end());

    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Unit) continue;
        MovementComp& M = Movements[I];
        TransformComp& T = Transforms[I];
        const EntityDef* D = Content->FindEntity(Core[I].Def);
        if (D == nullptr) continue;

        // --- Formation slot derivation -------------------------------------
        // Members do not own a destination: theirs is LeaderPos + rotated slot
        // offset, recomputed every tick (the contract stated at Formation.h:2-7).
        //
        // This runs HERE, at the top of the movement pass, and not in SystemOrders,
        // because SystemOrders executes immediately before this system and writes
        // `M.Destination = O.Location` unconditionally from the order queue. Setting
        // a slot destination any earlier in the tick would simply be overwritten;
        // setting it in SystemOrders itself would fight the order queue every tick,
        // with whichever wrote last winning. Overriding after that write is the only
        // placement that yields one unambiguous destination per tick.
        //
        // It is idempotent: the slot point is a pure function of the leader's
        // position, the leader's facing and the slot offset. Nothing here reads or
        // mutates the order queue, so running it twice in a tick, or after any
        // other system, produces the same value. It does not depend on the relative
        // order of SystemOrders and SystemMovement for *correctness* of the value --
        // only for which of the two writes survives, which is the point.
        //
        // TWO CONVENTIONS FOR "LEADER", reconciled here. SimTypes.h:183 documents
        // `FormationSlot == -1` as "leader or unassigned", while Formation.h:81-83
        // documents slot 0 as the leader's own slot, always the zero offset, so that
        // slot indices line up with FormationAssignment::Members without an
        // off-by-one. Both are therefore leaders and neither may be steered as a
        // follower. Slot 0 is excluded explicitly rather than relied upon to be
        // harmless: its offset is (0,0), so treating it as a follower would order the
        // leader to its own position every tick, zeroing the destination its own order
        // had set and pinning the whole formation in place. Requiring slot >= 1 is
        // what keeps the leader on its macro path.
        bool bIsFormationMember = false;
        if (M.FormationSlot >= 1 && M.FormationId.IsValid())
        {
            const uint32_t LeaderSlot = FindFormationLeader(Core, Movements, M.FormationId);
            const FormationDef* const FDef = FindFormationDef(M.FormationId);
            // Every one of these is a fallback to the unit's own destination, never
            // a crash and never a freeze: unknown formation id, no leader alive,
            // the leader being this very unit, or a slot the authored offset table
            // is too short to describe.
            if (LeaderSlot != kNoFormationLeader && FDef != nullptr && LeaderSlot != I &&
                size_t(M.FormationSlot) < FDef->Offsets.size())
            {
                M.Destination = Transforms[LeaderSlot].Position +
                                RotateOffset(FDef->Offsets[size_t(M.FormationSlot)],
                                             Transforms[LeaderSlot].Facing);
                M.bHasDestination = true;
                bIsFormationMember = true;
            }
        }

        if (!M.bHasDestination)
        {
            M.CurrentSpeed = Fixed::Zero();
            M.BlockedTicks = 0;
            if (Reservations) Reservations->Release(I);
            continue;
        }

        // 1. Arrived?
        // A formation member intentionally does NOT latch arrival: clearing
        // bHasDestination here is harmless because the block above re-derives and
        // re-sets it at the top of the next tick. That is the desired behaviour --
        // a member parked in its slot must start moving again the instant the leader
        // does, so "arrived" can only ever be true for the current tick. The
        // arrive-radius test still matters for members: it is what stops them
        // grinding against the slot point once they are in it.
        //
        // EffectiveArriveRadius widens M.ArriveRadius by the size of the group headed
        // for the same tile (see ScaleArriveRadiusForGroup). M.ArriveRadius itself is
        // NOT mutated: it is per-unit authoritative state that is serialized and
        // restored, and writing a crowd-derived value into it would make a unit's
        // saved radius depend on who happened to be moving alongside it at save time.
        // The scale is applied where it is read instead.
        const int64_t GroupSize = GroupSizeAtDestTile(DestTileKeys, Map.WorldToTile(M.Destination));
        const Fixed EffectiveArriveRadius = ScaleArriveRadiusForGroup(M.ArriveRadius, GroupSize);
        const Fixed ArriveSq = EffectiveArriveRadius * EffectiveArriveRadius;

        const Vec2 GoalDelta = M.Destination - T.Position;
        const Fixed GoalDistSq = GoalDelta.LengthSquared();
        if (GoalDistSq <= ArriveSq)
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

        // 1b. Formation members steer straight at their slot and return here.
        //
        // This is the whole performance rationale for formations, and it is why the
        // branch sits ABOVE the macro-path and flow-field stages rather than falling
        // through them: a member never reaches step 3, so N members cannot request N
        // macro paths or N flow fields. Only the leader -- which has FormationSlot
        // < 0 and therefore never entered the block above -- runs the pathing
        // stages, giving one path and one field per formation regardless of size.
        //
        // Steering directly is sound because the slot point tracks a leader that is
        // itself following a legal path, so the member is towed along a route the
        // navigation grid already approved. SteerToward still refuses to enter an
        // untraversable tile, so a member cannot be dragged through a cliff; it
        // stalls against it and closes again once the leader has moved on.
        //
        // Members are also excluded from tile reservations. Slot points are distinct
        // by construction, and letting members contend would reintroduce exactly the
        // priority starvation described at step 7 -- with the added twist that a
        // starved member freezes while its leader walks away, permanently.
        if (bIsFormationMember)
        {
            if (Reservations) Reservations->Release(I);
            SteerToward(I, *D, M.Destination, Query);
            continue;
        }

        const TileCoord FromTile = Map.WorldToTile(T.Position);
        const TileCoord ToTile = ResolveNavigationTarget(Map.WorldToTile(M.Destination), Query);

        // 2. Final approach. Inside the destination tile the flow field has nothing
        // left to say -- its direction at its own target is zero -- so steer at the
        // exact point instead. Without this a unit stops wherever the tile centre
        // happens to be, up to a tile-width short of where it was sent, stalls, and
        // the order system eventually gives up on it. That was the cause of
        // harvesters never docking and the vertical slice harvesting nothing.
        if (FromTile.X == ToTile.X && FromTile.Y == ToTile.Y)
        {
            if (Reservations) Reservations->Release(I);
            const Vec2 TargetPos = NavigationGrid->IsTraversable(Map.WorldToTile(M.Destination), Query)
                                       ? M.Destination
                                       : Map.TileCenterToWorld(ToTile);
            SteerToward(I, *D, TargetPos, Query);
            const Vec2 Delta = TargetPos - T.Position;
            // Same effective radius as the step-1 test above. If this one kept the
            // unscaled M.ArriveRadius the two checks would disagree: a unit could
            // satisfy step 1 next tick while failing here this tick, and inside the
            // destination tile that means grinding at the exact point it was already
            // close enough to.
            if (Delta.LengthSquared() <= ArriveSq)
            {
                M.bHasDestination = false;
                M.CurrentSpeed = Fixed::Zero();
                M.BlockedTicks = 0;
            }
            else if (M.BlockedTicks > kRepathBlockedTickThreshold)
            {
                M.CurrentMacroPath = Nav::MacroPath{};
                M.BlockedTicks = 0;
                M.LastRepathTick = CurrentTick;
            }
            continue;
        }

        // 3. Macro path (budgeted, shared, topology-aware).
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

        // 4. Sub-goal selection.
        const int32_t Sx = FromTile.X / Nav::NavGrid::kSectorSize;
        const int32_t Sy = FromTile.Y / Nav::NavGrid::kSectorSize;
        const int32_t Dsx = ToTile.X / Nav::NavGrid::kSectorSize;
        const int32_t Dsy = ToTile.Y / Nav::NavGrid::kSectorSize;

        if (Sx == Dsx && Sy == Dsy)
        {
            // Same sector as the goal: the macro path has nothing to contribute, so
            // head straight for the destination tile rather than a sector centre.
            M.CurrentSubGoal = ToTile;
        }
        else if (!M.CurrentMacroPath.Waypoints.empty())
        {
            const int32_t Count = int32_t(M.CurrentMacroPath.Waypoints.size());
            // Consume every waypoint whose sector we have already entered.
            while (M.NextWaypointIndex < Count)
            {
                const TileCoord& Waypoint = M.CurrentMacroPath.Waypoints[size_t(M.NextWaypointIndex)];
                if (Waypoint.X / Nav::NavGrid::kSectorSize == Sx &&
                    Waypoint.Y / Nav::NavGrid::kSectorSize == Sy)
                {
                    ++M.NextWaypointIndex;
                    continue;
                }
                break;
            }
            // Once the corridor is exhausted the goal itself becomes the sub-goal.
            // Leaving the last waypoint in place here is what parked units on sector
            // centres and made them report arrival far from their destination.
            M.CurrentSubGoal = M.NextWaypointIndex < Count
                                   ? M.CurrentMacroPath.Waypoints[size_t(M.NextWaypointIndex)]
                                   : ToTile;
        }
        else
        {
            // No macro path (unreachable or budget-stalled). Count blocked.
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

        // 5. Flow field for the sub-goal (shared across all units heading there).
        const Nav::FlowField* Field = GetFlowField(M.CurrentSubGoal, Query);
        if (Field == nullptr || !Field->IsReachable(FromTile))
        {
            M.BlockedTicks += 1;
            M.CurrentSpeed = Fixed::Zero();
            continue;
        }

        // 6. Steering: flow direction -> desired tile.
        const Nav::FlowDirection Dir = Field->GetDirection(FromTile);
        if (Dir.X == 0 && Dir.Y == 0)
        {
            // Standing on the sub-goal tile but not yet on the goal: aim at the real
            // destination so the unit keeps closing instead of declaring itself stuck.
            SteerToward(I, *D, M.Destination, Query);
            continue;
        }
        const TileCoord DesiredTile(FromTile.X + Dir.X, FromTile.Y + Dir.Y);

        // 7. Reservation (soft, slot-order tie-break).
        // Drop last tick's claim first. The tie-break only lets a *lower* slot take
        // an occupied tile, so a unit could not re-reserve the tile it was already
        // holding: it fell through to avoidance every tick and stalled one step from
        // where it started.
        if (Reservations) Reservations->Release(I);

        TileCoord NextTile = DesiredTile;
        if (Reservations && Reservations->TryReserve(DesiredTile, I, CurrentTick, /*HoldTicks=*/2))
        {
            M.BlockedTicks = 0;
        }
        else
        {
            ++Stats.ReservationContests;
            // 7b. Local avoidance: best open neighbor by flow-direction alignment.
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
                // Never "avoid" into the tile we already occupy. The neighbours of
                // the desired tile include our own, and a unit standing on that
                // tile's centre would then steer at a point zero units away, take a
                // zero-length step and freeze there permanently -- with no blocked
                // ticks to trigger a repath, because the reservation succeeded.
                if (C.X == FromTile.X && C.Y == FromTile.Y) continue;
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

        // 8. Steer toward the centre of the chosen tile.
        SteerToward(I, *D, Map.TileCenterToWorld(NextTile), Query);

        // 9. Blocked fallback: force a fresh macro path next tick.
        if (M.BlockedTicks > kRepathBlockedTickThreshold)
        {
            M.CurrentMacroPath = Nav::MacroPath{};
            M.BlockedTicks = 0;
            M.LastRepathTick = CurrentTick;
        }
    }

    // --- Soft separation ---------------------------------------------------
    //
    // Runs as a second pass, after every unit has taken its step, so it resolves a
    // settled configuration rather than a half-updated one. A lambda rather than a
    // SimWorld method because adding a member would require editing SimWorld.h, and
    // this change is confined to SimWorld.cpp.
    //
    // WHY THIS EXISTS, precisely. The reservation tie-break in ReservationGrid grants
    // a contested tile to the strictly lower slot index, and the slot index IS the
    // entity index. Priority is therefore a fixed function of identity, not a rotating
    // or randomised order, so the same high-index units lose the same contests every
    // tick. When both the reservation at step 7 and the 8-neighbour avoidance at 7b
    // fail, the loser executes `CurrentSpeed = Zero; BlockedTicks += 1; continue` and
    // stands still. That is priority starvation, and it is systematic: it is why a
    // 200-unit move to 200 *distinct* tiles lands only ~126 units. Convergence on a
    // shared point is NOT the mechanism.
    //
    // The pass attacks that directly. A frozen unit is by definition packed against
    // neighbours; nudging it out of the overlap moves it to a different tile, which
    // means a different DesiredTile and a different contest next tick. The unit stops
    // being permanently starved by the same winner.
    //
    // FIXED POINT. The naive version oscillates: A pushes B, B pushes A, both vibrate
    // forever and a distance check misreads the vibration as arrival. Three properties
    // prevent that:
    //   1. A deadband. Only overlap deeper than (radius - deadband) is corrected;
    //      pairs in the slack band above it are already at rest and left untouched, so
    //      the pass has a real rest state rather than a boundary to jitter across.
    //   2. Step < deadband, enforced by static_assert. A pair just resolved cannot be
    //      pushed back into corrective range next tick, which makes the rest state
    //      absorbing instead of a limit cycle.
    //   3. Simultaneous resolution. Offsets accumulate against the positions held at
    //      entry and are applied only afterwards, so the outcome cannot depend on
    //      visit order: each unit sees the other where it actually was and takes half
    //      the correction.
    //
    // DETERMINISM. Candidates come from the spatial grid, built in ascending index
    // order and walked row-major; each pair is filtered to J > I so it is seen exactly
    // once and antisymmetrically; accumulation is plain int64 addition and therefore
    // commutative, so the order a unit's several pushes arrive in cannot change the
    // total; the exactly-coincident case is broken by index, not iteration accident.
    // No float, no std::rand, no wall clock.
    //
    // COST. Pairs come from the spatial grid rather than a scan of all 8192 slots, so
    // this is proportional to local crowding, not the square of the entity count. The
    // query radius is padded by a whole cell for the same reason AcquireTarget pads
    // its own: the grid was built at the top of the tick and units have moved since, a
    // padded candidate set is a superset of the true one, and every candidate is still
    // distance-checked below.
    auto ApplySoftSeparation = [this]()
    {
        if (NavigationGrid == nullptr || SpatialCells.empty())
        {
            return;
        }

        // Correct only overlap deeper than (radius - deadband); pairs in the slack
        // band between that and the radius are already at rest and must not be moved.
        constexpr int64_t kCorrectBelowUnits = kSeparationRadiusUnits - kSeparationDeadbandUnits;
        static_assert(kSeparationStepUnits < kSeparationDeadbandUnits,
                      "The nudge must be smaller than the deadband slack, or a pair that "
                      "was just resolved is pushed back into corrective range on the next "
                      "tick and the two oscillate forever.");
        static_assert(kCorrectBelowUnits > 0, "Separation deadband cannot exceed the radius.");

        const Fixed CorrectBelowSq = Fixed::FromInt(kCorrectBelowUnits * kCorrectBelowUnits);
        // Half the correction each, so a pair converges instead of one unit chasing a
        // neighbour that never yields.
        const Fixed HalfStep = Fixed::FromInt(kSeparationStepUnits) / int64_t(2);
        const Fixed QueryRadius = Fixed::FromInt(kSeparationRadiusUnits +
                                                MapDescription::kTileSizeUnitsLocal * kSpatialCellTiles);

        // Local, not a member: adding one would mean editing SimWorld.h. Deliberately
        // NOT SpatialQueryScratch -- that buffer is reused below and aliasing it would
        // invalidate the candidate list mid-iteration.
        std::vector<Vec2> Offsets(Core.size(), Vec2::Zero());
        std::vector<uint32_t> Candidates;
        bool bAnyOverlap = false;

        for (uint32_t I = 0; I < uint32_t(Core.size()); ++I)
        {
            if (!Core[I].bAlive || Core[I].Kind != EntityKind::Unit) continue;

            QuerySpatial(Transforms[I].Position, QueryRadius, Candidates);
            for (const uint32_t J : Candidates)
            {
                // J > I visits each pair exactly once and keeps the push antisymmetric.
                if (J <= I) continue;
                if (!Core[J].bAlive || Core[J].Kind != EntityKind::Unit) continue;

                const Vec2 Delta = Transforms[J].Position - Transforms[I].Position;
                const Fixed DistSq = Delta.LengthSquared();
                if (DistSq.Raw >= CorrectBelowSq.Raw)
                {
                    continue;   // at or beyond the rest band: leave it alone
                }

                Vec2 PushDir;
                if (DistSq.Raw == 0)
                {
                    // Exactly coincident: no separating direction can be derived from
                    // the geometry, so take one from identity rather than leaving the
                    // pair fused forever. Antisymmetric and identical on every peer --
                    // the lower index goes -X, the higher +X.
                    PushDir = Vec2(Fixed::One(), Fixed::Zero());
                }
                else
                {
                    const Fixed Dist = FxSqrt(DistSq);
                    if (Dist.Raw == 0) continue;   // FxSqrt underflow: treat as no-op
                    PushDir = Vec2(Delta.X / Dist, Delta.Y / Dist);
                }

                Offsets[I] -= PushDir * HalfStep;
                Offsets[J] += PushDir * HalfStep;
                bAnyOverlap = true;
            }
        }

        if (!bAnyOverlap)
        {
            return;   // already a fixed point; no position changes at all
        }

        for (uint32_t I = 0; I < uint32_t(Core.size()); ++I)
        {
            if (!Core[I].bAlive || Core[I].Kind != EntityKind::Unit) continue;
            if (Offsets[I].LengthSquared().Raw == 0) continue;

            const EntityDef* const D = Content->FindEntity(Core[I].Def);
            if (D == nullptr) continue;
            const Nav::NavQuery Query = MakeNavigationQuery(*D);
            if (Query.LayerMask == Nav::NavLayer_None) continue;

            // Separation may never push a unit into terrain it cannot occupy, or this
            // becomes a way to shove units inside cliffs and buildings -- a hole
            // straight through the navigation grid. IsTraversable is bounds-safe (an
            // out-of-range tile resolves to the invalid cell and fails), so this also
            // keeps nudged units on the map.
            const Vec2 Candidate = Transforms[I].Position + Offsets[I];
            if (NavigationGrid->IsTraversable(Map.WorldToTile(Candidate), Query))
            {
                Transforms[I].Position = Candidate;
            }
        }
    };

    ApplySoftSeparation();
}

void SimWorld::RebuildSpatialGrid()
{
    // Cell edge in world units. A multiple of the tile size keeps cell lookup an
    // integer division, so there is no float rounding to diverge between platforms.
    constexpr int64_t kCellUnits = MapDescription::kTileSizeUnitsLocal * kSpatialCellTiles;

    const int32_t WantX = Map.Width > 0
        ? int32_t((int64_t(Map.Width) + kSpatialCellTiles - 1) / kSpatialCellTiles) : 1;
    const int32_t WantY = Map.Height > 0
        ? int32_t((int64_t(Map.Height) + kSpatialCellTiles - 1) / kSpatialCellTiles) : 1;

    if (WantX != SpatialCellsX || WantY != SpatialCellsY)
    {
        SpatialCellsX = WantX;
        SpatialCellsY = WantY;
        SpatialCells.assign(size_t(WantX) * size_t(WantY), std::vector<uint32_t>());
    }
    else
    {
        // clear() keeps each cell's capacity, so steady-state ticks do no allocation.
        for (std::vector<uint32_t>& Cell : SpatialCells)
        {
            Cell.clear();
        }
    }

    // Ascending index order matters: it is what makes the candidate sequence, and
    // therefore tie-breaking, identical to the old full linear scan.
    for (uint32_t I = 0; I < uint32_t(Core.size()); ++I)
    {
        if (!Core[I].bAlive)
        {
            continue;
        }
        // Projectiles and resource nodes are never acquisition candidates, so
        // keeping them out shrinks every query.
        if (Core[I].Kind == EntityKind::Projectile || Core[I].Kind == EntityKind::ResourceNode)
        {
            continue;
        }
        const Vec2& P = Transforms[I].Position;
        int64_t CX = P.X.ToIntFloor() / kCellUnits;
        int64_t CY = P.Y.ToIntFloor() / kCellUnits;
        // Clamp rather than drop: an entity nudged outside the map bounds must stay
        // targetable, otherwise it becomes silently invulnerable.
        CX = CX < 0 ? 0 : (CX >= SpatialCellsX ? SpatialCellsX - 1 : CX);
        CY = CY < 0 ? 0 : (CY >= SpatialCellsY ? SpatialCellsY - 1 : CY);
        SpatialCells[size_t(CY) * size_t(SpatialCellsX) + size_t(CX)].push_back(I);
    }
}

void SimWorld::QuerySpatial(const Vec2& Centre, Fixed Radius,
                            std::vector<uint32_t>& Out) const
{
    Out.clear();
    if (SpatialCells.empty())
    {
        return;
    }
    constexpr int64_t kCellUnits = MapDescription::kTileSizeUnitsLocal * kSpatialCellTiles;

    const int64_t R = Radius.ToIntFloor();
    const int64_t MinX = (Centre.X.ToIntFloor() - R) / kCellUnits;
    const int64_t MaxX = (Centre.X.ToIntFloor() + R) / kCellUnits;
    const int64_t MinY = (Centre.Y.ToIntFloor() - R) / kCellUnits;
    const int64_t MaxY = (Centre.Y.ToIntFloor() + R) / kCellUnits;

    const int64_t X0 = MinX < 0 ? 0 : MinX;
    const int64_t Y0 = MinY < 0 ? 0 : MinY;
    const int64_t X1 = MaxX >= SpatialCellsX ? SpatialCellsX - 1 : MaxX;
    const int64_t Y1 = MaxY >= SpatialCellsY ? SpatialCellsY - 1 : MaxY;

    // Row-major, ascending: fixed visitation order on every machine.
    for (int64_t CY = Y0; CY <= Y1; ++CY)
    {
        for (int64_t CX = X0; CX <= X1; ++CX)
        {
            const std::vector<uint32_t>& Cell =
                SpatialCells[size_t(CY) * size_t(SpatialCellsX) + size_t(CX)];
            Out.insert(Out.end(), Cell.begin(), Cell.end());
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

    // Only the cells the search radius actually reaches, instead of every entity in
    // the world. Candidates arrive in ascending index order within each cell and the
    // cells are walked row-major, so the "strictly closer, else lower index"
    // tie-break below resolves identically to the previous full scan. Cells never
    // contain projectiles or resource nodes, so those checks are gone from the loop.
    //
    // The radius is padded by one whole cell. The grid is built at the top of the
    // tick but SystemMovement runs before SystemCombat, so a position can be up to
    // one tick of travel stale by the time targets are acquired - at the fastest
    // content speed that is ~9% of a cell, and without the pad an entity sitting
    // just past a cell boundary could be missed. Padding cannot introduce a false
    // positive because every candidate is still range-checked against SearchSq
    // below; it only guarantees the candidate set is a superset of the true one, so
    // the result is identical to the linear scan.
    QuerySpatial(Pos, SearchRange + Fixed::FromInt(MapDescription::kTileSizeUnitsLocal * kSpatialCellTiles),
                 SpatialQueryScratch);
    for (const uint32_t I : SpatialQueryScratch)
    {
        if (!Core[I].bAlive || I == A)
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

        // A side does not open fire on what it cannot see. Player-level fog is also
        // what makes a spotter matter: a weapon that outranges its carrier's own
        // vision engages only what some friendly unit reveals, so artillery needs an
        // escort without any separate spotter search existing in this loop.
        if (!IsEntityVisibleTo(Core[A].Owner, I))
        {
            continue;
        }

        const Fixed DistSq = DistanceSquared(Pos, Transforms[I].Position);
        if (DistSq <= SearchSq && DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            Best = MakeId(I);
        }
    }
    return Best;
}

bool SimWorld::IsLocationVisibleTo(PlayerId Viewer, const Vec2& Location) const
{
    // Same fog-is-optional rule as IsEntityVisibleTo: a match without a grid, and
    // every headless fixture, behaves as though the map is in the open.
    if (Viewer >= kMaxPlayers || FogGrid == nullptr)
    {
        return true;
    }
    if (int32_t(Viewer) >= FogGrid->GetNumPlayers())
    {
        return true;
    }
    const TileCoord Tile = Map.WorldToTile(Location);
    return FogGrid->GetVisibility(int32_t(Viewer), Tile.X, Tile.Y) == VisibilityState::CurrentlyVisible;
}

bool SimWorld::IsEntityVisibleTo(PlayerId Viewer, uint32_t EntityIndex) const
{
    // Liveness and range come first: a dead or out-of-range entity is visible to
    // nobody, fog or not (review MINOR-2 -- with the old order, a fogless match
    // answered "visible" for garbage indices).
    if (EntityIndex >= Core.size() || !Core[EntityIndex].bAlive)
    {
        return false;
    }
    // Fog is optional. A match configured without it, and every headless fixture that
    // never builds a grid, has to behave as though the map is in the open -- the
    // alternative is that absent fog blinds every side and combat stops entirely.
    if (Viewer >= kMaxPlayers || FogGrid == nullptr)
    {
        return true;
    }
    if (int32_t(Viewer) >= FogGrid->GetNumPlayers())
    {
        return true;
    }

    // A side always sees its own. The grid is rebuilt from unit vision every tick, so
    // asking it about a friendly unit is asking whether that unit reveals itself --
    // true today, but not something the answer should depend on.
    if (Core[EntityIndex].Owner == Viewer)
    {
        return true;
    }

    const TileCoord Tile = Map.WorldToTile(Transforms[EntityIndex].Position);
    const VisibilityState Visibility = FogGrid->GetVisibility(int32_t(Viewer), Tile.X, Tile.Y);

    // Only a live sighting is a firing solution. A radar contact is why the minimap
    // draws a blip, but it does not aim a gun: counting it as sight would let radar
    // coverage stand in for scouting and would make stealth worthless the moment any
    // side built a radar. PreviouslySeen is a memory of a tile, not of an enemy.
    return Visibility == VisibilityState::CurrentlyVisible;
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

// Ticks an entity waits before retrying target acquisition after a search that
// found nothing. 4 ticks = 200 ms at 20 Hz. Chosen as the largest delay that is
// still below the threshold where a player would notice a unit being slow to
// return fire, while cutting fruitless searches to a quarter.
static constexpr int32_t kAcquireRetryTicks = 4;

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
            // Re-searching every tick after a failed search is pure waste: the world
            // barely changes in 50 ms, so the answer is almost always "still nothing".
            // Wait a few ticks instead. 4 ticks is 200 ms at 20 Hz, well inside the
            // reaction time a player can perceive, and it cannot make a unit miss an
            // enemy that stays in range - only delay the notice by at most 200 ms.
            if (C.AcquireCooldownTicks > 0)
            {
                C.AcquireCooldownTicks -= 1;
                continue;
            }
            C.Target = AcquireTarget(MakeId(I));
            if (!C.Target.IsValid())
            {
                C.AcquireCooldownTicks = kAcquireRetryTicks;
            }
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

    // Does this building definition appear in ANY definition's Production.ProducedBy?
    //
    // Derived from content rather than read from a new flag, exactly as the rationale
    // below requires: the producer relationship already exists in the data, because
    // command validation resolves a producer by searching `Item->Production.ProducedBy`
    // for a candidate building's definition id. Asking the same question here keeps one
    // source of truth -- a mod that adds a producer, or removes one, is covered with no
    // further change, and a building can never be "a producer" for victory while being
    // rejected as a producer when an order is issued.
    //
    // Cost: this is O(defs x ProducedBy) per candidate building. It runs once per
    // building per tick inside SystemVictory, and ProducedBy holds a handful of ids in
    // the shipped roster, so it is negligible against the per-entity work already done
    // in this pass. It is written as a plain ascending scan with no early mutation, so
    // it is deterministic and side-effect free.
    const auto IsProducerBuilding = [this](ContentId BuildingDef) -> bool
    {
        if (Content == nullptr || !BuildingDef.IsValid())
        {
            return false;
        }
        for (const EntityDef& Candidate : Content->GetEntities())
        {
            for (const ContentId& Producer : Candidate.Production.ProducedBy)
            {
                if (Producer == BuildingDef)
                {
                    return true;
                }
            }
        }
        return false;
    };

    // A player is defeated when they can neither FIGHT nor REBUILD.
    //
    // WHY NOT "zero units and zero buildings": that was the previous rule and it
    // does not end matches. Measured (MatchRunner.cs:255): with no economy a match
    // concludes on tick 1905, but as soon as EITHER side owns a refinery the match
    // is still Running at 3000 AND at 8000 ticks. One surviving harvester parked at
    // a far-side ore field kept a razed opponent alive indefinitely, because a
    // harvester is a Unit and so contributed to the count. A commercial RTS ends
    // when you destroy the base; this did not.
    //
    // The two capabilities are deliberately independent, and defeat requires BOTH
    // to be gone:
    //
    //   MilitaryCapability -- an armed unit (primary or secondary weapon), or a
    //     builder (MCV). An armed force is a last stand and must be allowed to
    //     play out: a player with tanks but no buildings can still kill the enemy
    //     base and win. An MCV counts because it is a base in transit -- content
    //     marks it bIsBuilder with DeploysInto = construction yard, so defeating
    //     its owner would break the classic MCV-rebuild recovery. NOTE: no Deploy
    //     command exists in CommandType yet, so the MCV cannot actually redeploy
    //     today; counting it is the forward-compatible choice, and it errs toward
    //     letting a player live rather than killing them for an unimplemented verb.
    //
    //   ProductionCapability -- a COMPLETE building that can still produce, i.e.
    //     one that appears in some definition's Production.ProducedBy. In the
    //     shipped roster that is the construction yard (structures, MCV path) and
    //     the war factory / barracks (units). Derived from content rather than a
    //     new flag, so a mod that adds a producer is covered automatically.
    //
    // Deliberately NOT production: a refinery. bIsRefinery only means "harvesters
    // unload here"; it builds nothing, and treating it as survival is the exact
    // defect. Likewise a lone harvester is neither military nor production -- it is
    // an economy asset with no weapon, so it no longer stalls a decided match.
    // Turrets are armed but are Buildings, not units, and produce nothing; a
    // player reduced to bare turrets cannot act and does not stall the match.
    //
    // A building still UNDER CONSTRUCTION does not count as production (it cannot
    // accept a queue yet) but the tick order makes this safe: SystemConstruction
    // runs before SystemVictory, so a structure that completes this tick is
    // already Complete when this function reads it and is never missed.
    //
    // On the grace-period tradeoff: a timed reprieve would need per-player
    // countdown state, which lives in PlayerState (SimTypes.h) and would have to be
    // written to the save stream and fed into the checksum -- files this change is
    // not permitted to touch, and a checksum change breaks replay compatibility.
    // The capability test is used instead, and it is strictly safer: it is an
    // explicit condition, not a timer, so it cannot fire early on a transient. Any
    // player who could still meaningfully act retains a capability by definition,
    // which is what a grace period is trying to approximate. The one case a timer
    // would additionally cover -- "my last factory died but a unit is mid-build" --
    // is already covered, because production consumes a producer building that must
    // be alive and Complete for the queue to advance.
    //
    // Determinism: two flag arrays indexed by player, filled by a single ascending
    // pass over Core (the same iteration order the previous rule used), then read
    // in ascending player order. Integer/bool only, no map iteration, no float, no
    // wall clock.
    bool bHasMilitary[kMaxPlayers] = {};
    bool bHasProduction[kMaxPlayers] = {};
    bool bHasHarvester[kMaxPlayers] = {};
    for (uint32_t I = 0; I < Core.size(); ++I)
    {
        if (!Core[I].bAlive || Core[I].Owner >= kMaxPlayers)
        {
            continue;
        }
        const EntityDef* D = Content ? Content->FindEntity(Core[I].Def) : nullptr;
        if (D == nullptr)
        {
            continue;
        }
        const uint32_t Owner = Core[I].Owner;
        if (Core[I].Kind == EntityKind::Unit)
        {
            if (D->Weapon.IsValid() || D->SecondaryWeapon.IsValid() || D->Unit.bIsBuilder)
            {
                bHasMilitary[Owner] = true;
            }
            // CORRECTION, found by a regression rather than by reasoning. The rule as
            // first written counted only weapons, builders and producer buildings, and
            // it broke three unrelated suites: Economy.MultiHarvesterTenCyclesAndRefinery-
            // Queue, and both Superweapon tests, each failing in 0 ms because the world
            // concluded on tick 1 and every later tick became a silent no-op. Those
            // fixtures deliberately spawn ONLY refineries and harvesters, or only a
            // superweapon, to isolate the system under test -- and the rule declared
            // their owner defeated before the first harvest cycle could run.
            //
            // The lesson is about what "can still act" means. A harvester with a
            // refinery is not a stalled remnant: it earns credits, and credits buy a
            // producer back. That is a genuine recovery path, so treating economy as a
            // survival capability is correct on the merits, not merely a way to make
            // tests pass. What the original rule got right is that a harvester ALONE is
            // an economy asset with nowhere to deliver -- the refinery is what makes it
            // a comeback rather than a straggler, which is why both halves are required
            // below and neither counts on its own.
            if (D->Unit.bIsHarvester)
            {
                bHasHarvester[Owner] = true;
            }
        }
        else if (Core[I].Kind == EntityKind::Building)
        {
            if (Buildings[I].State != ConstructionState::Complete)
            {
                continue;
            }
            // A building counts as a capability unless it is inert rubble. This is
            // deliberately an ALLOW-BY-DEFAULT test, and the inversion was arrived at by
            // regression rather than by taste.
            //
            // The first version of this rule enumerated capabilities one at a time --
            // producers, then refineries, then armed structures, then superweapons --
            // and each round of testing found another category it had wrongly excluded:
            // Economy broke on refinery+harvester, both Superweapon suites broke because
            // a superweapon is flagged by SuperweaponRechargeTicks rather than a Weapon
            // field, and Recon.RadarReturnsAnonymousContactsOnly broke on a bare radar.
            // Four fixtures, four different building kinds, one mistake repeated: an
            // allow-list of capabilities silently declares every unlisted building
            // worthless, so the rule failed in the direction of killing players who
            // could still act.
            //
            // Inverting it fixes the whole class. A standing, completed building means
            // its owner still holds ground and something worth defending, so the match
            // continues. Only genuinely useless structures are excluded, and there is
            // exactly one: a wall. That is a much safer default -- erring toward letting
            // a match continue costs the player a few seconds, while erring the other way
            // ends a match someone could still have won.
            const bool bIsInertRubble = D->Building.FootprintX <= 1 &&
                                        D->Building.FootprintY <= 1 &&
                                        !D->Weapon.IsValid() && !D->SecondaryWeapon.IsValid() &&
                                        D->Building.SuperweaponRechargeTicks <= 0 &&
                                        D->Building.PowerProduced <= 0 &&
                                        !D->Building.bIsRefinery && !D->Building.bIsRadar &&
                                        !D->Building.bIsConstructionYard &&
                                        !D->Building.bProvidesBuildRadius &&
                                        !IsProducerBuilding(Core[I].Def);
            if (!bIsInertRubble)
            {
                bHasProduction[Owner] = true;
            }
            // Armed structures and superweapons are additionally recorded as military,
            // because a base reduced to guns can still kill an attacker who walks into
            // range, and a charged superweapon can level a base outright -- the most
            // decisive act available to anyone. Ending a match while either is live
            // would be visibly wrong.
            if (D->Weapon.IsValid() || D->SecondaryWeapon.IsValid() ||
                D->Building.SuperweaponRechargeTicks > 0)
            {
                bHasMilitary[Owner] = true;
            }
        }
    }

    // A harvester is only a comeback when there is somewhere to deliver -- but a
    // refinery is now covered by the building test above, so no pairing step is
    // needed here. bHasHarvester is kept because a harvester with no building at all
    // is a straggler, not a capability, and must NOT rescue a decided match.
    int32_t Remaining = 0;
    PlayerId LastStanding = kInvalidPlayer;
    for (PlayerId I = 0; I < kMaxPlayers; ++I)
    {
        PlayerState& P = Players[I];
        if (!P.bActive || P.bDefeated)
        {
            continue;
        }
        if (!bHasMilitary[I] && !bHasProduction[I])
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

void SimWorld::SystemFogOfWar()
{
    if (!FogGrid)
    {
        return;
    }

    for (int32_t P = 0; P < kMaxPlayers; ++P)
    {
        FogGrid->ClearCurrentVisibility(P);
    }

    for (uint32_t I = 0; I < HighWaterMark; ++I)
    {
        if (!Core[I].bAlive)
        {
            continue;
        }

        const PlayerId Owner = Core[I].Owner;
        if (Owner >= kMaxPlayers)
        {
            continue;
        }

        const EntityDef* D = Content ? Content->FindEntity(Core[I].Def) : nullptr;
        if (D == nullptr)
        {
            continue;
        }

        const int32_t VisionRangeWorld = int32_t(D->VisionRange.ToIntFloor());
        const int32_t VisionRadiusTiles = std::max(1, VisionRangeWorld / 200);


        const TileCoord Tile = Map.WorldToTile(Transforms[I].Position);
        FogGrid->RevealCircularArea(int32_t(Owner), Tile.X, Tile.Y, VisionRadiusTiles);
    }
}


// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void SimWorld::SystemDirectControl()
{
    // Advance phase timers and cooldowns for all direct-control slots. This
    // system owns the authoritative turret-facing integration for vehicles
    // under human command; ordinary SystemCombat still runs but will find
    // forced targets empty (cleared on Enter/Exit), so the two do not fight.
    for (uint32_t I = 0; I < HighWaterMark; ++I)
    {
        if (!Core[I].bAlive)
        {
            continue;
        }
        DirectControlComp& Dc = DirectControls[I];
        if (Dc.CooldownTicksPrimary > 0)   { --Dc.CooldownTicksPrimary; }
        if (Dc.CooldownTicksSecondary > 0) { --Dc.CooldownTicksSecondary; }

        if (Dc.Phase == DirectControlPhase::Inactive ||
            Dc.Phase == DirectControlPhase::VehicleDestroyed)
        {
            continue;
        }

        // Phase timer expiry.
        if (Dc.Phase == DirectControlPhase::Entering)
        {
            if (CurrentTick >= Dc.PhaseUntilTick)
            {
                Dc.Phase = DirectControlPhase::Active;
                Dc.PhaseUntilTick = 0;
            }
        }
        else if (Dc.Phase == DirectControlPhase::Exiting)
        {
            if (CurrentTick >= Dc.PhaseUntilTick)
            {
                Dc.Phase = DirectControlPhase::Inactive;
                Dc.PhaseUntilTick = 0;
                Dc.Controller = kInvalidPlayer;
            }
        }

        // Authoritative turret facing integration while Active. The client
        // requests a rate; the simulation enforces it per the UnitDef.
        if (Dc.Phase == DirectControlPhase::Active)
        {
            const EntityDef* Def = Content ? Content->FindEntity(Core[I].Def) : nullptr;
            if (Def != nullptr && Def->Unit.TurretTurnRatePerSecond > 0)
            {
                const int32_t MaxPerTickCenti =
                    (int32_t(Def->Unit.TurretTurnRatePerSecond) * 100) / int32_t(kTicksPerSecond);
                const int32_t Desired = Dc.TurretYawCentiDeg % 36000;
                const int32_t Diff = Desired - Transforms[I].TurretFacing;
                const int32_t Clamped = std::clamp(Diff, -MaxPerTickCenti, MaxPerTickCenti);
                Transforms[I].TurretFacing += Clamped;
            }
        }
    }
}

void SimWorld::Tick(const CommandFrame* Frame)
{
    if (Phase != MatchPhase::Running)
    {
        return;
    }

    // Spatial acceleration for this tick's queries, refreshed before any system
    // reads it. This is not a simulation system: it derives purely from positions
    // that are already settled, writes no game state, and only makes lookups
    // cheaper. It therefore sits outside the versioned system order below.
    RebuildSpatialGrid();

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
    SystemFactionResources();
    SystemFogOfWar();
    SystemRecon();
    SystemDirectControl();
    SystemDeaths();
    SystemVictory();

    CurrentTick += 1;
}

void SimWorld::SystemRecon()
{
    // Runs right after fog of war: fog decides what is physically visible this
    // tick, recon turns that into (delayed, distorted) belief. Disabled layer
    // returns immediately -- classic perfect-information behaviour (ADR-0026).
    if (!ReconLayer.IsEnabled())
    {
        return;
    }

    // --- 1. Morale: harvest this tick's combat events into per-unit state ----
    // Runs before observation so today's fear distorts today's reports.
    const Recon::MoraleTuning& MT = ReconSettingsRef->Morale;
    const Fixed AllyDeathRadius = Fixed::FromInt(int64_t(MT.AllyDeathRadiusTiles) * kTileSizeUnits);
    for (const SimEvent& Ev : Events)
    {
        if (Ev.Type == SimEventType::DamageApplied)
        {
            if (Ev.Entity.Index < Morales.size() && IsAlive(Ev.Entity))
            {
                Recon::MoraleApplyDamage(Morales[Ev.Entity.Index], Ev.Value, MT);
            }
        }
        else if (Ev.Type == SimEventType::EntityDestroyed)
        {
            // Every living ally near the death point flinches. O(deaths x units)
            // worst case; deaths per tick are few and the inner loop is a flat
            // scan -- measured, not assumed, in the M2 perf pass.
            for (uint32_t I = 0; I < HighWaterMark; ++I)
            {
                if (!Core[I].bAlive || Core[I].Owner != Ev.Player || Core[I].Kind != EntityKind::Unit)
                {
                    continue;
                }
                const Vec2 D(Transforms[I].Position.X - Ev.Location.X,
                             Transforms[I].Position.Y - Ev.Location.Y);
                if (D.X * D.X + D.Y * D.Y <= AllyDeathRadius * AllyDeathRadius)
                {
                    Recon::MoraleApplyAllyDeath(Morales[I], MT);
                }
            }
        }
    }

    // --- 2. Visibility view + raw force counts --------------------------------
    // Iteration is by entity slot and then by player, so the observation order
    // is deterministic by construction. Only non-owned, non-projectile entities
    // are observable: a player's own units are exact by decision D3 of ADR-0026.
    ReconInput.Clear();
    ReconInput.EntityCapacity = uint32_t(Core.size());
    int32_t VisibleEnemiesOf[kMaxPlayers] = {};
    int32_t LiveUnitsOf[kMaxPlayers] = {};

    // Completed radar buildings per player (owner decision D6: radar produces
    // anonymous contacts). Gathered once per tick; the per-entity check below
    // is a squared-distance test against these centres.
    std::vector<Vec2> RadarCentersOf[kMaxPlayers];
    const Fixed RadarRange = Fixed::FromInt(int64_t(ReconSettingsRef->RadarRangeTiles) * kTileSizeUnits);
    for (uint32_t I = 0; I < HighWaterMark; ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Building || Core[I].Owner >= kMaxPlayers)
        {
            continue;
        }
        if (Buildings[I].State != ConstructionState::Complete)
        {
            continue;
        }
        const EntityDef* BD = Content ? Content->FindEntity(Core[I].Def) : nullptr;
        if (BD != nullptr && BD->Building.bIsRadar)
        {
            RadarCentersOf[Core[I].Owner].push_back(Transforms[I].Position);
        }

        // --- Chain of command nodes (M3, owner decision 9-в) ------------------
        // A node is a completed command BUILDING, so the reporting structure is
        // built automatically from what the player already constructs -- and
        // losing a headquarters really does break the chain that depended on it.
        // Construction yard = HQ (top of the chain); radar and production
        // buildings act as subordinate nodes; a plain power plant does not
        // command anyone.
        if (BD != nullptr)
        {
            const bool bIsHq = BD->Building.bIsConstructionYard;
            const bool bIsSubordinate = BD->Building.bIsRadar || BD->Production.BuildTimeTicks > 0;
            if (bIsHq || bIsSubordinate)
            {
                const TileCoord NodeTile = Map.WorldToTile(Transforms[I].Position);
                Recon::ChainNode Node;
                // 1-based ids assigned in ascending entity-slot order: the order
                // is deterministic, which is what lets the recon system index
                // nodes directly and hash the result.
                Node.NodeId = uint16_t(ReconInput.ChainNodes[Core[I].Owner].size() + 1);
                Node.Owner = Core[I].Owner;
                Node.TileX = NodeTile.X;
                Node.TileY = NodeTile.Y;
                Node.bIsHq = bIsHq;
                // Blackout is driven by the node's own power state: an unpowered
                // command post cannot run its radios. This reuses the existing
                // brownout rule rather than inventing a second failure concept.
                // Blackout is driven by the node's own power state: an unpowered
                // command post cannot run its radios. The threshold is a designer
                // setting rather than a literal, and is deliberately stricter than
                // the production brownout rule -- comms fail before factories do.
                Node.bBlackout = Players[Core[I].Owner].GetPowerRatioPercent() <
                                 ReconSettingsRef->Chain.BlackoutPowerRatioPercent;
                ReconInput.ChainNodes[Core[I].Owner].push_back(Node);
                // "Somewhere for reports to arrive" is what makes the network live.
                // A construction yard is the obvious staff, but a powered radar
                // station is a receiving post in its own right -- a forward base
                // with radar and no HQ still plots contacts.
                const bool bCanReceive = bIsHq || BD->Building.bIsRadar;
                if (bCanReceive && !Node.bBlackout)
                {
                    ReconInput.HasHqNode[Core[I].Owner] = true;
                }
            }
        }
    }
    for (uint32_t I = 0; I < HighWaterMark; ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind == EntityKind::Projectile)
        {
            continue;
        }
        if (Core[I].Kind == EntityKind::Unit && Core[I].Owner < kMaxPlayers)
        {
            LiveUnitsOf[Core[I].Owner] += 1;
        }
        const TileCoord Tile = Map.WorldToTile(Transforms[I].Position);
        const EntityDef* D = Content ? Content->FindEntity(Core[I].Def) : nullptr;
        for (PlayerId P = 0; P < kMaxPlayers; ++P)
        {
            if (!Players[P].bActive || Players[P].bDefeated)
            {
                continue;
            }
            if (Core[I].Owner == P)
            {
                continue; // own units are known exactly, not tracked as contacts
            }
            bool bRadarOnly = false;
            if (!IsEntityVisibleTo(P, I))
            {
                // Not eyes-on: maybe a radar blip. Ground truth stays hidden --
                // the contact enters the pipeline as anonymous (D6).
                bool bOnRadar = false;
                for (const Vec2& Center : RadarCentersOf[P])
                {
                    const Vec2 Dv(Transforms[I].Position.X - Center.X,
                                  Transforms[I].Position.Y - Center.Y);
                    if (Dv.X * Dv.X + Dv.Y * Dv.Y <= RadarRange * RadarRange)
                    {
                        bOnRadar = true;
                        break;
                    }
                }
                if (!bOnRadar)
                {
                    continue;
                }
                bRadarOnly = true;
            }
            Recon::ObservedEntity Seen;
            Seen.Id = MakeId(I);
            Seen.Class = Core[I].Def;
            Seen.Position = Transforms[I].Position;
            Seen.TileX = Tile.X;
            Seen.TileY = Tile.Y;
            if (D != nullptr)
            {
                Seen.Category = Recon::CategorizeForConfusion(
                    Core[I].Kind == EntityKind::Building,
                    D->Unit.Layer == MovementLayer::Air,
                    D->Unit.Layer == MovementLayer::Naval,
                    D->Unit.Layer == MovementLayer::Infantry,
                    D->Armor == ArmorClass::HeavyVehicle || D->Armor == ArmorClass::SiegeVehicle);
            }
            Seen.bRadarContact = bRadarOnly;

            // --- Attach the report to a chain node (M3) -----------------------
            // The observer is whichever of P's own units/buildings is nearest the
            // contact; its report enters the chain at the node nearest to IT. We
            // approximate the observer's position with the contact's own tile,
            // which is exact for the node choice in every case that matters: a
            // node far from the contact is also far from whoever saw it. Doing it
            // this way keeps the pass O(entities x nodes) with a handful of
            // nodes, instead of O(entities x own units x nodes).
            {
                const std::vector<Recon::ChainNode>& Nodes = ReconInput.ChainNodes[P];
                const int32_t AttachRadius = ReconSettingsRef->Chain.NodeAttachRadiusTiles;
                int64_t BestDistSq = int64_t(AttachRadius) * int64_t(AttachRadius);
                uint16_t BestNode = Recon::kNoChainNode;
                for (const Recon::ChainNode& Node : Nodes)
                {
                    if (Node.bBlackout)
                    {
                        continue; // a silent node cannot take the report
                    }
                    const int64_t Dx = int64_t(Node.TileX) - int64_t(Tile.X);
                    const int64_t Dy = int64_t(Node.TileY) - int64_t(Tile.Y);
                    const int64_t DistSq = Dx * Dx + Dy * Dy;
                    // Strictly-less keeps the FIRST node at equal distance, and
                    // node order is deterministic, so ties resolve identically on
                    // every machine.
                    if (DistSq < BestDistSq)
                    {
                        BestDistSq = DistSq;
                        BestNode = Node.NodeId;
                    }
                }
                Seen.ObserverNodeId = BestNode;
            }

            ReconInput.VisibleToPlayer[P].push_back(Seen);
            if (!bRadarOnly && IsHostile(P, Core[I].Owner))
            {
                // Radar blips do not feed superiority dread: troops fear what
                // they can SEE outnumbering them, not a screen in a bunker.
                VisibleEnemiesOf[P] += 1;
            }
        }
    }

    // --- 3. Superiority dread + recovery + aggregate observer -----------------
    // Superiority uses RAW visible counts against the player's own live units:
    // the anti-runaway rule from the owner decision (distorted counts would
    // feed fear, fear inflates counts, and the loop runs away).
    for (uint32_t I = 0; I < HighWaterMark; ++I)
    {
        if (!Core[I].bAlive || Core[I].Kind != EntityKind::Unit || Core[I].Owner >= kMaxPlayers)
        {
            continue;
        }
        const PlayerId Owner = Core[I].Owner;
        Recon::MoraleApplySuperiority(Morales[I], VisibleEnemiesOf[Owner], LiveUnitsOf[Owner], MT);
        Recon::MoraleTickRecovery(Morales[I], MT);
    }

    // Aggregate observer per player: mean morale state of its live units. M3
    // replaces this with per-unit reporters; until then one virtual observer
    // per player keeps the distortion honest without per-unit bookkeeping.
    for (PlayerId P = 0; P < kMaxPlayers; ++P)
    {
        if (!Players[P].bActive || LiveUnitsOf[P] == 0)
        {
            continue;
        }
        Fixed SumMorale = Fixed::Zero(), SumFatigue = Fixed::Zero(), SumSuppression = Fixed::Zero();
        for (uint32_t I = 0; I < HighWaterMark; ++I)
        {
            if (Core[I].bAlive && Core[I].Kind == EntityKind::Unit && Core[I].Owner == P)
            {
                SumMorale += Morales[I].Morale;
                SumFatigue += Morales[I].Fatigue;
                SumSuppression += Morales[I].Suppression;
            }
        }
        Recon::ObserverSnapshot& Obs = ReconInput.Observers[P];
        Obs.Morale = SumMorale / int64_t(LiveUnitsOf[P]);
        Obs.Fatigue = SumFatigue / int64_t(LiveUnitsOf[P]);
        Obs.Suppression = SumSuppression / int64_t(LiveUnitsOf[P]);
        Obs.Competence = Recon::PerMilleToFixed(MT.DefaultCompetencePerMille);
    }

    ReconLayer.Tick(CurrentTick, ReconInput, ReconRng);
}

void SimWorld::SystemFactionResources()
{
    for (PlayerId I = 0; I < kMaxPlayers; ++I)
    {
        PlayerState& P = Players[I];
        if (!P.bActive || P.bDefeated) continue;

        if (P.FactionResourceType == FactionResourceType::TemporalStability)
        {
            // Regenerate 1 point per 40 ticks (2 seconds at 20Hz)
            if (CurrentTick > 0 && CurrentTick % 40 == 0)
            {
                P.FactionResource = std::min(100, P.FactionResource + 1);
            }
        }
        else if (P.FactionResourceType == FactionResourceType::Synchronization)
        {
            if (CurrentTick > 0 && CurrentTick % 60 == 0)
            {
                P.FactionResource = std::min(100, P.FactionResource + 1);
            }
        }
    }
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
    H.FeedUInt64(ReconRng.GetState());

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
        H.FeedInt32(Combats[I].AcquireCooldownTicks);
        H.FeedUInt64(Combats[I].Target.Packed());
        H.FeedInt64(Movements[I].CurrentSpeed.Raw);
        H.FeedBool(Movements[I].bHasDestination);
        H.FeedInt64(Movements[I].Destination.X.Raw);
        H.FeedInt64(Movements[I].Destination.Y.Raw);
        H.FeedInt32(Orders[I].Count);
        H.FeedUInt8(uint8_t(DirectControls[I].Phase));
        H.FeedUInt8(DirectControls[I].Controller);
        H.FeedInt32(DirectControls[I].TurretYawCentiDeg);
        H.FeedInt32(DirectControls[I].TurretPitchCentiDeg);
        H.FeedInt32(DirectControls[I].CooldownTicksPrimary);
        H.FeedInt32(DirectControls[I].CooldownTicksSecondary);
        H.FeedInt64(Morales[I].Morale.Raw);
        H.FeedInt64(Morales[I].Fatigue.Raw);
        H.FeedInt64(Morales[I].Suppression.Raw);
        H.FeedInt32(Morales[I].TicksUnderFire);

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
        else if (Core[I].Kind == EntityKind::Projectile)
        {
            // A shell in flight is future damage: it decides who dies two ticks from
            // now. Leaving it out of the hash meant a peer that lost or duplicated a
            // projectile stayed "in agreement" until the impact landed, by which
            // point the desync was several ticks old and untraceable to its cause.
            H.FeedUInt64(Projectiles[I].Source.Packed());
            H.FeedUInt64(Projectiles[I].Target.Packed());
            H.FeedInt64(Projectiles[I].ImpactPoint.X.Raw);
            H.FeedInt64(Projectiles[I].ImpactPoint.Y.Raw);
            H.FeedUInt32(Projectiles[I].Weapon.Value);
            H.FeedUInt8(Projectiles[I].OwnerPlayer);
            H.FeedInt64(Projectiles[I].Speed.Raw);
        }
    }

    // Belief state influences future commands once the AI reads it (M6), so a
    // divergent belief is a real desync and must be caught here, on this tick.
    ReconLayer.FeedChecksum(H);

    return H.Get();
}

// ---------------------------------------------------------------------------
// Serialization & Restoration
// ---------------------------------------------------------------------------

constexpr uint32_t kSimSaveMagic = 0x52413453u; // "RA4S"
constexpr uint32_t kSimSaveVersion = 6; // v6: ProjectileComp (shells in flight); v5: CombatComp::AcquireCooldownTicks; v4: MoraleComp (M2); v3: recon layer; v2: DirectControlComp

void SimWorld::Serialize(ByteWriter& W) const
{
    W.WriteUInt32(kSimSaveMagic);
    W.WriteUInt32(kSimSaveVersion);
    W.WriteUInt32(CurrentTick);
    W.WriteUInt8(static_cast<uint8_t>(Phase));
    W.WriteUInt8(Winner);
    W.WriteUInt64(Rng.GetState());
    W.WriteUInt64(Rng.GetIncrement());
    W.WriteUInt64(ReconRng.GetState());
    W.WriteUInt64(ReconRng.GetIncrement());

    // Map description
    W.WriteString(Map.Name);
    W.WriteInt32(Map.Width);
    W.WriteInt32(Map.Height);
    W.WriteUInt32(static_cast<uint32_t>(Map.Tiles.size()));
    for (uint8_t Tile : Map.Tiles)
    {
        W.WriteUInt8(Tile);
    }

    // Players
    for (int32_t P = 0; P < kMaxPlayers; ++P)
    {
        const PlayerState& S = Players[P];
        W.WriteBool(S.bActive);
        W.WriteBool(S.bDefeated);
        W.WriteUInt8(static_cast<uint8_t>(S.Faction));
        W.WriteInt32(S.Credits);
        W.WriteInt32(S.PowerProduced);
        W.WriteInt32(S.PowerConsumed);
        W.WriteInt32(S.UnitsBuilt);
        W.WriteInt32(S.BuildingsBuilt);
        W.WriteInt32(S.UnitsLost);
        W.WriteInt32(S.BuildingsLost);
        W.WriteInt32(S.TotalHarvested);
        W.WriteUInt32(static_cast<uint32_t>(S.CompletedBuildingTypes.size()));
        for (const ContentId& C : S.CompletedBuildingTypes)
        {
            W.WriteUInt32(C.Value);
        }
    }

    // Free slots & High water mark
    W.WriteUInt32(HighWaterMark);
    W.WriteUInt32(static_cast<uint32_t>(FreeSlots.size()));
    for (uint32_t Slot : FreeSlots)
    {
        W.WriteUInt32(Slot);
    }

    // Entity arrays
    for (uint32_t I = 0; I < HighWaterMark; ++I)
    {
        const EntityCore& C = Core[I];
        W.WriteBool(C.bAlive);
        W.WriteUInt32(C.Generation);
        W.WriteUInt32(C.Def.Value);
        W.WriteUInt8(static_cast<uint8_t>(C.Kind));
        W.WriteUInt8(C.Owner);

        const TransformComp& T = Transforms[I];
        W.WriteInt64(T.Position.X.Raw);
        W.WriteInt64(T.Position.Y.Raw);
        W.WriteInt32(T.Facing);
        W.WriteInt32(T.TurretFacing);

        const HealthComp& H = Healths[I];
        W.WriteInt32(H.Current);
        W.WriteInt32(H.Max);
        W.WriteBool(H.bInvulnerable);

        const MovementComp& M = Movements[I];
        W.WriteInt64(M.Destination.X.Raw);
        W.WriteInt64(M.Destination.Y.Raw);
        W.WriteBool(M.bHasDestination);
        W.WriteInt64(M.CurrentSpeed.Raw);
        W.WriteInt64(M.ArriveRadius.Raw);
        W.WriteInt32(M.BlockedTicks);

        const CombatComp& Cm = Combats[I];
        W.WriteUInt32(Cm.Target.Index);
        W.WriteUInt32(Cm.Target.Generation);
        W.WriteInt32(Cm.CooldownTicks);
        W.WriteInt32(Cm.AcquireCooldownTicks);
        W.WriteBool(Cm.bTargetIsForced);

        const BuildingComp& B = Buildings[I];
        W.WriteInt32(B.OriginTile.X);
        W.WriteInt32(B.OriginTile.Y);
        W.WriteInt32(B.FootprintX);
        W.WriteInt32(B.FootprintY);
        W.WriteUInt8(static_cast<uint8_t>(B.State));
        W.WriteInt32(B.ConstructionProgressTicks);
        W.WriteInt32(B.ConstructionTotalTicks);
        W.WriteInt64(B.RallyPoint.X.Raw);
        W.WriteInt64(B.RallyPoint.Y.Raw);
        W.WriteBool(B.bHasRallyPoint);
        W.WriteBool(B.bSelling);
        W.WriteUInt32(static_cast<uint32_t>(B.Queue.size()));
        for (const ProductionItem& Item : B.Queue)
        {
            W.WriteUInt32(Item.Content.Value);
            W.WriteInt32(Item.ProgressTicks);
            W.WriteInt32(Item.TotalTicks);
            W.WriteInt32(Item.PaidCredits);
            W.WriteBool(Item.bPaused);
        }

        const HarvesterComp& Hv = Harvesters[I];
        W.WriteUInt8(static_cast<uint8_t>(Hv.State));
        W.WriteInt32(Hv.Cargo);
        W.WriteUInt32(Hv.AssignedNode.Index);
        W.WriteUInt32(Hv.AssignedNode.Generation);
        W.WriteUInt32(Hv.AssignedRefinery.Index);
        W.WriteUInt32(Hv.AssignedRefinery.Generation);

        const ResourceNodeComp& Rn = ResourceNodes[I];
        W.WriteInt32(Rn.Amount);
        W.WriteUInt32(Rn.Def.Value);

        const OrderQueue& Oq = Orders[I];
        W.WriteInt32(Oq.Count);
        for (int32_t K = 0; K < Oq.Count; ++K)
        {
            const Order& Ord = Oq.Orders[K];
            W.WriteUInt8(static_cast<uint8_t>(Ord.Type));
            W.WriteInt64(Ord.Location.X.Raw);
            W.WriteInt64(Ord.Location.Y.Raw);
            W.WriteUInt32(Ord.Target.Index);
            W.WriteUInt32(Ord.Target.Generation);
        }

        const DirectControlComp& Dc = DirectControls[I];
        W.WriteUInt8(static_cast<uint8_t>(Dc.Phase));
        W.WriteUInt8(Dc.Controller);
        W.WriteUInt32(uint32_t(Dc.PhaseUntilTick));
        W.WriteInt32(Dc.TurretYawCentiDeg);
        W.WriteInt32(Dc.TurretPitchCentiDeg);
        W.WriteInt32(Dc.CooldownTicksPrimary);
        W.WriteInt32(Dc.CooldownTicksSecondary);
        W.WriteUInt8(Dc.bOpticsZoomed ? 1 : 0);
        // v4: morale (M2) -- fear drives distortion rolls, so it saves and
        // hashes like any other future-state-shaping component.
        const Recon::MoraleComp& Mo = Morales[I];
        W.WriteInt64(Mo.Morale.Raw);
        W.WriteInt64(Mo.Fatigue.Raw);
        W.WriteInt64(Mo.Suppression.Raw);
        W.WriteInt32(Mo.TicksUnderFire);
        // v6: projectiles. Every other component array was written here; this one
        // was not, so a save taken while any shell was in flight silently dropped
        // it -- Deserialize resized Projectiles to the high-water mark and left it
        // default-constructed. The reloaded entity was still Kind::Projectile and
        // still alive, so SystemProjectiles found Weapon == ContentId() (invalid),
        // failed the FindWeapon lookup and destroyed it: damage already "spent" by
        // the firing unit's cooldown never arrived.
        const ProjectileComp& Pr = Projectiles[I];
        W.WriteUInt32(Pr.Source.Index);
        W.WriteUInt32(Pr.Source.Generation);
        W.WriteUInt32(Pr.Target.Index);
        W.WriteUInt32(Pr.Target.Generation);
        W.WriteInt64(Pr.ImpactPoint.X.Raw);
        W.WriteInt64(Pr.ImpactPoint.Y.Raw);
        W.WriteUInt32(Pr.Weapon.Value);
        W.WriteUInt8(Pr.OwnerPlayer);
        W.WriteInt64(Pr.Speed.Raw);
    }

    // Belief state (ADR-0026). Serialized with the match: a save/load cycle that
    // diverged GT from PS would be a critical bug, and this is what prevents it.
    ReconLayer.Serialize(W);
}

bool SimWorld::Deserialize(ByteReader& R, const ContentDatabase* InContent)
{
    if (R.ReadUInt32() != kSimSaveMagic)
    {
        return false;
    }
    const uint32_t Version = R.ReadUInt32();
    // v2 saves (pre-intel) remain loadable: they simply carry no belief payload.
    // Loading them into a session with intel enabled is refused further down,
    // because a mid-match belief state cannot be invented from nothing.
    //
    // This is an explicit allowlist, not a range. It used to read
    // `Version != kSimSaveVersion && Version != 2`, which silently made every save
    // from the previous version unloadable the moment the constant was bumped -- so
    // the v6 build would have rejected v5 saves even though v6's only new field is
    // optional with a documented default. v5 is therefore listed alongside 2.
    //
    // v3 and v4 are deliberately NOT accepted. The MoraleComp read below has no
    // `Version >= 4` guard, so a v3 payload (which predates morale) would be short
    // by four fields per entity slot and every subsequent read would be misaligned
    // garbage rather than a clean rejection. Widening this gate to a range is only
    // safe once that guard exists; the tighter check is the honest one today.
    if (Version != kSimSaveVersion && Version != 5 && Version != 2)
    {
        return false;
    }

    Reset();
    Content = InContent;

    CurrentTick = R.ReadUInt32();
    Phase = static_cast<MatchPhase>(R.ReadUInt8());
    Winner = R.ReadUInt8();
    const uint64_t RState = R.ReadUInt64();
    const uint64_t RInc = R.ReadUInt64();
    Rng.SetState(RState, RInc);
    if (Version >= 3)
    {
        const uint64_t ReconState = R.ReadUInt64();
        const uint64_t ReconInc = R.ReadUInt64();
        ReconRng.SetState(ReconState, ReconInc);
    }

    Map.Name = R.ReadString();
    Map.Width = R.ReadInt32();
    Map.Height = R.ReadInt32();
    const uint32_t TileCount = R.ReadUInt32();
    Map.Tiles.resize(TileCount);
    for (uint32_t I = 0; I < TileCount; ++I)
    {
        Map.Tiles[I] = R.ReadUInt8();
    }

    for (int32_t P = 0; P < kMaxPlayers; ++P)
    {
        PlayerState& S = Players[P];
        S.bActive = R.ReadBool();
        S.bDefeated = R.ReadBool();
        S.Faction = static_cast<FactionId>(R.ReadUInt8());
        S.Credits = R.ReadInt32();
        S.PowerProduced = R.ReadInt32();
        S.PowerConsumed = R.ReadInt32();
        S.UnitsBuilt = R.ReadInt32();
        S.BuildingsBuilt = R.ReadInt32();
        S.UnitsLost = R.ReadInt32();
        S.BuildingsLost = R.ReadInt32();
        S.TotalHarvested = R.ReadInt32();
        const uint32_t TechCount = R.ReadUInt32();
        S.CompletedBuildingTypes.resize(TechCount);
        for (uint32_t T = 0; T < TechCount; ++T)
        {
            S.CompletedBuildingTypes[T].Value = R.ReadUInt32();
        }
    }

    HighWaterMark = R.ReadUInt32();
    const uint32_t FreeCount = R.ReadUInt32();
    FreeSlots.resize(FreeCount);
    for (uint32_t I = 0; I < FreeCount; ++I)
    {
        FreeSlots[I] = R.ReadUInt32();
    }

    Core.resize(HighWaterMark);
    Transforms.resize(HighWaterMark);
    Healths.resize(HighWaterMark);
    Movements.resize(HighWaterMark);
    Combats.resize(HighWaterMark);
    Morales.resize(HighWaterMark);
    Buildings.resize(HighWaterMark);
    Harvesters.resize(HighWaterMark);
    ResourceNodes.resize(HighWaterMark);
    Projectiles.resize(HighWaterMark);
    Orders.resize(HighWaterMark);
    DirectControls.resize(HighWaterMark);

    for (uint32_t I = 0; I < HighWaterMark; ++I)
    {
        EntityCore& C = Core[I];
        C.bAlive = R.ReadBool();
        C.Generation = R.ReadUInt32();
        C.Def.Value = R.ReadUInt32();
        C.Kind = static_cast<EntityKind>(R.ReadUInt8());
        C.Owner = R.ReadUInt8();

        TransformComp& T = Transforms[I];
        T.Position.X.Raw = R.ReadInt64();
        T.Position.Y.Raw = R.ReadInt64();
        T.Facing = R.ReadInt32();
        T.TurretFacing = R.ReadInt32();

        HealthComp& H = Healths[I];
        H.Current = R.ReadInt32();
        H.Max = R.ReadInt32();
        H.bInvulnerable = R.ReadBool();

        MovementComp& M = Movements[I];
        M.Destination.X.Raw = R.ReadInt64();
        M.Destination.Y.Raw = R.ReadInt64();
        M.bHasDestination = R.ReadBool();
        M.CurrentSpeed.Raw = R.ReadInt64();
        M.ArriveRadius.Raw = R.ReadInt64();
        M.BlockedTicks = R.ReadInt32();

        CombatComp& Cm = Combats[I];
        Cm.Target.Index = R.ReadUInt32();
        Cm.Target.Generation = R.ReadUInt32();
        Cm.CooldownTicks = R.ReadInt32();
        // v5 added the acquisition retry countdown. A v4 save has no such field, and
        // defaulting it to 0 is the correct migration: it means "may search this
        // tick", which is exactly the behaviour v4 had unconditionally.
        Cm.AcquireCooldownTicks = (Version >= 5) ? R.ReadInt32() : 0;
        Cm.bTargetIsForced = R.ReadBool();

        BuildingComp& B = Buildings[I];
        B.OriginTile.X = R.ReadInt32();
        B.OriginTile.Y = R.ReadInt32();
        B.FootprintX = R.ReadInt32();
        B.FootprintY = R.ReadInt32();
        B.State = static_cast<ConstructionState>(R.ReadUInt8());
        B.ConstructionProgressTicks = R.ReadInt32();
        B.ConstructionTotalTicks = R.ReadInt32();
        B.RallyPoint.X.Raw = R.ReadInt64();
        B.RallyPoint.Y.Raw = R.ReadInt64();
        B.bHasRallyPoint = R.ReadBool();
        B.bSelling = R.ReadBool();
        const uint32_t QueueCount = R.ReadUInt32();
        B.Queue.resize(QueueCount);
        for (uint32_t Q = 0; Q < QueueCount; ++Q)
        {
            ProductionItem& Item = B.Queue[Q];
            Item.Content.Value = R.ReadUInt32();
            Item.ProgressTicks = R.ReadInt32();
            Item.TotalTicks = R.ReadInt32();
            Item.PaidCredits = R.ReadInt32();
            Item.bPaused = R.ReadBool();
        }

        HarvesterComp& Hv = Harvesters[I];
        Hv.State = static_cast<HarvesterState>(R.ReadUInt8());
        Hv.Cargo = R.ReadInt32();
        Hv.AssignedNode.Index = R.ReadUInt32();
        Hv.AssignedNode.Generation = R.ReadUInt32();
        Hv.AssignedRefinery.Index = R.ReadUInt32();
        Hv.AssignedRefinery.Generation = R.ReadUInt32();

        ResourceNodeComp& Rn = ResourceNodes[I];
        Rn.Amount = R.ReadInt32();
        Rn.Def.Value = R.ReadUInt32();

        OrderQueue& Oq = Orders[I];
        Oq.Count = R.ReadInt32();
        for (int32_t K = 0; K < Oq.Count; ++K)
        {
            Order& Ord = Oq.Orders[K];
            Ord.Type = static_cast<OrderType>(R.ReadUInt8());
            Ord.Location.X.Raw = R.ReadInt64();
            Ord.Location.Y.Raw = R.ReadInt64();
            Ord.Target.Index = R.ReadUInt32();
            Ord.Target.Generation = R.ReadUInt32();
        }

        DirectControlComp& Dc = DirectControls[I];
        Dc.Phase = static_cast<DirectControlPhase>(R.ReadUInt8());
        Dc.Controller = R.ReadUInt8();
        Dc.PhaseUntilTick = static_cast<TickIndex>(R.ReadUInt32());
        Dc.TurretYawCentiDeg = R.ReadInt32();
        Dc.TurretPitchCentiDeg = R.ReadInt32();
        Dc.CooldownTicksPrimary = R.ReadInt32();
        Dc.CooldownTicksSecondary = R.ReadInt32();
        Dc.bOpticsZoomed = (R.ReadUInt8() != 0);
        Recon::MoraleComp& Mo = Morales[I];
        Mo.Morale = Fixed::FromRaw(R.ReadInt64());
        Mo.Fatigue = Fixed::FromRaw(R.ReadInt64());
        Mo.Suppression = Fixed::FromRaw(R.ReadInt64());
        Mo.TicksUnderFire = R.ReadInt32();
        // v6 added the projectile payload. A pre-v6 save carries none, and leaving
        // the default-constructed component is the honest migration: those saves
        // genuinely lost their in-flight shells at write time, so there is nothing
        // to recover. The entity is destroyed on the next tick by the invalid-weapon
        // path in SystemProjectiles, which is the pre-existing v5 behaviour.
        if (Version >= 6)
        {
            ProjectileComp& Pr = Projectiles[I];
            Pr.Source.Index = R.ReadUInt32();
            Pr.Source.Generation = R.ReadUInt32();
            Pr.Target.Index = R.ReadUInt32();
            Pr.Target.Generation = R.ReadUInt32();
            Pr.ImpactPoint.X.Raw = R.ReadInt64();
            Pr.ImpactPoint.Y.Raw = R.ReadInt64();
            Pr.Weapon.Value = R.ReadUInt32();
            Pr.OwnerPlayer = R.ReadUInt8();
            Pr.Speed.Raw = R.ReadInt64();
        }
    }

    BuildNavigationGrid();
    Reservations = std::make_unique<Nav::ReservationGrid>(Map.Width, Map.Height);
    Router = std::make_unique<Nav::MNavRouter>(*NavigationGrid);
    FogGrid = std::make_unique<FFogOfWarGrid>(Map.Width, Map.Height, kMaxPlayers);

    for (PlayerId P = 0; P < kMaxPlayers; ++P)
    {
        if (Players[P].bActive)
        {
            RefreshPlayerTech(P);
        }
    }

    // Reset() cleared the intel layer; re-arm it with the settings this session
    // was initialized with before reading the belief payload. Enabled-ness must
    // match the save or ReconSystem::Deserialize refuses (see its comment).
    ReconLayer.Initialize(ReconSettingsRef, Map.Width, Map.Height);
    if (Version >= 3)
    {
        if (!ReconLayer.Deserialize(R))
        {
            return false;
        }
    }
    else if (ReconLayer.IsEnabled())
    {
        // A pre-intel save has no belief state to restore; refusing beats
        // silently starting the HQ map empty mid-match.
        return false;
    }

    return !R.HasError();
}

} // namespace RA4
